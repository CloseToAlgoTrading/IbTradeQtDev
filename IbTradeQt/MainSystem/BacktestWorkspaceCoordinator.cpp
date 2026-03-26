#include "BacktestWorkspaceCoordinator.h"
#include "NHelper.h"
#include "IBacktestSessionSwitchPrompt.h"
#include "ISystemBackend.h"
#include "ibtradesystemview.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/PreparePreflightDialog.h"
#include "BacktestUI/YahooSymbolCheckDialog.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "Backtest/BacktestController.h"
#include "Backtest/BacktestPreFlightCoordinator.h"
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/DataQuality.h"
#include <cmath>
#include "cbasicroot.h"
#include "Strategies/Generic/ModelType.h"
#include "Strategies/Generic/cgenericmodelApi.h"
#include "DB/dbquery.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QtSql/QSqlDatabase>

#include "Pipeline/UniverseResolver.h"

using Backtest::Workspace::Session;
using Backtest::Workspace::SessionKey;
using Backtest::Workspace::SessionKind;
using Backtest::Workspace::recomputeSessionDirty;

namespace {

double maxDrawdownFromEquityCurve(const QVector<Backtest::LedgerSnapshot>& curve)
{
    double peak = -1.0;
    double maxDd = 0.0;
    for (const auto& s : curve) {
        const double v = s.portfolioValue;
        if (peak < 0.0)
            peak = v;
        peak = qMax(peak, v);
        if (peak > 1e-12)
            maxDd = qMax(maxDd, (peak - v) / peak);
    }
    return maxDd;
}

double annualizedReturnFromTotalReturn(double totalReturn, double years)
{
    if (years <= 1e-9 || totalReturn <= -1.0 + 1e-12)
        return 0.0;
    return std::pow(1.0 + totalReturn, 1.0 / years) - 1.0;
}

/// Older persistence wrote benchmarkValue 0 when strategy and benchmark series lengths differed.
/// PnL then plots as ~ -initialCapital. Forward-fill from last valid level after load.
void repairBenchmarkEquityMisstoredZeros(QVector<Backtest::LedgerSnapshot>& bm)
{
    if (bm.isEmpty())
        return;
    double lastValid = -1.0;
    for (int k = 0; k < bm.size(); ++k) {
        const double v = bm[k].portfolioValue;
        if (v > 1e-6) {
            lastValid = v;
        } else if (lastValid > 0.0) {
            bm[k].portfolioValue = lastValid;
        }
    }
}

} // namespace

namespace {

struct VersionLookup {
    QString versionId;
    int versionNumber = 0;

    bool isValid() const { return !versionId.isEmpty(); }
};

VersionLookup findMatchingVersion(ISystemBackend* backend,
                                  const QString& catalogStrategyId,
                                  const QJsonObject& cfg)
{
    VersionLookup match;
    if (!backend)
        return match;

    const QString canon = Backtest::Workspace::canonicalJsonString(cfg);
    for (const QJsonValue& v : backend->listStrategyVersions(catalogStrategyId)) {
        const QJsonObject obj = v.toObject();
        const QJsonDocument dj =
            QJsonDocument::fromJson(obj.value(QStringLiteral("configJson")).toString().toUtf8());
        if (!dj.isObject())
            continue;
        if (Backtest::Workspace::canonicalJsonString(dj.object()) != canon)
            continue;

        match.versionId = obj.value(QStringLiteral("versionId")).toString();
        match.versionNumber = obj.value(QStringLiteral("versionNumber")).toInt(0);
        return match;
    }

    return match;
}

VersionLookup findVersionById(ISystemBackend* backend,
                              const QString& catalogStrategyId,
                              const QString& versionId)
{
    VersionLookup match;
    if (!backend || catalogStrategyId.isEmpty() || versionId.isEmpty())
        return match;

    for (const QJsonValue& v : backend->listStrategyVersions(catalogStrategyId)) {
        const QJsonObject obj = v.toObject();
        if (obj.value(QStringLiteral("versionId")).toString() != versionId)
            continue;

        match.versionId = versionId;
        match.versionNumber = obj.value(QStringLiteral("versionNumber")).toInt(0);
        return match;
    }

    return match;
}

bool sessionUsesTemporaryStateWarning(const Session& s)
{
    return s.dirty && s.key.kind == SessionKind::CatalogVersion;
}

QString temporaryBacktestStateLossMessage(const Session& s)
{
    QString message;
    if (!s.lastRunId.isEmpty()) {
        message += QStringLiteral(
            "The last backtest run is already stored in Run History.\n\n");
    }
    message += QStringLiteral(
        "The current backtest workspace state is temporary. "
        "If you switch strategies now, this backtest workspace state will be discarded "
        "unless you explicitly save it as a new version first.");
    return message;
}

QStringList runSymbolsForHistoricalBars(const Backtest::BacktestRunConfig& config,
                                        const Backtest::BacktestRunRecord& record)
{
    QStringList symbols = config.symbols;
    if (symbols.isEmpty()) {
        const QStringList persisted = record.symbols.split(
            QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString& symbol : persisted)
            symbols.append(symbol.trimmed());
    }

    QStringList normalized;
    for (const QString& symbol : symbols) {
        const QString trimmed = symbol.trimmed();
        if (!trimmed.isEmpty() && !normalized.contains(trimmed))
            normalized.append(trimmed);
    }
    return normalized;
}

QDateTime historicalBarsFromDate(const Backtest::BacktestRunConfig& config,
                                 const Backtest::BacktestRunRecord& record)
{
    if (config.startDate.isValid())
        return config.startDate.toUTC();
    return QDateTime::fromString(record.startDate, Qt::ISODate).toUTC();
}

QDateTime historicalBarsToDate(const Backtest::BacktestRunConfig& config,
                               const Backtest::BacktestRunRecord& record)
{
    if (config.endDate.isValid())
        return config.endDate.toUTC();
    return QDateTime::fromString(record.endDate, Qt::ISODate).toUTC();
}

QMap<QString, QList<DbHistoricalBar>> loadHistoricalBarsForRun(
    const Backtest::BacktestRunConfig& config,
    const Backtest::BacktestRunRecord& record,
    const QString& conn)
{
    QMap<QString, QList<DbHistoricalBar>> barsBySymbol;

    const QStringList symbols = runSymbolsForHistoricalBars(config, record);
    if (symbols.isEmpty())
        return barsBySymbol;

    const QDateTime from = historicalBarsFromDate(config, record);
    const QDateTime to = historicalBarsToDate(config, record);
    if (!from.isValid() || !to.isValid() || from > to)
        return barsBySymbol;

    for (const QString& symbol : symbols) {
        auto q = query_fetchHistoricalBars(symbol,
                                           config.resolution,
                                           config.dataSourceId,
                                           from.toString(Qt::ISODate),
                                           to.toString(Qt::ISODate),
                                           conn);
        if (!q.exec())
            continue;

        QList<DbHistoricalBar> symbolBars;
        while (q.next()) {
            DbHistoricalBar bar;
            bar.symbol = symbol;
            bar.resolution = config.resolution;
            bar.dataSourceId = config.dataSourceId;
            bar.timestamp = q.value(QStringLiteral("timestamp")).toString();
            bar.open = q.value(QStringLiteral("open")).toDouble();
            bar.high = q.value(QStringLiteral("high")).toDouble();
            bar.low = q.value(QStringLiteral("low")).toDouble();
            bar.close = q.value(QStringLiteral("close")).toDouble();
            bar.volume = q.value(QStringLiteral("volume")).toDouble();
            symbolBars.append(bar);
        }

        if (!symbolBars.isEmpty())
            barsBySymbol.insert(symbol, symbolBars);
    }

    return barsBySymbol;
}

std::optional<SessionKey> sessionKeyForRun(const Backtest::BacktestLoadedRun& run)
{
    if (!run.record.configJson.isEmpty()) {
        const QJsonDocument configDoc =
            QJsonDocument::fromJson(run.record.configJson.toUtf8());
        if (configDoc.isObject()) {
            const Backtest::BacktestRunConfig config =
                Backtest::BacktestRunConfig::fromJson(configDoc.object());
            if (!config.strategyId.isEmpty()) {
                SessionKey key;
                key.kind = SessionKind::LiveNode;
                key.nodeId = config.strategyId;
                return key;
            }
            if (!config.catalogStrategyId.isEmpty() && !config.catalogVersionId.isEmpty()) {
                SessionKey key;
                key.kind = SessionKind::CatalogVersion;
                key.strategyId = config.catalogStrategyId;
                key.versionId = config.catalogVersionId;
                return key;
            }
        }
    }

    if (!run.record.catalogStrategyId.isEmpty() && !run.record.catalogVersionId.isEmpty()) {
        SessionKey key;
        key.kind = SessionKind::CatalogVersion;
        key.strategyId = run.record.catalogStrategyId;
        key.versionId = run.record.catalogVersionId;
        return key;
    }

    if (!run.record.strategyId.isEmpty()) {
        SessionKey key;
        key.kind = SessionKind::LiveNode;
        key.nodeId = run.record.strategyId;
        return key;
    }

    return std::nullopt;
}

} // namespace

BacktestWorkspaceCoordinator::BacktestWorkspaceCoordinator(QObject* parent)
    : QObject(parent)
    , m_unsavedPrompt(std::make_unique<QtUnsavedChangesPrompt>())
    , m_backtestSwitchPrompt(std::make_unique<QtBacktestSessionSwitchPrompt>())
{
}

BacktestWorkspaceCoordinator::~BacktestWorkspaceCoordinator() = default;


void BacktestWorkspaceCoordinator::setUnsavedChangesPrompt(
    std::unique_ptr<IUnsavedChangesPrompt> prompt)
{
    if (prompt)
        m_unsavedPrompt = std::move(prompt);
    else
        m_unsavedPrompt = std::make_unique<QtUnsavedChangesPrompt>();
}

void BacktestWorkspaceCoordinator::setBacktestSessionSwitchPrompt(
    std::unique_ptr<IBacktestSessionSwitchPrompt> prompt)
{
    if (prompt)
        m_backtestSwitchPrompt = std::move(prompt);
    else
        m_backtestSwitchPrompt = std::make_unique<QtBacktestSessionSwitchPrompt>();
}

bool BacktestWorkspaceCoordinator::isBacktestRunning() const
{
    return m_controller && m_controller->isRunning();
}

void BacktestWorkspaceCoordinator::setView(CIBTradeSystemView* view) { m_view = view; }
void BacktestWorkspaceCoordinator::setBackend(ISystemBackend* backend) { m_backend = backend; }
void BacktestWorkspaceCoordinator::setDock(BacktestUI::BacktestWorkspaceDock* dock)
{
    m_dock = dock;
    if (!m_dock)
        return;

    if (m_activeKey && m_sessions.contains(*m_activeKey)) {
        activateSession(*m_activeKey, true);
    } else {
        m_dock->showEmptyState();
    }
}

void BacktestWorkspaceCoordinator::ensureController()
{
    if (m_controller)
        return;
    m_controller = new Backtest::BacktestController(
        NHelper::getStorageConfig().backtestStore.path, nullptr, this);

    connect(m_controller, &Backtest::BacktestController::progressChanged,
            m_dock, &BacktestUI::BacktestWorkspaceDock::setProgress);
    connect(m_controller, &Backtest::BacktestController::statusChanged,
            m_dock, &BacktestUI::BacktestWorkspaceDock::setStatus);
    connect(m_controller, &Backtest::BacktestController::finished,
            this, &BacktestWorkspaceCoordinator::onBacktestFinished);
    connect(m_controller, &Backtest::BacktestController::failed,
            this, &BacktestWorkspaceCoordinator::onBacktestFailed);
}

void BacktestWorkspaceCoordinator::wireSignals()
{
    if (!m_dock || !m_view) return;

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::runRequested,
            this, [this](const Backtest::BacktestRunConfig& config) {
        ensureController();
        if (!m_controller) return;
        Backtest::BacktestRunConfig runCfg = config;
        mergeSessionAssetListIntoRunConfig(runCfg);

        if (runCfg.dataSourceId == QLatin1String("yahoo")) {
            QStringList syms =
                Backtest::BacktestPreFlightCoordinator::resolveRunConfigSymbols(runCfg).symbols;
            const QString bm = runCfg.benchmarkSymbol.trimmed().toUpper();
            if (!bm.isEmpty() && !syms.contains(bm))
                syms.append(bm);
            if (syms.isEmpty()) {
                QMessageBox::warning(
                    m_dock ? m_dock->window() : nullptr,
                    QStringLiteral("Yahoo symbol check"),
                    QStringLiteral(
                        "No symbols to validate. Add symbols or configure a pipeline with a static universe."));
                return;
            }
            m_pendingYahooRun = runCfg;
            m_dock->setRunning(true);
            auto* flight = new Backtest::BacktestPreFlightCoordinator(this);
            QObject::connect(
                flight, &Backtest::BacktestPreFlightCoordinator::yahooSymbolCheckFinished, this,
                [this, flight](const Backtest::YahooUniverseValidator::Result& r) {
                    flight->deleteLater();
                    onYahooRunValidationFinished(r);
                });
            flight->requestYahooSymbolCheckAsync(syms);
            return;
        }

        m_dock->setRunning(true);
        m_controller->start(runCfg);
    });

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::prepareRunRequested,
            this, [this](const Backtest::BacktestRunConfig& config) { launchPrepareRun(config); });

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::loadRunRequested,
            this, &BacktestWorkspaceCoordinator::onLoadRun);

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::deleteRunRequested,
            this, &BacktestWorkspaceCoordinator::onDeleteRunRequested);

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::stopBacktestRequested,
            this, [this]() {
                ensureController();
                if (m_controller)
                    m_controller->requestStop();
            });

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::userWorkspacePipelineEdited,
            this, &BacktestWorkspaceCoordinator::onUserPipelineEdited);
    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::userRunFieldsEdited,
            this, &BacktestWorkspaceCoordinator::onUserRunFieldsEdited);
    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::saveChangesRequested,
            this, &BacktestWorkspaceCoordinator::onSaveChangesRequested);
    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::resetToBaselineRequested,
            this, &BacktestWorkspaceCoordinator::onResetToBaselineRequested);
    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::saveAsNewVersionRequested,
            this, &BacktestWorkspaceCoordinator::onSaveAsNewVersionRequested);

    auto* selector = m_view->backtestStrategySelector();
    if (selector) {
        connect(selector, &BacktestUI::BacktestStrategySelector::strategySelected,
                this, [this](const QString& id, const QString& name,
                              const QString& path, const QJsonObject& cfg) {
            openStrategy(id, name, path, cfg);
        });

        connect(selector, &BacktestUI::BacktestStrategySelector::catalogVersionSelected,
                this, &BacktestWorkspaceCoordinator::openCatalogVersion);

        connect(selector, &BacktestUI::BacktestStrategySelector::blockSelected,
                this, [this](const QString& cat, const QString& key,
                              bool isArr, int idx, const QJsonObject&) {
            if (!m_dock || !m_activeKey || !m_sessions.contains(*m_activeKey))
                return;
            Session& s = m_sessions[*m_activeKey];
            // Use session working pipeline only — tree PipelineJsonRole can lag edits from
            // Block Details and would overwrite unsaved changes.
            syncActiveSessionFromPanel();
            recomputeSessionDirty(s);
            m_dock->setSessionDirtyState(s.dirty);
            m_dock->showBlockDetails(cat, key, isArr, idx, s.workingPipeline);
        });

        connect(selector, &BacktestUI::BacktestStrategySelector::refreshRequested,
                this, &BacktestWorkspaceCoordinator::refreshStrategies);
    }
}

void BacktestWorkspaceCoordinator::syncActiveSessionFromPanel()
{
    if (!m_dock || !m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    Session& s = m_sessions[*m_activeKey];
    auto* panel = m_dock->runConfigPanel();
    if (!panel)
        return;
    s.workingRunFields = panel->runFieldsSnapshot();
    const QString merged = panel->mergedPipelineConfigJson();
    if (!merged.isEmpty()) {
        const QJsonDocument d = QJsonDocument::fromJson(merged.toUtf8());
        if (d.isObject())
            s.workingPipeline = d.object();
    }
    recomputeSessionDirty(s);
}

void BacktestWorkspaceCoordinator::onUserPipelineEdited(const QJsonObject& pipeline)
{
    if (!m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    Session& s = m_sessions[*m_activeKey];
    if (auto* panel = m_dock ? m_dock->runConfigPanel() : nullptr)
        panel->setWorkingPipelineFromJson(pipeline);
    s.workingPipeline = pipeline;
    syncActiveSessionFromPanel();
    recomputeSessionDirty(s);
    m_dock->setSessionDirtyState(s.dirty);
    if (s.lastRunId.isEmpty())
        return;
    s.resultsStale = true;
    m_dock->setResultsStale(true);
}

void BacktestWorkspaceCoordinator::onUserRunFieldsEdited()
{
    syncActiveSessionFromPanel();
    if (!m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    Session& s = m_sessions[*m_activeKey];
    m_dock->setSessionDirtyState(s.dirty);
    if (!s.lastRunId.isEmpty()) {
        s.resultsStale = true;
        m_dock->setResultsStale(true);
    }
}

void BacktestWorkspaceCoordinator::onSaveChangesRequested()
{
    if (!m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    Session& s = m_sessions[*m_activeKey];
    if (s.key.kind != SessionKind::LiveNode) {
        QMessageBox::information(m_view, QStringLiteral("Save"),
            QStringLiteral("Use \"Save as New Version\" for catalog preview."));
        return;
    }
    if (!persistActiveLiveSession())
        return;
    m_dock->applyWorkspaceSession(s, false);
    m_dock->setSessionDirtyState(s.dirty);
}

void BacktestWorkspaceCoordinator::onResetToBaselineRequested()
{
    if (!m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    Session& s = m_sessions[*m_activeKey];
    s.workingPipeline = s.baselinePipeline;
    s.workingRunFields = s.baselineRunFields;
    recomputeSessionDirty(s);
    m_dock->applyWorkspaceSession(s, false);
}

void BacktestWorkspaceCoordinator::onSaveAsNewVersionRequested()
{
    saveAsNewVersionForActiveSession();
}

bool BacktestWorkspaceCoordinator::persistActiveLiveSession()
{
    if (!m_backend || !m_activeKey || !m_sessions.contains(*m_activeKey))
        return false;
    Session& s = m_sessions[*m_activeKey];
    if (s.key.kind != SessionKind::LiveNode)
        return false;
    syncActiveSessionFromPanel();
    if (!m_backend->updatePipelineConfig(s.key.nodeId, s.workingPipeline))
        return false;
    s.baselinePipeline = s.workingPipeline;
    s.baselineRunFields = s.workingRunFields;
    recomputeSessionDirty(s);
    return true;
}

BacktestWorkspaceCoordinator::SaveAsNewVersionResult
BacktestWorkspaceCoordinator::createNewVersionForActiveSession()
{
    SaveAsNewVersionResult result;
    if (!m_backend || !m_activeKey || !m_sessions.contains(*m_activeKey))
        return result;

    Session& s = m_sessions[*m_activeKey];
    syncActiveSessionFromPanel();

    const QString catalogStrategyId = s.strategyDefId.isEmpty()
        ? s.catalogStrategyIdForCatalogPreview
        : s.strategyDefId;
    if (catalogStrategyId.isEmpty()) {
        result.outcome = SaveAsNewVersionResult::Outcome::MissingCatalogStrategyId;
        return result;
    }

    const VersionLookup match =
        findMatchingVersion(m_backend, catalogStrategyId, s.workingPipeline);
    if (match.isValid()) {
        result.outcome = SaveAsNewVersionResult::Outcome::AlreadyExists;
        result.version.versionId = match.versionId;
        result.version.versionNumber = match.versionNumber;
        return result;
    }

    const QString newVerId = m_backend->createStrategyVersion(
        catalogStrategyId,
        s.workingPipeline,
        QStringLiteral("Saved from backtest workspace"),
        s.catalogVersionId);
    if (newVerId.isEmpty())
        return result;

    const VersionLookup created = findVersionById(m_backend, catalogStrategyId, newVerId);
    result.outcome = SaveAsNewVersionResult::Outcome::Created;
    result.version.versionId = newVerId;
    result.version.versionNumber = created.versionNumber;
    emit catalogRefreshNeeded();
    return result;
}

void BacktestWorkspaceCoordinator::rebaseSessionToSavedVersion(
    const CatalogVersionInfo& versionInfo)
{
    if (!m_activeKey || !m_sessions.contains(*m_activeKey) || versionInfo.versionId.isEmpty())
        return;

    const SessionKey oldKey = *m_activeKey;
    Session session = m_sessions.take(oldKey);
    session.catalogVersionId = versionInfo.versionId;
    if (versionInfo.versionNumber > 0)
        session.strategyVersion = versionInfo.versionNumber;
    session.baselinePipeline = session.workingPipeline;
    session.baselineRunFields = session.workingRunFields;
    recomputeSessionDirty(session);

    SessionKey newKey = oldKey;
    if (newKey.kind == SessionKind::CatalogVersion) {
        newKey.versionId = versionInfo.versionId;
        session.key = newKey;
        const QString catalogStrategyId = session.strategyDefId.isEmpty()
            ? session.catalogStrategyIdForCatalogPreview
            : session.strategyDefId;
        if (m_backend && !catalogStrategyId.isEmpty()) {
            const QString strategyName = m_backend->strategyCatalogEntry(catalogStrategyId)
                .value(QStringLiteral("name"))
                .toString();
            if (!strategyName.isEmpty()) {
                session.displayName =
                    strategyName + QStringLiteral(" v") + QString::number(session.strategyVersion);
            }
        }
    }

    if (m_sessions.contains(newKey))
        m_sessions.remove(newKey);
    m_sessions.insert(newKey, session);
    m_activeKey = newKey;

    if (m_dock) {
        m_dock->applyWorkspaceSession(m_sessions[newKey], false);
        m_dock->setSessionDirtyState(m_sessions[newKey].dirty);
    }
}

void BacktestWorkspaceCoordinator::updateSessionFromLoadedOrFinishedRun(
    const SessionKey& key,
    const Backtest::BacktestLoadedRun& run)
{
    if (!m_sessions.contains(key) || run.record.runId.isEmpty())
        return;

    Session& s = m_sessions[key];
    s.lastRunId = run.record.runId;
    s.resultsStale = false;
}

void BacktestWorkspaceCoordinator::saveAsNewVersionForActiveSession()
{
    const SaveAsNewVersionResult result = saveActiveSessionAsNewVersion();

    if (result.outcome == SaveAsNewVersionResult::Outcome::MissingCatalogStrategyId) {
        QMessageBox::warning(m_view, QStringLiteral("Save as New Version"),
            QStringLiteral("No catalog strategy id."));
        return;
    }
    if (result.outcome == SaveAsNewVersionResult::Outcome::AlreadyExists) {
        QMessageBox::information(m_view, QStringLiteral("Save as New Version"),
            QStringLiteral("This configuration already exists as version %1")
                .arg(result.version.versionNumber));
        return;
    }
    if (result.outcome == SaveAsNewVersionResult::Outcome::Failed) {
        QMessageBox::warning(m_view, QStringLiteral("Save as New Version"),
            QStringLiteral("Failed to create version."));
        return;
    }

    QMessageBox::information(m_view, QStringLiteral("Save as New Version"),
        QStringLiteral("New version created successfully."));
}

BacktestWorkspaceCoordinator::SaveAsNewVersionResult
BacktestWorkspaceCoordinator::saveActiveSessionAsNewVersion()
{
    const SaveAsNewVersionResult result = createNewVersionForActiveSession();
    if (result.outcome == SaveAsNewVersionResult::Outcome::Created
        || result.outcome == SaveAsNewVersionResult::Outcome::AlreadyExists) {
        rebaseSessionToSavedVersion(result.version);
    }
    return result;
}

Session BacktestWorkspaceCoordinator::makeLiveSession(const SessionKey& key,
                                                   const QString& displayName,
                                                   const QString& portfolioPath,
                                                   const QJsonObject& pipelineConfig,
                                                   const QString& strategyDefId,
                                                   int strategyVersion,
                                                   const QString& catalogVersionId) const
{
    Session s;
    s.key = key;
    s.displayName = displayName;
    s.portfolioPath = portfolioPath;
    s.strategyDefId = strategyDefId;
    s.strategyVersion = strategyVersion > 0 ? strategyVersion : 1;
    s.catalogVersionId = catalogVersionId;
    s.baselinePipeline = pipelineConfig;
    s.workingPipeline = pipelineConfig;
    return s;
}

bool BacktestWorkspaceCoordinator::tryResolveSessionSwitch(const SessionKey& nextKey)
{
    if (!m_activeKey || *m_activeKey == nextKey)
        return true;

    if (!m_sessions.contains(*m_activeKey))
        return true;

    Session& cur = m_sessions[*m_activeKey];
    recomputeSessionDirty(cur);
    if (!cur.dirty)
        return true;

    if (sessionUsesTemporaryStateWarning(cur)) {
        const BacktestSessionSwitchChoice choice =
            m_backtestSwitchPrompt->askContinueCancel(
                m_view,
                QStringLiteral("Temporary backtest state"),
                temporaryBacktestStateLossMessage(cur));
        if (choice == BacktestSessionSwitchChoice::CancelSwitch)
            return false;

        m_sessions.remove(*m_activeKey);
        m_activeKey.reset();
        return true;
    }

    const UnsavedPromptChoice choice = m_unsavedPrompt->askSaveDiscardCancel(
        m_view,
        QStringLiteral("Unsaved changes"),
        QStringLiteral("Save changes to the current session before switching?"));

    if (choice == UnsavedPromptChoice::Cancel)
        return false;

    if (choice == UnsavedPromptChoice::Save) {
        if (cur.key.kind == SessionKind::CatalogVersion) {
            QMessageBox::information(m_view, QStringLiteral("Unsaved changes"),
                QStringLiteral("Catalog preview changes are temporary in backtest. "
                               "Use the explicit \"Save as New Version\" button if you want "
                               "to keep them, or discard them."));
            return false;
        }
        if (!persistActiveLiveSession())
            return false;
    }

    m_sessions.remove(*m_activeKey);
    m_activeKey.reset();
    return true;
}

void BacktestWorkspaceCoordinator::activateSession(const SessionKey& key, bool clearResultPanels)
{
    if (!m_dock || !m_sessions.contains(key))
        return;
    m_activeKey = key;
    Session& s = m_sessions[key];
    recomputeSessionDirty(s);
    m_dock->applyWorkspaceSession(s, clearResultPanels);

    if (key.kind == SessionKind::LiveNode) {
        if (m_view) {
            m_view->switchToBacktestTab();
            if (auto* sel = m_view->backtestStrategySelector())
                sel->highlightStrategy(key.nodeId);
        }
        populateRunHistory(key.nodeId, s.strategyDefId);
    } else {
        if (m_view)
            m_view->switchToBacktestTab();
        populateRunHistory(QString(), s.strategyDefId);
    }
}

void BacktestWorkspaceCoordinator::openStrategy(
    const QString& strategyId, const QString& displayName,
    const QString& portfolioPath, const QJsonObject& pipelineConfig)
{
    if (!m_dock) return;

    SessionKey key;
    key.kind = SessionKind::LiveNode;
    key.nodeId = strategyId;

    if (m_activeKey && *m_activeKey == key && m_sessions.contains(key)) {
        activateSession(key, false);
        return;
    }

    if (!tryResolveSessionSwitch(key))
        return;

    QString strategyDefId;
    QString catalogVersionId;
    int     strategyVersion = 1;
    if (m_backend) {
        QJsonObject defJson = m_backend->strategyDefinitionForNode(strategyId);
        if (!defJson.isEmpty()) {
            strategyDefId    = defJson.value(QStringLiteral("strategyDefId")).toString();
            strategyVersion  = defJson.value(QStringLiteral("version")).toInt(1);
            catalogVersionId = defJson.value(QStringLiteral("versionId")).toString();
        }
    }

    if (m_sessions.contains(key)) {
        m_activeKey = key;
        activateSession(key, false);
        return;
    }

    Session s = makeLiveSession(key, displayName, portfolioPath, pipelineConfig,
                                strategyDefId, strategyVersion, catalogVersionId);
    m_sessions.insert(key, s);
    activateSession(key, true);

    Session& stored = m_sessions[key];
    syncActiveSessionFromPanel();
    stored.baselineRunFields = stored.workingRunFields;
    stored.baselinePipeline = stored.workingPipeline;
    recomputeSessionDirty(stored);
    m_dock->applyWorkspaceSession(stored, false);
}

void BacktestWorkspaceCoordinator::openCatalogVersion(
    const QString& catalogStrategyId, const QString& catalogVersionId)
{
    if (!m_backend || !m_dock) return;

    SessionKey key;
    key.kind = SessionKind::CatalogVersion;
    key.strategyId = catalogStrategyId;
    key.versionId = catalogVersionId;

    if (m_activeKey && *m_activeKey == key && m_sessions.contains(key)) {
        activateSession(key, false);
        return;
    }

    if (!tryResolveSessionSwitch(key))
        return;

    if (m_sessions.contains(key)) {
        activateSession(key, false);
        return;
    }

    QJsonArray versions = m_backend->listStrategyVersions(catalogStrategyId);
    QJsonObject verJson;
    for (const QJsonValue& v : versions) {
        QJsonObject obj = v.toObject();
        if (obj.value(QStringLiteral("versionId")).toString() == catalogVersionId) {
            verJson = obj;
            break;
        }
    }
    if (verJson.isEmpty()) return;

    QJsonArray catalog = m_backend->listStrategyCatalog(true);
    QString displayName = QStringLiteral("Strategy");
    for (const QJsonValue& c : catalog) {
        QJsonObject obj = c.toObject();
        if (obj.value(QStringLiteral("strategyId")).toString() == catalogStrategyId) {
            displayName = obj.value(QStringLiteral("name")).toString(displayName);
            break;
        }
    }

    int versionNumber = verJson.value(QStringLiteral("versionNumber")).toInt(1);
    QString configJson = verJson.value(QStringLiteral("configJson")).toString();

    QJsonDocument configDoc = QJsonDocument::fromJson(configJson.toUtf8());
    QJsonObject configObj = configDoc.object();

    Session s;
    s.key = key;
    s.displayName = displayName + QStringLiteral(" v") + QString::number(versionNumber);
    s.portfolioPath.clear();
    s.strategyDefId = catalogStrategyId;
    s.strategyVersion = versionNumber;
    s.catalogVersionId = catalogVersionId;
    s.catalogStrategyIdForCatalogPreview = catalogStrategyId;
    s.baselinePipeline = configObj;
    s.workingPipeline = configObj;

    m_sessions.insert(key, s);
    activateSession(key, true);

    Session& stored = m_sessions[key];
    syncActiveSessionFromPanel();
    stored.baselineRunFields = stored.workingRunFields;
    stored.baselinePipeline = stored.workingPipeline;
    recomputeSessionDirty(stored);
    m_dock->applyWorkspaceSession(stored, false);
}

void BacktestWorkspaceCoordinator::refreshStrategies()
{
    auto* selector = m_view ? m_view->backtestStrategySelector() : nullptr;
    if (!selector || !m_backend) return;

    CGenericModelApi* root = m_backend->dataRoot();
    if (!root) { selector->populate({}); return; }

    QList<BacktestUI::StrategyListItem> items;

    for (auto& accountPtr : root->getModels()) {
        CGenericModelApi* account = accountPtr.data();
        const QString accountName = account->getName();
        for (auto& portfolioPtr : account->getModels()) {
            CGenericModelApi* portfolio = portfolioPtr.data();
            const QString portfolioName = portfolio->getName();
            for (auto& stratPtr : portfolio->getModels()) {
                CGenericModelApi* strat = stratPtr.data();
                if (strat->modelType() != ModelType::STRATEGY_PIPELINE) continue;
                const QString stratId = strat->getId().toString(QUuid::WithoutBraces);

                BacktestUI::StrategyListItem item;
                item.strategyId    = stratId;
                item.name          = strat->getName();
                item.accountName   = accountName;
                item.portfolioName = portfolioName;
                item.pipelineConfig = m_backend->pipelineConfig(stratId);

                QJsonObject defJson = m_backend->strategyDefinitionForNode(stratId);
                if (!defJson.isEmpty()) {
                    item.strategyDefId    = defJson.value(QStringLiteral("strategyDefId")).toString();
                    item.version          = defJson.value(QStringLiteral("version")).toInt(1);
                    item.catalogVersionId = defJson.value(QStringLiteral("versionId")).toString();
                }

                items.append(item);
            }
        }
    }

    selector->populate(items);

    QList<BacktestUI::CatalogVersionItem> catalogItems;
    QJsonArray catalog = m_backend->listStrategyCatalog(false);
    for (const QJsonValue& c : catalog) {
        QJsonObject sObj = c.toObject();
        QString stratId   = sObj.value(QStringLiteral("strategyId")).toString();
        QString stratName = sObj.value(QStringLiteral("name")).toString();

        QJsonArray vers = m_backend->listStrategyVersions(stratId);
        for (const QJsonValue& v : vers) {
            QJsonObject vObj = v.toObject();
            BacktestUI::CatalogVersionItem ci;
            ci.strategyId    = stratId;
            ci.strategyName  = stratName;
            ci.versionId     = vObj.value(QStringLiteral("versionId")).toString();
            ci.versionNumber = vObj.value(QStringLiteral("versionNumber")).toInt(1);
            ci.configJson    = vObj.value(QStringLiteral("configJson")).toString();
            ci.isPublished   = vObj.value(QStringLiteral("isPublished")).toBool();
            catalogItems.append(ci);
        }
    }
    selector->populateCatalog(catalogItems);
}

void BacktestWorkspaceCoordinator::refreshActiveRunHistory()
{
    if (!m_dock || !m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    refreshRunHistoryForSession(*m_activeKey);
}

void BacktestWorkspaceCoordinator::refreshRunHistoryForSession(const SessionKey& key)
{
    if (!m_dock || !m_sessions.contains(key))
        return;

    const Session& s = m_sessions[key];
    if (key.kind == SessionKind::LiveNode)
        populateRunHistory(key.nodeId, s.strategyDefId);
    else
        populateRunHistory(QString(), s.strategyDefId);
}

void BacktestWorkspaceCoordinator::populateRunHistory(
    const QString& strategyId, const QString& strategyDefId)
{
    ensureController();
    if (!m_controller) return;
    const QString conn = m_controller->dbConnectionName();

    auto populate = [&](QSqlQuery q) {
        QList<DbBacktestRunSummary> summaries;
        if (q.exec()) {
            while (q.next()) {
                DbBacktestRunSummary s;
                s.runId        = q.value(QStringLiteral("runId")).toString();
                s.strategyId   = q.value(QStringLiteral("strategyId")).toString();
                s.symbols      = q.value(QStringLiteral("symbols")).toString();
                s.startDate    = q.value(QStringLiteral("startDate")).toString();
                s.endDate      = q.value(QStringLiteral("endDate")).toString();
                s.status       = q.value(QStringLiteral("status")).toString();
                s.dataSourceId = q.value(QStringLiteral("dataSourceId")).toString();
                s.createdAt    = q.value(QStringLiteral("createdAt")).toString();
                s.totalReturn  = q.value(QStringLiteral("totalReturn")).toDouble();
                s.sharpeRatio  = q.value(QStringLiteral("sharpeRatio")).toDouble();
                s.strategyDefId   = q.value(QStringLiteral("strategyDefId")).toString();
                s.scopeType       = q.value(QStringLiteral("scopeType")).toString();
                s.scopeRefId      = q.value(QStringLiteral("scopeRefId")).toString();
                s.strategyVersion = q.value(QStringLiteral("strategyVersion")).isNull()
                                        ? 1 : q.value(QStringLiteral("strategyVersion")).toInt();
                summaries.append(s);
            }
        }
        m_dock->setRunHistory(summaries);
    };

    if (!strategyDefId.isEmpty())
        populate(query_fetchRunsForDefinition(strategyDefId, conn));
    else if (!strategyId.isEmpty())
        populate(query_fetchRunsForStrategy(strategyId, conn));
}

void BacktestWorkspaceCoordinator::onLoadRun(const QString& runId)
{
    if (!m_dock) return;
    ensureController();
    if (!m_controller) return;

    const QString conn = m_controller->dbConnectionName();
    QSqlDatabase db = QSqlDatabase::database(conn);
    if (!db.isOpen()) return;

    Backtest::BacktestLoadedRun loaded;

    {
        auto q = query_fetchBacktestRun(runId, conn);
        if (q.exec() && q.next()) {
            loaded.record.runId               = q.value(QStringLiteral("runId")).toString();
            loaded.record.strategyId          = q.value(QStringLiteral("strategyId")).toString();
            loaded.record.strategyDisplayName = q.value(QStringLiteral("strategyDisplayName")).toString();
            loaded.record.portfolioPath       = q.value(QStringLiteral("portfolioPath")).toString();
            loaded.record.configJson          = q.value(QStringLiteral("configJson")).toString();
            loaded.record.symbols             = q.value(QStringLiteral("symbols")).toString();
            loaded.record.startDate           = q.value(QStringLiteral("startDate")).toString();
            loaded.record.endDate             = q.value(QStringLiteral("endDate")).toString();
            loaded.record.status              = q.value(QStringLiteral("status")).toString();
        }
    }

    {
        auto q = query_fetchBacktestMetrics(runId, conn);
        if (q.exec() && q.next()) {
            loaded.result.totalReturn      = q.value(QStringLiteral("totalReturn")).toDouble();
            loaded.result.annualizedReturn = q.value(QStringLiteral("annualizedReturn")).toDouble();
            loaded.result.sharpeRatio      = q.value(QStringLiteral("sharpeRatio")).toDouble();
            loaded.result.maxDrawdown      = q.value(QStringLiteral("maxDrawdown")).toDouble();
            loaded.result.winRate          = q.value(QStringLiteral("winRate")).toDouble();
            loaded.result.totalTrades      = q.value(QStringLiteral("totalTrades")).toInt();
            loaded.result.initialCapital   = q.value(QStringLiteral("initialCapital")).toDouble();
            loaded.result.finalCapital     = q.value(QStringLiteral("finalCapital")).toDouble();
            loaded.result.alphaVsBenchmark = q.value(QStringLiteral("alpha")).toDouble();
            loaded.result.benchmark.totalReturn = q.value(QStringLiteral("benchmarkReturn")).toDouble();
            loaded.result.benchmark.sharpeRatio = q.value(QStringLiteral("benchmarkSharpe")).toDouble();
            loaded.result.benchmark.symbol =
                q.value(QStringLiteral("benchmarkSymbol")).toString();
            loaded.result.benchmark.annualizedReturn =
                q.value(QStringLiteral("benchmarkAnnualizedReturn")).toDouble();
            loaded.result.benchmark.maxDrawdown =
                q.value(QStringLiteral("benchmarkMaxDrawdown")).toDouble();
            loaded.result.benchmark.startPrice =
                q.value(QStringLiteral("benchmarkStartPrice")).toDouble();
            loaded.result.benchmark.endPrice =
                q.value(QStringLiteral("benchmarkEndPrice")).toDouble();
        }
    }

    {
        auto q = query_fetchEquityCurve(runId, conn);
        if (q.exec()) {
            while (q.next()) {
                Backtest::LedgerSnapshot s;
                s.timestamp      = QDateTime::fromString(q.value(QStringLiteral("timestamp")).toString(), Qt::ISODate);
                s.portfolioValue = q.value(QStringLiteral("value")).toDouble();
                loaded.result.equityCurve.append(s);

                Backtest::LedgerSnapshot bm;
                bm.timestamp      = s.timestamp;
                bm.portfolioValue = q.value(QStringLiteral("benchmarkValue")).toDouble();
                loaded.result.benchmark.equityCurve.append(bm);
            }
        }
    }

    repairBenchmarkEquityMisstoredZeros(loaded.result.benchmark.equityCurve);

    {
        auto q = query_fetchBacktestTrades(runId, conn);
        if (q.exec()) {
            int id = 0;
            while (q.next()) {
                Backtest::FilledOrder f;
                f.orderId   = ++id;
                f.symbol    = q.value(QStringLiteral("symbol")).toString();
                f.quantity  = (q.value(QStringLiteral("side")).toString() == QLatin1String("BUY"))
                    ? q.value(QStringLiteral("quantity")).toDouble()
                    : -q.value(QStringLiteral("quantity")).toDouble();
                f.fillPrice = q.value(QStringLiteral("fillPrice")).toDouble();
                f.timestamp = QDateTime::fromString(q.value(QStringLiteral("timestamp")).toString(), Qt::ISODate);
                loaded.result.tradeLog.append(f);
            }
        }
    }

    loaded.result.startDate =
        QDateTime::fromString(loaded.record.startDate, Qt::ISODate);
    loaded.result.endDate = QDateTime::fromString(loaded.record.endDate, Qt::ISODate);
    loaded.result.dataQuality = Backtest::DataQuality::DailyBars;

    Backtest::BacktestRunConfig runConfig;
    if (!loaded.record.configJson.isEmpty()) {
        runConfig = Backtest::BacktestRunConfig::fromJson(
            QJsonDocument::fromJson(loaded.record.configJson.toUtf8()).object());
        if (loaded.result.benchmark.symbol.isEmpty())
            loaded.result.benchmark.symbol = runConfig.benchmarkSymbol;
        loaded.histBars = loadHistoricalBarsForRun(runConfig, loaded.record, conn);
    }

    const double years =
        loaded.result.startDate.secsTo(loaded.result.endDate) / (365.25 * 24.0 * 3600.0);
    // Legacy rows: benchmark annualized / max DD may be missing from metrics — derive when needed.
    if (loaded.result.benchmark.annualizedReturn == 0.0
        && std::fabs(loaded.result.benchmark.totalReturn) > 1e-15) {
        loaded.result.benchmark.annualizedReturn =
            annualizedReturnFromTotalReturn(loaded.result.benchmark.totalReturn, years);
    }
    if (!loaded.result.benchmark.equityCurve.isEmpty()) {
        const double ddFromCurve = maxDrawdownFromEquityCurve(loaded.result.benchmark.equityCurve);
        if (loaded.result.benchmark.maxDrawdown <= 0.0 && ddFromCurve > 0.0)
            loaded.result.benchmark.maxDrawdown = ddFromCurve;
    }

    m_dock->applyLoadedRunConfiguration(loaded);
    if (m_activeKey && m_sessions.contains(*m_activeKey) && !loaded.record.runId.isEmpty()) {
        updateSessionFromLoadedOrFinishedRun(*m_activeKey, loaded);
        m_dock->setResultsStale(false);
    }
    m_dock->displayResult(loaded, QStringLiteral("Ready"));
}

void BacktestWorkspaceCoordinator::onDeleteRunRequested(const QString& runId)
{
    if (runId.isEmpty() || !m_dock)
        return;
    ensureController();
    if (!m_controller)
        return;

    const auto ans = QMessageBox::question(
        m_view ? m_view->window() : nullptr,
        QStringLiteral("Delete backtest run"),
        QStringLiteral("Permanently delete this run from the database? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ans != QMessageBox::Yes)
        return;

    const QString conn = m_controller->dbConnectionName();
    if (!query_deleteBacktestRunByRunId(runId, conn)) {
        QMessageBox::warning(
            m_view ? m_view->window() : nullptr,
            QStringLiteral("Delete failed"),
            QStringLiteral("Could not delete the run."));
        return;
    }

    if (m_activeKey && m_sessions.contains(*m_activeKey)) {
        Session& s = m_sessions[*m_activeKey];
        if (s.lastRunId == runId) {
            s.lastRunId.clear();
            s.resultsStale = false;
            m_dock->clearDisplayedBacktestResult();
            m_dock->setResultsStale(false);
            recomputeSessionDirty(s);
            m_dock->setSessionDirtyState(s.dirty);
        }
    }

    refreshActiveRunHistory();
}

void BacktestWorkspaceCoordinator::onBacktestFinished(const Backtest::BacktestLoadedRun& run)
{
    if (!m_dock)
        return;

    m_dock->setRunning(false);

    const std::optional<SessionKey> finishedKey = sessionKeyForRun(run);
    if (finishedKey && m_sessions.contains(*finishedKey))
        updateSessionFromLoadedOrFinishedRun(*finishedKey, run);

    const bool affectsActive = finishedKey
        && m_activeKey
        && *finishedKey == *m_activeKey
        && m_sessions.contains(*finishedKey);
    if (!affectsActive)
        return;

    m_dock->displayResult(run);
    m_dock->setResultsStale(false);
    refreshRunHistoryForSession(*finishedKey);
}

void BacktestWorkspaceCoordinator::onBacktestFailed(const QString& reason)
{
    if (!m_dock) return;
    m_dock->setRunning(false);
    if (reason.startsWith(QStringLiteral("Cancelled")))
        m_dock->setStatus(reason);
    else
        m_dock->setStatus(QStringLiteral("Failed: ") + reason);
}

void BacktestWorkspaceCoordinator::mergeSessionAssetListIntoRunConfig(Backtest::BacktestRunConfig& runCfg)
{
    if (!m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    const Session& s = m_sessions[*m_activeKey];
    QJsonObject      al;
    if (s.key.kind == SessionKind::LiveNode && m_backend && !s.key.nodeId.isEmpty()) {
        al = m_backend->nodeInfo(s.key.nodeId).value(QStringLiteral("assetList")).toObject();
    } else if (s.key.kind == SessionKind::CatalogVersion) {
        al = s.workingPipeline.value(QStringLiteral("assetList")).toObject();
    }
    QJsonObject merged = al;
    if (!runCfg.assetListJson.isEmpty()) {
        const QJsonObject panel =
            QJsonDocument::fromJson(runCfg.assetListJson.toUtf8()).object();
        for (auto it = panel.begin(); it != panel.end(); ++it)
            merged[it.key()] = it.value();
    }
    if (!merged.isEmpty()) {
        runCfg.assetListJson =
            QString::fromUtf8(QJsonDocument(merged).toJson(QJsonDocument::Compact));
    }
}

void BacktestWorkspaceCoordinator::launchPrepareRun(const Backtest::BacktestRunConfig& config)
{
    Backtest::BacktestRunConfig runCfg = config;
    mergeSessionAssetListIntoRunConfig(runCfg);
    auto* panel = m_dock ? m_dock->runConfigPanel() : nullptr;
    if (panel)
        panel->setPrepareEnabled(false);
    auto* flight = new Backtest::BacktestPreFlightCoordinator(this);
    QObject::connect(flight, &Backtest::BacktestPreFlightCoordinator::prepareFinished, this,
                     [this, flight](const Backtest::BacktestPreFlightResult& res) {
                         flight->deleteLater();
                         onPreFlightPrepareFinished(res);
                     });
    Backtest::BacktestPreFlightCoordinator::PrepareInput in;
    in.dbFilePath = NHelper::getStorageConfig().backtestStore.path;
    in.runConfig  = runCfg;
    flight->requestPrepareAsync(in);
}

void BacktestWorkspaceCoordinator::onPreFlightPrepareFinished(const Backtest::BacktestPreFlightResult& r)
{
    if (m_dock && m_dock->runConfigPanel())
        m_dock->runConfigPanel()->setPrepareEnabled(true);
    QWidget* parent = m_dock ? m_dock->window() : nullptr;
    if (!r.ok) {
        QMessageBox::warning(parent, QStringLiteral("Prepare run"), r.errorMessage);
        return;
    }
    bool openDataManagement = false;
    PreparePreflightDialog::run(parent, r, r.needsDataAttention(), &openDataManagement);
    if (openDataManagement && m_view)
        m_view->switchToDataManagementTab();
}

void BacktestWorkspaceCoordinator::onYahooRunValidationFinished(
    const Backtest::YahooUniverseValidator::Result& r)
{
    Backtest::BacktestRunConfig cfg = m_pendingYahooRun;
    m_pendingYahooRun               = {};

    if (!m_dock)
        return;
    if (!r.failedSymbolErrors.isEmpty()) {
        const auto choice =
            YahooSymbolCheckDialog::run(m_dock->window(), r.failedSymbolErrors);
        if (choice == YahooSymbolCheckDialog::Choice::Cancelled) {
            m_dock->setRunning(false);
            return;
        }
        const QStringList kept =
            Backtest::BacktestPreFlightCoordinator::stripRunConfigRemovingYahooFailures(
                &cfg, r.failedSymbolErrors);
        if (kept.isEmpty()) {
            QMessageBox::warning(m_dock->window(), QStringLiteral("Yahoo symbol check"),
                                 QStringLiteral("No symbols remain after removing failed tickers."));
            m_dock->setRunning(false);
            return;
        }
    }
    ensureController();
    if (!m_controller) {
        m_dock->setRunning(false);
        return;
    }
    m_controller->start(cfg);
}
