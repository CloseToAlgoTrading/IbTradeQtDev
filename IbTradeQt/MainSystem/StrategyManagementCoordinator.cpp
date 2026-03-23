#include "StrategyManagementCoordinator.h"
#include "ISystemBackend.h"
#include "ibtradesystemview.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"
#include "CPortfolioConfigModel.h"
#include "Pipeline/PipelineConstants.h"
#include "cbasicroot.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QUuid>

StrategyManagementCoordinator::StrategyManagementCoordinator(QObject* parent)
    : QObject(parent)
{
}

void StrategyManagementCoordinator::setView(CIBTradeSystemView* view) { m_view = view; }
void StrategyManagementCoordinator::setBackend(ISystemBackend* backend) { m_backend = backend; }
void StrategyManagementCoordinator::setPanel(StrategyMgmt::StrategyManagementPanel* panel)
{
    m_panel = panel;
}

void StrategyManagementCoordinator::wireSignals()
{
    if (!m_panel) return;

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::strategySelected,
            this, &StrategyManagementCoordinator::onStrategySelected);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::newStrategyRequested,
            this, [this]() { onCreateStrategy(QString(), QJsonObject()); });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::metadataChanged,
            this, [this](const QString& sid, const QString& name,
                         const QString& desc, const QString& tags,
                         const QString& state) {
        if (!m_backend) return;
        m_backend->updateStrategyCatalogMeta(sid, name, desc, tags, state);
        refreshCatalog();
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::newVersionRequested,
            this, [this](const QString& strategyId, const QJsonObject& config) {
        onSaveVersion(strategyId, config, QString());
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::publishRequested,
            this, &StrategyManagementCoordinator::onPublishVersion);

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::archiveRequested,
            this, &StrategyManagementCoordinator::confirmAndDeleteStrategy);

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
        if (!m_backend) return;

        QJsonArray versions = m_backend->listStrategyVersions(strategyId);
        QJsonObject latestConfig;
        if (!versions.isEmpty()) {
            QString cfgStr = versions.last().toObject().value("configJson").toString();
            latestConfig = QJsonDocument::fromJson(cfgStr.toUtf8()).object();
        }

        bool isArray = Pipeline::categoryIsArray(category);
        QLatin1StringView key = Pipeline::categoryKey(category);
        if (key.isEmpty()) return;

        QJsonObject block;
        block[Pipeline::Key::BlockId] = blockId;
        block[Pipeline::Key::Config]  = defaultConfig;

        if (isArray) {
            QJsonArray arr = latestConfig.value(key).toArray();
            arr.append(block);
            latestConfig[key] = arr;
        } else {
            latestConfig[key] = block;
        }

        m_backend->createStrategyVersion(strategyId, latestConfig,
            QStringLiteral("Added %1 block: %2").arg(category, blockId));

        refreshCatalog();
        QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
        QJsonArray newVersions = m_backend->listStrategyVersions(strategyId);
        m_panel->showStrategyDetail(entry, newVersions);
    });

    connect(m_panel, &StrategyMgmt::StrategyManagementPanel::removeBlockRequested,
            this, [this](const QString& strategyId, const QString& category,
                         int blockIndex) {
        if (!m_backend) return;

        QJsonArray versions = m_backend->listStrategyVersions(strategyId);
        QJsonObject latestConfig;
        if (!versions.isEmpty()) {
            QString cfgStr = versions.last().toObject().value("configJson").toString();
            latestConfig = QJsonDocument::fromJson(cfgStr.toUtf8()).object();
        }

        bool isArray = Pipeline::categoryIsArray(category);
        QLatin1StringView key = Pipeline::categoryKey(category);
        if (key.isEmpty()) return;

        if (isArray) {
            QJsonArray arr = latestConfig.value(key).toArray();
            if (blockIndex >= 0 && blockIndex < arr.size())
                arr.removeAt(blockIndex);
            latestConfig[key] = arr;
        } else {
            latestConfig.remove(key);
        }

        m_backend->createStrategyVersion(strategyId, latestConfig,
            QStringLiteral("Removed %1 block").arg(category));

        refreshCatalog();
        QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
        QJsonArray newVersions = m_backend->listStrategyVersions(strategyId);
        m_panel->showStrategyDetail(entry, newVersions);
    });

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

    QJsonArray entries = m_backend->listStrategyCatalog(true);

    QMap<QString, int> versionCounts;
    QMap<QString, QJsonObject> latestConfigs;
    for (const auto& e : entries) {
        QString sid = e.toObject().value("strategyId").toString();
        QJsonArray versions = m_backend->listStrategyVersions(sid);
        versionCounts[sid] = versions.size();
        if (!versions.isEmpty()) {
            QString cfgStr = versions.last().toObject().value("configJson").toString();
            latestConfigs[sid] = QJsonDocument::fromJson(cfgStr.toUtf8()).object();
        }
    }

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

void StrategyManagementCoordinator::onRenameStrategy(const QString& /*strategyId*/,
                                                      const QString& /*newName*/)
{
    // Placeholder for future rename support
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
    m_backend->publishVersion(versionId);
    QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    QJsonArray versions = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versions);
}

void StrategyManagementCoordinator::onUnpublishVersion(const QString& /*strategyId*/,
                                                        const QString& /*versionId*/)
{
    // Placeholder for future unpublish support
}

void StrategyManagementCoordinator::onSaveVersion(const QString& strategyId,
                                                    const QJsonObject& config,
                                                    const QString& /*notes*/)
{
    if (!m_backend || !m_view || !m_panel) return;
    QString notes = QInputDialog::getText(m_view,
        QStringLiteral("New Version"),
        QStringLiteral("Notes for this version:"));
    m_backend->createStrategyVersion(strategyId, config, notes);
    refreshCatalog();
    QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    QJsonArray versions = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versions);
}

void StrategyManagementCoordinator::onDeleteVersion(const QString& /*strategyId*/,
                                                     const QString& /*versionId*/)
{
    // Placeholder for future version deletion
}

void StrategyManagementCoordinator::onDeployVersion(const QString& catalogStrategyId,
                                                     const QString& catalogVersionId,
                                                     const QString& /*targetStrategyNodeId*/)
{
    if (!m_backend || !m_view) return;

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
