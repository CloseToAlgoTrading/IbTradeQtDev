#include "BacktestWorkspaceCoordinator.h"
#include "ISystemBackend.h"
#include "ibtradesystemview.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "Backtest/BacktestController.h"
#include "cbasicroot.h"
#include "DB/dbquery.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QtSql/QSqlDatabase>

BacktestWorkspaceCoordinator::BacktestWorkspaceCoordinator(QObject* parent)
    : QObject(parent)
{
}

void BacktestWorkspaceCoordinator::setView(CIBTradeSystemView* view) { m_view = view; }
void BacktestWorkspaceCoordinator::setBackend(ISystemBackend* backend) { m_backend = backend; }
void BacktestWorkspaceCoordinator::setDock(BacktestUI::BacktestWorkspaceDock* dock) { m_dock = dock; }

void BacktestWorkspaceCoordinator::wireSignals()
{
    if (!m_dock || !m_view) return;

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::runRequested,
            this, [this](const Backtest::BacktestRunConfig& config) {
        if (!m_controller) return;
        m_dock->setRunning(true);
        m_controller->start(config);
    });

    connect(m_dock, &BacktestUI::BacktestWorkspaceDock::loadRunRequested,
            this, &BacktestWorkspaceCoordinator::onLoadRun);

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
                              bool isArr, int idx, const QJsonObject& cfg) {
            if (m_dock) m_dock->showBlockDetails(cat, key, isArr, idx, cfg);
        });

        connect(selector, &BacktestUI::BacktestStrategySelector::refreshRequested,
                this, &BacktestWorkspaceCoordinator::refreshStrategies);
    }
}

void BacktestWorkspaceCoordinator::createController()
{
    delete m_controller;
    m_controller = new Backtest::BacktestController(
        QStringLiteral("myLocalDb.sqlite"), nullptr, this);

    connect(m_controller, &Backtest::BacktestController::progressChanged,
            m_dock, &BacktestUI::BacktestWorkspaceDock::setProgress);
    connect(m_controller, &Backtest::BacktestController::statusChanged,
            m_dock, &BacktestUI::BacktestWorkspaceDock::setStatus);
    connect(m_controller, &Backtest::BacktestController::finished,
            this, &BacktestWorkspaceCoordinator::onBacktestFinished);
    connect(m_controller, &Backtest::BacktestController::failed,
            this, &BacktestWorkspaceCoordinator::onBacktestFailed);
}

void BacktestWorkspaceCoordinator::openStrategy(
    const QString& strategyId, const QString& displayName,
    const QString& portfolioPath, const QJsonObject& pipelineConfig)
{
    if (!m_dock) return;

    QString strategyDefId;
    QString catalogVersionId;
    int     strategyVersion = 1;
    if (m_backend) {
        QJsonObject defJson = m_backend->strategyDefinitionForNode(strategyId);
        if (!defJson.isEmpty()) {
            strategyDefId    = defJson.value("strategyDefId").toString();
            strategyVersion  = defJson.value("version").toInt(1);
            catalogVersionId = defJson.value("versionId").toString();
        }
    }

    createController();

    Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            pipelineConfig.value("backtestProfile").toObject());

    const QString pipelineConfigJson = QString::fromUtf8(
        QJsonDocument(pipelineConfig).toJson(QJsonDocument::Compact));

    m_dock->selectStrategy(strategyId, displayName, portfolioPath,
                           profile, pipelineConfigJson,
                           strategyDefId, strategyVersion, catalogVersionId);

    if (m_view) {
        m_view->switchToBacktestTab();
        if (auto* sel = m_view->backtestStrategySelector())
            sel->highlightStrategy(strategyId);
    }

    populateRunHistory(strategyId, strategyDefId);
}

void BacktestWorkspaceCoordinator::openCatalogVersion(
    const QString& catalogStrategyId, const QString& catalogVersionId)
{
    if (!m_backend || !m_dock) return;

    QJsonArray versions = m_backend->listStrategyVersions(catalogStrategyId);
    QJsonObject verJson;
    for (const QJsonValue& v : versions) {
        QJsonObject obj = v.toObject();
        if (obj.value("versionId").toString() == catalogVersionId) {
            verJson = obj;
            break;
        }
    }
    if (verJson.isEmpty()) return;

    QJsonArray catalog = m_backend->listStrategyCatalog(true);
    QString displayName = QStringLiteral("Strategy");
    for (const QJsonValue& c : catalog) {
        QJsonObject obj = c.toObject();
        if (obj.value("strategyId").toString() == catalogStrategyId) {
            displayName = obj.value("name").toString(displayName);
            break;
        }
    }

    int versionNumber = verJson.value("versionNumber").toInt(1);
    QString configJson = verJson.value("configJson").toString();

    createController();

    QJsonDocument configDoc = QJsonDocument::fromJson(configJson.toUtf8());
    QJsonObject configObj = configDoc.object();
    Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            configObj.value("backtestProfile").toObject());

    m_dock->selectStrategy(
        QString(),
        displayName + QStringLiteral(" v") + QString::number(versionNumber),
        QString(), profile, configJson,
        catalogStrategyId, versionNumber, catalogVersionId);

    if (m_view)
        m_view->switchToBacktestTab();
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
                    item.strategyDefId    = defJson.value("strategyDefId").toString();
                    item.version          = defJson.value("version").toInt(1);
                    item.catalogVersionId = defJson.value("versionId").toString();
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
        QString stratId   = sObj.value("strategyId").toString();
        QString stratName = sObj.value("name").toString();

        QJsonArray vers = m_backend->listStrategyVersions(stratId);
        for (const QJsonValue& v : vers) {
            QJsonObject vObj = v.toObject();
            BacktestUI::CatalogVersionItem ci;
            ci.strategyId    = stratId;
            ci.strategyName  = stratName;
            ci.versionId     = vObj.value("versionId").toString();
            ci.versionNumber = vObj.value("versionNumber").toInt(1);
            ci.configJson    = vObj.value("configJson").toString();
            ci.isPublished   = vObj.value("isPublished").toBool();
            catalogItems.append(ci);
        }
    }
    selector->populateCatalog(catalogItems);
}

void BacktestWorkspaceCoordinator::populateRunHistory(
    const QString& strategyId, const QString& strategyDefId)
{
    if (!m_controller) return;
    const QString conn = m_controller->dbConnectionName();

    auto populate = [&](QSqlQuery q) {
        QList<DbBacktestRunSummary> summaries;
        if (q.exec()) {
            while (q.next()) {
                DbBacktestRunSummary s;
                s.runId        = q.value("runId").toString();
                s.strategyId   = q.value("strategyId").toString();
                s.symbols      = q.value("symbols").toString();
                s.startDate    = q.value("startDate").toString();
                s.endDate      = q.value("endDate").toString();
                s.status       = q.value("status").toString();
                s.dataSourceId = q.value("dataSourceId").toString();
                s.createdAt    = q.value("createdAt").toString();
                s.totalReturn  = q.value("totalReturn").toDouble();
                s.sharpeRatio  = q.value("sharpeRatio").toDouble();
                s.strategyDefId   = q.value("strategyDefId").toString();
                s.scopeType       = q.value("scopeType").toString();
                s.scopeRefId      = q.value("scopeRefId").toString();
                s.strategyVersion = q.value("strategyVersion").isNull()
                                        ? 1 : q.value("strategyVersion").toInt();
                summaries.append(s);
            }
        }
        m_dock->setRunHistory(summaries);
    };

    if (!strategyDefId.isEmpty())
        populate(query_fetchRunsForDefinition(strategyDefId, conn));
    else
        populate(query_fetchRunsForStrategy(strategyId, conn));
}

void BacktestWorkspaceCoordinator::onLoadRun(const QString& runId)
{
    if (!m_dock || !m_controller) return;

    const QString conn = m_controller->dbConnectionName();
    QSqlDatabase db = QSqlDatabase::database(conn);
    if (!db.isOpen()) return;

    Backtest::BacktestLoadedRun loaded;

    {
        auto q = query_fetchBacktestRun(runId, conn);
        if (q.exec() && q.next()) {
            loaded.record.runId               = q.value("runId").toString();
            loaded.record.strategyId          = q.value("strategyId").toString();
            loaded.record.strategyDisplayName = q.value("strategyDisplayName").toString();
            loaded.record.portfolioPath       = q.value("portfolioPath").toString();
            loaded.record.symbols             = q.value("symbols").toString();
            loaded.record.startDate           = q.value("startDate").toString();
            loaded.record.endDate             = q.value("endDate").toString();
            loaded.record.status              = q.value("status").toString();
        }
    }

    {
        auto q = query_fetchBacktestMetrics(runId, conn);
        if (q.exec() && q.next()) {
            loaded.result.totalReturn      = q.value("totalReturn").toDouble();
            loaded.result.annualizedReturn = q.value("annualizedReturn").toDouble();
            loaded.result.sharpeRatio      = q.value("sharpeRatio").toDouble();
            loaded.result.maxDrawdown      = q.value("maxDrawdown").toDouble();
            loaded.result.winRate          = q.value("winRate").toDouble();
            loaded.result.totalTrades      = q.value("totalTrades").toInt();
            loaded.result.initialCapital   = q.value("initialCapital").toDouble();
            loaded.result.finalCapital     = q.value("finalCapital").toDouble();
            loaded.result.alphaVsBenchmark = q.value("alpha").toDouble();
        }
    }

    {
        auto q = query_fetchEquityCurve(runId, conn);
        if (q.exec()) {
            while (q.next()) {
                Backtest::LedgerSnapshot s;
                s.timestamp      = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
                s.portfolioValue = q.value("value").toDouble();
                loaded.result.equityCurve.append(s);

                Backtest::LedgerSnapshot bm;
                bm.timestamp      = s.timestamp;
                bm.portfolioValue = q.value("benchmarkValue").toDouble();
                loaded.result.benchmark.equityCurve.append(bm);
            }
        }
    }

    {
        auto q = query_fetchBacktestTrades(runId, conn);
        if (q.exec()) {
            int id = 0;
            while (q.next()) {
                Backtest::FilledOrder f;
                f.orderId   = ++id;
                f.symbol    = q.value("symbol").toString();
                f.quantity  = (q.value("side").toString() == QLatin1String("BUY"))
                    ? q.value("quantity").toDouble()
                    : -q.value("quantity").toDouble();
                f.fillPrice = q.value("fillPrice").toDouble();
                f.timestamp = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
                loaded.result.tradeLog.append(f);
            }
        }
    }

    m_dock->displayResult(loaded);
}

void BacktestWorkspaceCoordinator::onBacktestFinished(const Backtest::BacktestLoadedRun& run)
{
    if (!m_dock) return;
    m_dock->setRunning(false);
    m_dock->displayResult(run);

    const QString conn = m_controller ? m_controller->dbConnectionName()
                                       : QStringLiteral("myLocalDb.sqlite");
    auto q = query_fetchRunsForStrategy(run.record.strategyId, conn);
    if (q.exec()) {
        QList<DbBacktestRunSummary> summaries;
        while (q.next()) {
            DbBacktestRunSummary s;
            s.runId        = q.value("runId").toString();
            s.strategyId   = q.value("strategyId").toString();
            s.symbols      = q.value("symbols").toString();
            s.startDate    = q.value("startDate").toString();
            s.endDate      = q.value("endDate").toString();
            s.status       = q.value("status").toString();
            s.dataSourceId = q.value("dataSourceId").toString();
            s.createdAt    = q.value("createdAt").toString();
            s.totalReturn  = q.value("totalReturn").toDouble();
            s.sharpeRatio  = q.value("sharpeRatio").toDouble();
            summaries.append(s);
        }
        m_dock->setRunHistory(summaries);
    }

    if (m_backend && m_view
        && !run.record.catalogStrategyId.isEmpty()
        && !run.record.catalogVersionId.isEmpty())
    {
        QJsonObject versionInfo = m_backend->strategyVersionInfo(run.record.catalogVersionId);
        QString versionConfigJson = versionInfo.value("configJson").toString();
        QJsonObject fullRunConfig = QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
        QString runPipelineJson = fullRunConfig.value("pipelineConfigJson").toString();
        QJsonDocument runPipelineDoc = QJsonDocument::fromJson(runPipelineJson.toUtf8());
        QJsonDocument verConfigDoc   = QJsonDocument::fromJson(versionConfigJson.toUtf8());

        if (runPipelineDoc != verConfigDoc
            && !runPipelineDoc.isEmpty() && !verConfigDoc.isEmpty())
        {
            auto answer = QMessageBox::question(
                m_view,
                QStringLiteral("Save as New Version?"),
                QStringLiteral(
                    "The backtest ran with a pipeline configuration that differs from the pinned version.\n\n"
                    "Would you like to save the run configuration as a new strategy version?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

            if (answer == QMessageBox::Yes) {
                QJsonObject pipelineConfig = runPipelineDoc.object();
                QString newVerId = m_backend->createStrategyVersion(
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
    }
}

void BacktestWorkspaceCoordinator::onBacktestFailed(const QString& reason)
{
    if (!m_dock) return;
    m_dock->setRunning(false);
    m_dock->setStatus(QStringLiteral("Failed: ") + reason);
}
