#include "BacktestWorkspaceCoordinator.h"
#include "NHelper.h"
#include "ISystemBackend.h"
#include "ibtradesystemview.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "Backtest/BacktestController.h"
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

QString findMatchingVersionNumber(ISystemBackend* backend,
                                  const QString& catalogStrategyId,
                                  const QJsonObject& cfg)
{
    if (!backend)
        return {};
    const QString canon = Backtest::Workspace::canonicalJsonString(cfg);
    for (const QJsonValue& v : backend->listStrategyVersions(catalogStrategyId)) {
        const QJsonObject o = v.toObject();
        const QJsonDocument dj = QJsonDocument::fromJson(
            o.value(QStringLiteral("configJson")).toString().toUtf8());
        if (!dj.isObject())
            continue;
        if (Backtest::Workspace::canonicalJsonString(dj.object()) == canon) {
            const int num = o.value(QStringLiteral("versionNumber")).toInt(0);
            return QString::number(num);
        }
    }
    return {};
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

BacktestWorkspaceCoordinator::BacktestWorkspaceCoordinator(QObject* parent)
    : QObject(parent)
    , m_unsavedPrompt(std::make_unique<QtUnsavedChangesPrompt>())
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

bool BacktestWorkspaceCoordinator::isBacktestRunning() const
{
    return m_controller && m_controller->isRunning();
}

void BacktestWorkspaceCoordinator::setView(CIBTradeSystemView* view) { m_view = view; }
void BacktestWorkspaceCoordinator::setBackend(ISystemBackend* backend) { m_backend = backend; }
void BacktestWorkspaceCoordinator::setDock(BacktestUI::BacktestWorkspaceDock* dock) { m_dock = dock; }

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
        if (m_activeKey && m_sessions.contains(*m_activeKey)) {
            const Session& s = m_sessions[*m_activeKey];
            QJsonObject      al;
            if (s.key.kind == SessionKind::LiveNode && m_backend && !s.key.nodeId.isEmpty()) {
                al = m_backend->nodeInfo(s.key.nodeId).value(QStringLiteral("assetList")).toObject();
            } else if (s.key.kind == SessionKind::CatalogVersion) {
                al = s.workingPipeline.value(QStringLiteral("assetList")).toObject();
            }
            // Merge: strategy/catalog assetList first, then Run Configuration panel overrides
            // (same symbol key) so ad-hoc SYMBOL:Type in the dock wins for that run.
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
        m_dock->setRunning(true);
        m_controller->start(runCfg);
    });

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::loadRunRequested,
            this, &BacktestWorkspaceCoordinator::onLoadRun);

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

void BacktestWorkspaceCoordinator::saveAsNewVersionForActiveSession()
{
    if (!m_backend || !m_activeKey || !m_sessions.contains(*m_activeKey))
        return;
    Session& s = m_sessions[*m_activeKey];
    syncActiveSessionFromPanel();

    const QString catalogStrategyId = s.strategyDefId.isEmpty()
        ? s.catalogStrategyIdForCatalogPreview
        : s.strategyDefId;
    if (catalogStrategyId.isEmpty()) {
        QMessageBox::warning(m_view, QStringLiteral("Save as New Version"),
            QStringLiteral("No catalog strategy id."));
        return;
    }

    const QString matchVer = findMatchingVersionNumber(
        m_backend, catalogStrategyId, s.workingPipeline);
    if (!matchVer.isEmpty()) {
        QMessageBox::information(m_view, QStringLiteral("Save as New Version"),
            QStringLiteral("This configuration already exists as version %1").arg(matchVer));
        return;
    }

    const QString newVerId = m_backend->createStrategyVersion(
        catalogStrategyId,
        s.workingPipeline,
        QStringLiteral("Saved from backtest workspace"));
    if (newVerId.isEmpty()) {
        QMessageBox::warning(m_view, QStringLiteral("Save as New Version"),
            QStringLiteral("Failed to create version."));
        return;
    }
    QMessageBox::information(m_view, QStringLiteral("Save as New Version"),
        QStringLiteral("New version created successfully."));
    emit catalogRefreshNeeded();
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

    const UnsavedPromptChoice choice = m_unsavedPrompt->askSaveDiscardCancel(
        m_view,
        QStringLiteral("Unsaved changes"),
        QStringLiteral("Save changes to the current session before switching?"));

    if (choice == UnsavedPromptChoice::Cancel)
        return false;

    if (choice == UnsavedPromptChoice::Save) {
        if (cur.key.kind == SessionKind::CatalogVersion) {
            QMessageBox::information(m_view, QStringLiteral("Unsaved changes"),
                QStringLiteral("Catalog preview cannot be saved to the live tree. "
                               "Use \"Save as New Version\" or discard."));
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

    if (!loaded.record.configJson.isEmpty()) {
        const Backtest::BacktestRunConfig rc = Backtest::BacktestRunConfig::fromJson(
            QJsonDocument::fromJson(loaded.record.configJson.toUtf8()).object());
        if (loaded.result.benchmark.symbol.isEmpty())
            loaded.result.benchmark.symbol = rc.benchmarkSymbol;
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
    m_dock->displayResult(loaded);
}

void BacktestWorkspaceCoordinator::onBacktestFinished(const Backtest::BacktestLoadedRun& run)
{
    if (!m_dock) return;
    m_dock->setRunning(false);
    m_dock->displayResult(run);

    if (m_activeKey && m_sessions.contains(*m_activeKey)) {
        Session& s = m_sessions[*m_activeKey];
        s.lastRunId = run.record.runId;
        s.resultsStale = false;
        m_dock->setResultsStale(false);
    }

    ensureController();
    const QString conn = m_controller ? m_controller->dbConnectionName()
                                       : QString();
    auto q = query_fetchRunsForStrategy(run.record.strategyId, conn);
    if (q.exec()) {
        QList<DbBacktestRunSummary> summaries;
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
            summaries.append(s);
        }
        m_dock->setRunHistory(summaries);
    }

    if (m_backend && m_view
        && !run.record.catalogStrategyId.isEmpty()
        && !run.record.catalogVersionId.isEmpty())
    {
        QJsonObject fullRunConfig = QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
        QString runPipelineJson = fullRunConfig.value(QStringLiteral("pipelineConfigJson")).toString();
        QJsonDocument runPipelineDoc = QJsonDocument::fromJson(runPipelineJson.toUtf8());
        if (!runPipelineDoc.isObject() || runPipelineDoc.object().isEmpty())
            return;

        const QJsonObject pipelineConfig = runPipelineDoc.object();

        auto answer = QMessageBox::question(
            m_view,
            QStringLiteral("Save backtest pipeline?"),
            QStringLiteral(
                "The backtest run is stored in the database.\n\n"
                "Save the run's pipeline configuration as a new catalog version?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (answer != QMessageBox::Yes)
            return;

        const QString matchVer = findMatchingVersionNumber(
            m_backend, run.record.catalogStrategyId, pipelineConfig);
        if (!matchVer.isEmpty()) {
            QMessageBox::information(
                m_view,
                QStringLiteral("Save as New Version"),
                QStringLiteral("This configuration already exists as version %1.").arg(matchVer));
            return;
        }

        const QString newVerId = m_backend->createStrategyVersion(
            run.record.catalogStrategyId, pipelineConfig,
            QStringLiteral("Saved from backtest run ") + run.record.runId);
        if (!newVerId.isEmpty()) {
            QMessageBox::information(m_view,
                QStringLiteral("Version Created"),
                QStringLiteral("New version created successfully."));
            emit catalogRefreshNeeded();
        }
    }
}

void BacktestWorkspaceCoordinator::onBacktestFailed(const QString& reason)
{
    if (!m_dock) return;
    m_dock->setRunning(false);
    m_dock->setStatus(QStringLiteral("Failed: ") + reason);
}
