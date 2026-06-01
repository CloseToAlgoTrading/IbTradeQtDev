#include "StrategyManagementCoordinator.h"
#include "StrategyManagementUnsavedDraftFlow.h"
#include "ISystemBackend.h"
#include "ibtradesystemview.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyCatalogPanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"
#include "CPortfolioConfigModel.h"
#include "Pipeline/PipelineConfigMutations.h"
#include "cbasicroot.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QUuid>

StrategyManagementCoordinator::StrategyManagementCoordinator(QObject* parent)
    : QObject(parent)
{
}

StrategyManagementCoordinator::~StrategyManagementCoordinator() = default;

void StrategyManagementCoordinator::setView(CIBTradeSystemView* view) { m_view = view; }
void StrategyManagementCoordinator::setBackend(ISystemBackend* backend) { m_backend = backend; }
void StrategyManagementCoordinator::setPanel(StrategyMgmt::StrategyManagementPanel* panel)
{
    m_panel = panel;
}

void StrategyManagementCoordinator::wireSignals()
{
    if (!m_panel) return;

    m_unsavedDraftFlow = std::make_unique<StrategyManagementUnsavedDraftFlow>(
        m_backend, m_view, m_panel);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::strategySelected,
            this, [this](const QString& strategyId) {
        if (!m_panel || !m_backend || !m_unsavedDraftFlow)
            return;
        auto* detail = m_panel->detailPanel();
        if (detail && detail->isConfigDirty()
            && !detail->currentStrategyId().isEmpty()
            && detail->currentStrategyId() != strategyId) {
            if (!m_unsavedDraftFlow->tryResolveIfDirty(
                    QStringLiteral(
                        "Save or discard changes to the current strategy before selecting another?"))) {
                m_panel->catalogPanel()->selectStrategyById(detail->currentStrategyId());
                return;
            }
        }
        onStrategySelected(strategyId);
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::newStrategyRequested,
            this, [this]() { onCreateStrategy(QString(), QJsonObject()); });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::metadataChanged,
            this, &StrategyManagementCoordinator::onMetadataChanged);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::newVersionRequested,
            this, [this](const QString& strategyId, const QJsonObject& config) {
        onSaveVersion(strategyId, config, QString());
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::publishRequested,
            this, &StrategyManagementCoordinator::onPublishVersion);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::unpublishRequested,
            this, &StrategyManagementCoordinator::onUnpublishVersion);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::deleteVersionRequested,
            this, &StrategyManagementCoordinator::onDeleteVersion);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::archiveRequested,
            this, &StrategyManagementCoordinator::retireStrategy);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::deleteStrategyRequested,
            this, &StrategyManagementCoordinator::confirmAndDeleteStrategy);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::useInLiveRequested,
            this, [this](const QString& sid, const QString& vid) {
        onDeployVersion(sid, vid, QString());
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::openInBacktestRequested,
            this, &StrategyManagementCoordinator::onBacktestVersion);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::addBlockRequested,
            this, [this](const QString& strategyId, const QString& category,
                         const QString& blockId, const QJsonObject& defaultConfig) {
        if (!m_backend || !m_panel || !m_unsavedDraftFlow)
            return;

        auto* detail = m_panel->detailPanel();
        if (detail->currentStrategyId() != strategyId) {
            if (!m_unsavedDraftFlow->tryResolveIfDirty(
                    QStringLiteral(
                        "Save or discard changes before editing another strategy?"))) {
                m_panel->catalogPanel()->selectStrategyById(detail->currentStrategyId());
                return;
            }
            onStrategySelected(strategyId);
            detail = m_panel->detailPanel();
        }

        if (detail->currentVersionPublished()) {
            QMessageBox::information(
                m_view,
                QStringLiteral("Published Version Locked"),
                QStringLiteral("Published version parameters cannot be changed. Save as a new version before editing."));
            return;
        }

        QJsonObject cfg = detail->workingConfig();
        if (!Pipeline::addBlockToPipeline(cfg, category, blockId, defaultConfig))
            return;
        detail->setWorkingPipelineConfig(cfg);
        refreshCatalog();
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::removeBlockRequested,
            this, [this](const QString& strategyId, const QString& category,
                         int blockIndex) {
        if (!m_backend || !m_panel || !m_unsavedDraftFlow)
            return;

        auto* detail = m_panel->detailPanel();
        if (detail->currentStrategyId() != strategyId) {
            if (!m_unsavedDraftFlow->tryResolveIfDirty(
                    QStringLiteral(
                        "Save or discard changes before editing another strategy?"))) {
                m_panel->catalogPanel()->selectStrategyById(detail->currentStrategyId());
                return;
            }
            onStrategySelected(strategyId);
            detail = m_panel->detailPanel();
        }

        if (detail->currentVersionPublished()) {
            QMessageBox::information(
                m_view,
                QStringLiteral("Published Version Locked"),
                QStringLiteral("Published version parameters cannot be changed. Save as a new version before editing."));
            return;
        }

        QJsonObject cfg = detail->workingConfig();
        if (!Pipeline::removeBlockFromPipeline(cfg, category, blockIndex))
            return;
        detail->setWorkingPipelineConfig(cfg);
        refreshCatalog();
    });

    connect(m_panel->detailPanel(), &StrategyMgmt::StrategyDetailPanel::versionRowChangeRequested,
            this, &StrategyManagementCoordinator::onStrategyVersionRowChangeRequested);

    if (m_backend) {
        connect(m_backend, &ISystemBackend::strategyCatalogChanged,
                this, [this](const QString&) { refreshCatalog(); });
        connect(m_backend, &ISystemBackend::strategyVersionCreated,
                this, [this](const QString&, const QString&) { refreshCatalog(); });
    }
}

void StrategyManagementCoordinator::refreshCatalog()
{
    if (!m_panel || !m_backend) return;

    const QJsonArray entries = m_backend->listStrategyCatalog(true);

    QMap<QString, int> versionCounts;
    QMap<QString, QJsonObject> latestConfigs;
    for (const auto& e : entries) {
        const QJsonObject entry = e.toObject();
        const QString sid = entry.value("strategyId").toString();
        const QJsonArray versions = m_backend->listStrategyVersions(sid);
        versionCounts[sid] = versions.size();
        if (!versions.isEmpty()) {
            QString cfgStr = versions.last().toObject().value("configJson").toString();
            latestConfigs[sid] = QJsonDocument::fromJson(cfgStr.toUtf8()).object();
        }
    }

    auto* detail = m_panel->detailPanel();
    if (detail && detail->isConfigDirty() && !detail->currentStrategyId().isEmpty())
        latestConfigs[detail->currentStrategyId()] = detail->workingConfig();

    m_panel->populateCatalog(entries, versionCounts, latestConfigs);
}

void StrategyManagementCoordinator::onStrategySelected(const QString& strategyId)
{
    if (!m_backend || !m_panel) return;
    QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    QJsonArray versions = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versions);
}

void StrategyManagementCoordinator::onCreateStrategy(const QString& name,
                                                      const QJsonObject& /*initialConfig*/)
{
    if (!m_backend || !m_view) return;

    QString stratName = name;
    if (stratName.isEmpty()) {
        stratName = QInputDialog::getText(m_view, QStringLiteral("New Strategy"),
                                           QStringLiteral("Strategy name:"));
        if (stratName.isEmpty()) return;
    }

    QJsonArray existing = m_backend->listStrategyCatalog(true);
    for (const auto& e : existing) {
        if (e.toObject().value("name").toString() == stratName) {
            auto answer = QMessageBox::question(
                m_view,
                QStringLiteral("Duplicate Name"),
                QStringLiteral("A strategy named '%1' already exists. Create anyway?").arg(stratName),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
            break;
        }
    }

    m_backend->createStrategyCatalogEntry(stratName, static_cast<int>(ModelType::STRATEGY_PIPELINE));
    refreshCatalog();
}

void StrategyManagementCoordinator::onMetadataChanged(const QString& strategyId,
                                                       const QString& name,
                                                       const QString& description,
                                                       const QString& tags,
                                                       const QString& lifecycleState)
{
    if (!m_backend || !m_panel || strategyId.isEmpty())
        return;

    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        QMessageBox::warning(
            m_view,
            QStringLiteral("Strategy Name Required"),
            QStringLiteral("Strategy name cannot be empty."));
        onStrategySelected(strategyId);
        return;
    }

    if (!m_backend->updateStrategyCatalogMeta(strategyId,
                                              trimmedName,
                                              description,
                                              tags,
                                              lifecycleState)) {
        if (lifecycleState == QStringLiteral("retired")
            && m_backend->isCatalogStrategyActiveInLive(strategyId)) {
            QMessageBox::information(
                m_view,
                QStringLiteral("Cannot Retire Strategy"),
                QStringLiteral("This strategy has an enabled live deployment. Turn it off in Live Trading, then retire again."));
            onStrategySelected(strategyId);
            return;
        }
        QMessageBox::warning(
            m_view,
            QStringLiteral("Save Metadata Failed"),
            QStringLiteral("Strategy metadata could not be saved."));
        onStrategySelected(strategyId);
        return;
    }

    refreshCatalog();
    m_panel->catalogPanel()->selectStrategyById(strategyId);
    onStrategySelected(strategyId);
}

void StrategyManagementCoordinator::retireStrategy(const QString& strategyId)
{
    if (!m_backend || !m_view || !m_panel || strategyId.isEmpty())
        return;

    if (m_backend->isCatalogStrategyActiveInLive(strategyId)) {
        QMessageBox::information(
            m_view,
            QStringLiteral("Cannot Retire Strategy"),
            QStringLiteral("This strategy has an enabled live deployment. Turn it off in Live Trading, then retire again."));
        return;
    }

    const auto answer = QMessageBox::question(
        m_view,
        QStringLiteral("Retire Strategy"),
        QStringLiteral("Retire this strategy? Retired strategies keep their versions and history, but are hidden from normal catalog use."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    if (!m_backend->archiveStrategyCatalogEntry(strategyId)) {
        QMessageBox::warning(
            m_view,
            QStringLiteral("Retire Failed"),
            QStringLiteral("The strategy could not be retired. See the application log for details."));
        onStrategySelected(strategyId);
        return;
    }

    refreshCatalog();
    m_panel->catalogPanel()->selectStrategyById(strategyId);
    onStrategySelected(strategyId);
}

void StrategyManagementCoordinator::confirmAndDeleteStrategy(const QString& strategyId)
{
    if (!m_backend || !m_view || !m_panel || strategyId.isEmpty())
        return;

    if (m_backend->isCatalogStrategyActiveInLive(strategyId)) {
        QMessageBox::information(
            m_view,
            QStringLiteral("Cannot Delete Strategy"),
            QStringLiteral("This strategy is active in Live Trading (the \"On\" checkbox is enabled for at "
                           "least one deployment). Turn it off, then delete again."));
        return;
    }

    const auto answer = QMessageBox::question(
        m_view,
        QStringLiteral("Delete Strategy"),
        QStringLiteral("Permanently delete this strategy and all of its versions, backtest runs, "
                       "and live trading entries that use it? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    if (!m_backend->deleteStrategyCatalogCascade(strategyId)) {
        QMessageBox::warning(
            m_view,
            QStringLiteral("Delete Failed"),
            QStringLiteral("The strategy could not be deleted. See the application log for details."));
        return;
    }

    refreshCatalog();
    m_panel->detailPanel()->clear();
    emit refreshLiveTree();
}

void StrategyManagementCoordinator::onPublishVersion(const QString& strategyId,
                                                      const QString& versionId)
{
    if (!m_backend || !m_panel) return;
    if (!m_backend->publishVersion(versionId)) {
        QMessageBox::warning(
            m_view,
            QStringLiteral("Publish Failed"),
            QStringLiteral("The selected version could not be published."));
        return;
    }
    refreshCatalog();
    m_panel->catalogPanel()->selectStrategyById(strategyId);
    QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    QJsonArray versions = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versions);
    m_panel->detailPanel()->selectVersionById(versionId);
}

void StrategyManagementCoordinator::onUnpublishVersion(const QString& strategyId,
                                                        const QString& versionId)
{
    if (!m_backend || !m_panel) return;
    if (!m_backend->unpublishVersion(versionId)) {
        QMessageBox::information(
            m_view,
            QStringLiteral("Cannot Unpublish Version"),
            QStringLiteral("This version is still used by a live deployment. Remove or rebind that deployment before unpublishing."));
        return;
    }
    refreshCatalog();
    m_panel->catalogPanel()->selectStrategyById(strategyId);
    QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    QJsonArray versions = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versions);
    m_panel->detailPanel()->selectVersionById(versionId);
}

void StrategyManagementCoordinator::onSaveVersion(const QString& strategyId,
                                                    const QJsonObject& config,
                                                    const QString& /*notes*/)
{
    Q_UNUSED(strategyId);
    Q_UNUSED(config);
    if (!m_unsavedDraftFlow)
        return;
    m_unsavedDraftFlow->saveWorkingAsNewVersion();
}

void StrategyManagementCoordinator::onDeleteVersion(const QString& /*strategyId*/,
                                                     const QString& versionId)
{
    if (!m_backend || !m_view || !m_panel || !m_unsavedDraftFlow || versionId.isEmpty())
        return;

    auto* detail = m_panel->detailPanel();
    if (!detail)
        return;

    if (!m_unsavedDraftFlow->tryResolveIfDirty(
            QStringLiteral("Save or discard changes before deleting this version?"))) {
        return;
    }

    const QString strategyId = detail->currentStrategyId();
    if (strategyId.isEmpty())
        return;

    const QJsonArray versionsBefore = m_backend->listStrategyVersions(strategyId);
    int deletedRow = -1;
    for (int i = 0; i < versionsBefore.size(); ++i) {
        if (versionsBefore[i].toObject().value("versionId").toString() == versionId) {
            deletedRow = i;
            break;
        }
    }
    if (deletedRow < 0)
        return;

    const auto answer = QMessageBox::question(
        m_view,
        QStringLiteral("Delete Version"),
        QStringLiteral("Delete the selected strategy version? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    if (!m_backend->deleteStrategyVersion(versionId)) {
        QMessageBox::warning(
            m_view,
            QStringLiteral("Delete Version Failed"),
            QStringLiteral("This version could not be deleted. Versions that are still used by a "
                           "live deployment must be unbound first."));
        return;
    }

    const QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    const QJsonArray versionsAfter = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versionsAfter);
    if (!versionsAfter.isEmpty()) {
        const int replacementRow = qMin(deletedRow, versionsAfter.size() - 1);
        detail->selectVersionRow(replacementRow);
    }
}

void StrategyManagementCoordinator::onDeployVersion(const QString& catalogStrategyId,
                                                     const QString& catalogVersionId,
                                                     const QString& /*targetStrategyNodeId*/)
{
    if (!m_backend || !m_view) return;

    const QJsonObject entry = m_backend->strategyCatalogEntry(catalogStrategyId);
    const QJsonObject summary = entry.value(QStringLiteral("lifecycleSummary")).toObject();
    if (summary.value(QStringLiteral("lifecycle")).toString()
            == QStringLiteral("retired")) {
        QMessageBox::information(
            m_view,
            QStringLiteral("Strategy Retired"),
            QStringLiteral("Retired strategies are preserved for history and backtesting, but cannot be deployed live."));
        return;
    }

    CGenericModelApi* root = m_backend->dataRoot();
    if (!root) return;

    QStringList portfolioLabels;
    QStringList portfolioIds;
    for (auto& acct : root->getModels())
        for (auto& port : acct->getModels()) {
            QString pid = port->getId().toString(QUuid::WithoutBraces);
            portfolioLabels << acct->getName() + QStringLiteral(" / ") + port->getName();
            portfolioIds << pid;
        }

    if (portfolioIds.isEmpty()) {
        QMessageBox::warning(m_view, QStringLiteral("No Portfolios"),
            QStringLiteral("Create an account and portfolio first."));
        return;
    }

    bool ok = false;
    QString chosen = QInputDialog::getItem(
        m_view, QStringLiteral("Select Portfolio"),
        QStringLiteral("Deploy strategy to portfolio:"),
        portfolioLabels, 0, false, &ok);
    if (!ok) return;

    int idx = portfolioLabels.indexOf(chosen);
    if (idx < 0) return;
    QString portfolioId = portfolioIds.at(idx);

    QString nodeId = m_backend->createLiveNodeForExistingCatalog(
        portfolioId, ModelType::STRATEGY_PIPELINE,
        catalogStrategyId, catalogVersionId);
    if (nodeId.isEmpty()) return;

    emit refreshLiveTree();
}

void StrategyManagementCoordinator::onBacktestVersion(const QString& strategyId,
                                                       const QString& versionId)
{
    emit openInBacktest(strategyId, versionId);
}

bool StrategyManagementCoordinator::tryResolveUnsavedStrategyDraft(const QString& message)
{
    if (!m_unsavedDraftFlow)
        return true;
    return m_unsavedDraftFlow->tryResolveIfDirty(message);
}

void StrategyManagementCoordinator::onStrategyVersionRowChangeRequested(int newRow, int previousRow)
{
    if (!m_panel || !m_unsavedDraftFlow)
        return;
    auto* detail = m_panel->detailPanel();
    if (!detail || !detail->versionTable())
        return;

    {
        QSignalBlocker b(detail->versionTable());
        detail->versionTable()->selectRow(previousRow);
    }
    if (!m_unsavedDraftFlow->tryResolveIfDirty(
            QStringLiteral("Save or discard changes before switching version?"))) {
        return;
    }
    {
        QSignalBlocker b(detail->versionTable());
        detail->versionTable()->selectRow(newRow);
    }
    detail->loadVersionAtRow(newRow);
}
