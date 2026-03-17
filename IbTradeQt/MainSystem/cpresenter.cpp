#include "cpresenter.h"
#include "ReqManager.h"
#include "IBComClientImpl.h"
#include "cmainmodel.h"
#include "CPortfolioConfigModel.h"
#include "PipelineItemDelegate.h"
#include "PipelineDiagramWidget.h"
#include "cpipelinestrategyadapter.h"
#include "cstrategyfactory.h"
#include "cbasicaccount.h"
#include "cbasicportfolio.h"
#include "cbasicroot.h"
#include "Pipeline/BlockRegistry.h"
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include "PortfolioModelDefines.h"
#include "SystemTreeModel.h"
#include "SystemTreeDelegate.h"
#include "GlobalStatusBar.h"
#include "EventLogPanel.h"
#include "ContextWorkspace.h"
#include "AlertService.h"
#include <QDockWidget>
#include <QMenu>
#include <QPoint>
#include <QUuid>
#include "Backtest/BacktestController.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "DB/dbquery.h"
#include <QtSql/QSqlDatabase>
#include <QJsonDocument>


CPresenter::CPresenter(QObject *parent)
	: QObject(parent)
    , m_pLog(LOGGER)
    //, m_DataProvider()
    , m_pDataProvider(QSharedPointer<CBrokerDataProvider>::create())
    , pIbtsView(nullptr)
    , pGuiModel(nullptr)
    , threadIBClient(new QThread)
    , workerIBClient(new IBWorker::Worker(parent, *m_pDataProvider.data()))
    , threadAlfaTime(new QThread)
    , workerAlfaTime(new AlphaModGetTime(parent, *m_pDataProvider.data()))
    , pAboutDlgPresenter(new AboutDlgPresener(parent))
    // , pPairTradingPresenter(new PairTradingPresenter(parent, *m_pDataProvider.data()))
    // , pAutoDeltaAligPresenter(new AutoDeltaAligPresenter(parent, *m_pDataProvider.data()))
    // , pDBStorePresenter(new DBStorePresenter(parent, *m_pDataProvider.data()))
{
	
    //

    QSharedPointer<IBComClientImpl> pClient = QSharedPointer<IBComClientImpl>::create();

   //Define Data Provider
    m_pDataProvider->setClien(pClient);

    // Pipeline integration: create MarketDataRouter and wire to IBComClientImpl
    m_pMarketDataRouter = new IBComm::MarketDataRouter(this);
    pClient->setMarketDataRouter(m_pMarketDataRouter);

    workerIBClient->moveToThread(threadIBClient);
    QObject::connect(threadIBClient, SIGNAL(started()), workerIBClient, SLOT(process()));
    threadIBClient->start();

    QThread::currentThread()->setObjectName("mainThread");
    threadIBClient->setObjectName("myThread");

}

CPresenter::~CPresenter()
{
    workerIBClient->setCommand(IBWorker::EXIT);
}


void CPresenter::MapSignals()
{
	//MAP GUI Signals/Slots
	//Click connect button
    QObject::connect(pIbtsView->getUi().actionConnect, &QAction::triggered, this, &CPresenter::onClickMyButton);

    // //click Pair Trader button
 //    QObject::connect(pIbtsView->getUi().actionPair_Trader, &QAction::triggered, this, &CPresenter::onClickPairTraderButton);

 //    //click Auto Delta button
 //    QObject::connect(pIbtsView->getUi().actionAuto_Delta, &QAction::triggered, this, &CPresenter::onClickAutoDeltaButton);

 //    //click DBStore button
 //    QObject::connect(pIbtsView->getUi().actionDBStore, &QAction::triggered, this, &CPresenter::onClickDBStoreButton);



	//Click received Time button
	QObject::connect(workerAlfaTime, SIGNAL(signalTimeReceived(long)), pIbtsView, SLOT(slotOnTimeReceived(long)));
	//Received info about connection button state
	QObject::connect(this, SIGNAL(signalClickConnect(bool)), pIbtsView, SLOT(slotRecvConnectButtonState(bool)));

	//Logger connection to main gui
	QObject::connect(&LOGGER, SIGNAL(signalAddLogMsg(QString)), pIbtsView, SLOT(slotOnLogMsgReceived(QString)));

    // QObject::connect(workerAlfaTime, &AlphaModGetTime::signalPlanResetSubscribtion,
    //                  pDBStorePresenter.data(), &DBStorePresenter::signalResetSubscribtion, Qt::QueuedConnection);

    // QObject::connect(workerAlfaTime, &AlphaModGetTime::signalPlanResetSubscribtion,
    //                  pAutoDeltaAligPresenter->getPM().data(), &CProcessingBase::signalRestartSubscription, Qt::QueuedConnection);


    QTreeView * pTreeView = this->pIbtsView->getPortfolioConfigTreeView();
    CPortfolioConfigModel *pPConfigModel = this->getPGuiModel()->pPortfolioConfigModel();

    // Context menu driven by SystemTreeModel (replaces old index-based wiring)
    auto* sysModel = m_pSystemTreeModel;
    auto* root = this->getPGuiModel()->dataRoot();

    QObject::connect(pTreeView, &QTreeView::customContextMenuRequested,
                     this->pIbtsView, [pTreeView, sysModel, root, pPConfigModel, this](const QPoint& pos) {
        QModelIndex index = pTreeView->indexAt(pos);

        QMenu menu;

        // "Add Account" is always available (even on empty tree / right-click on blank space)
        QAction* addAccount = menu.addAction("Add New Account");

        QAction* addPortfolio = nullptr;
        QAction* addStrategy  = nullptr;
        QAction* removeNode   = nullptr;
        QAction* openBacktest = nullptr;
        QAction* addSelectionBlock  = nullptr;
        QAction* addAlphaBlock      = nullptr;
        QAction* addRebalanceBlock  = nullptr;
        QAction* addRiskBlock       = nullptr;
        QAction* addExecutionBlock  = nullptr;

        CGenericModelApi* clickedModel = nullptr;
        ModelType clickedType = ModelType::ROOT;

        if (index.isValid() && sysModel) {
            clickedModel = sysModel->modelAt(index);
            if (clickedModel)
                clickedType = clickedModel->modelType();

            if (clickedType == ModelType::ACCOUNT) {
                addPortfolio = menu.addAction("Add New Portfolio");
            }
            if (clickedType == ModelType::PORTFOLIO) {
                addStrategy = menu.addAction("Add Strategy");
            }

            bool isStrategy = (clickedType == ModelType::STRATEGY ||
                               clickedType == ModelType::STRATEGY_BASIC_TEST ||
                               clickedType == ModelType::STRATEGY_MA ||
                               clickedType == ModelType::STRATEGY_MOMENTUM ||
                               clickedType == ModelType::STRATEGY_PIPELINE);

            if (isStrategy) {
                openBacktest = menu.addAction("Open in Backtest Workspace");
            }

            bool isPipelineStrategy = (clickedType == ModelType::STRATEGY_PIPELINE);
            if (isPipelineStrategy) {
                menu.addSeparator();
                addSelectionBlock  = menu.addAction("Add Selection Model");
                addAlphaBlock      = menu.addAction("Add Alpha Model");
                addRebalanceBlock  = menu.addAction("Add Rebalance Model");
                addRiskBlock       = menu.addAction("Add Risk Model");
                addExecutionBlock  = menu.addAction("Add Execution Model");
            }

            if (clickedModel) {
                menu.addSeparator();
                removeNode = menu.addAction("Remove Selected Node");
            }
        }

        QAction* chosen = menu.exec(pTreeView->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == addAccount) {
            auto model = QSharedPointer<CBasicAccount>::create();
            model->setName("Account");
            model->setId(QUuid::createUuid());
            model->setParentActivationState(true);
            root->addModel(model);
            sysModel->rebuildFromRoot();
            pPConfigModel->setupModelData();
            pTreeView->expandAll();
        }
        else if (chosen == addPortfolio && clickedModel) {
            auto model = QSharedPointer<CBasicPortfolio>::create();
            model->setName("Portfolio");
            model->setId(QUuid::createUuid());
            model->setParentActivationState(clickedModel->getActiveStatus());
            model->setParentModel(clickedModel);
            clickedModel->addModel(model);
            sysModel->rebuildFromRoot();
            pPConfigModel->setupModelData();
            pTreeView->expandAll();
        }
        else if (chosen == addStrategy && clickedModel) {
            auto model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_PIPELINE);
            if (model) {
                model->setId(QUuid::createUuid());
                model->setParentActivationState(clickedModel->getActiveStatus());
                model->setParentModel(clickedModel);
                clickedModel->addModel(model);
                sysModel->rebuildFromRoot();
                pPConfigModel->setupModelData();
                pTreeView->expandAll();
            }
        }
        else if (chosen == openBacktest && clickedModel) {
            auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(clickedModel);
            if (adapter) {
                CGenericModelApi* parentModel = sysModel->parentModelAt(index);
                QString portfolioPath = parentModel ? parentModel->getName() : "";
                emit pPConfigModel->openInBacktestWorkspace(
                    clickedModel->getId().toString(QUuid::WithoutBraces),
                    clickedModel->getName(),
                    portfolioPath,
                    adapter->pipelineConfig());
            }
        }
        else if ((chosen == addSelectionBlock || chosen == addAlphaBlock ||
                  chosen == addRebalanceBlock || chosen == addRiskBlock ||
                  chosen == addExecutionBlock) && clickedModel) {
            auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(clickedModel);
            if (adapter) {
                QString category, jsonKey;
                bool isArray = false;

                if (chosen == addSelectionBlock)      { category = "Selection";  jsonKey = "selection";  isArray = false; }
                else if (chosen == addAlphaBlock)     { category = "Alpha";      jsonKey = "alphas";     isArray = true;  }
                else if (chosen == addRebalanceBlock) { category = "Rebalance";  jsonKey = "rebalance";  isArray = false; }
                else if (chosen == addRiskBlock)      { category = "Risk";       jsonKey = "risks";      isArray = true;  }
                else if (chosen == addExecutionBlock) { category = "Execution";  jsonKey = "execution";  isArray = false; }

                auto blocks = Pipeline::BlockRegistry::instance().blocksByCategory(category);
                if (!blocks.isEmpty()) {
                    QStringList displayNames, blockIds;
                    for (const auto& desc : blocks) {
                        QString label = desc.name;
                        if (!desc.description.isEmpty())
                            label += " -- " + desc.description;
                        displayNames.append(label);
                        blockIds.append(desc.id);
                    }

                    bool ok = false;
                    QString chosenBlock = QInputDialog::getItem(
                        pTreeView, QString("Select %1 Block").arg(category),
                        QString("Available %1 blocks:").arg(category.toLower()),
                        displayNames, 0, false, &ok);

                    if (ok && !chosenBlock.isEmpty()) {
                        int idx = displayNames.indexOf(chosenBlock);
                        if (idx >= 0) {
                            QString blockId = blockIds.at(idx);
                            auto descResult = Pipeline::BlockRegistry::instance().descriptor(blockId);
                            QJsonObject defaultCfg = descResult ? descResult.value().defaultConfig : QJsonObject();
                            QJsonObject config = adapter->pipelineConfig();

                            QJsonObject entry;
                            entry["blockId"] = blockId;
                            entry["config"] = defaultCfg;

                            if (isArray) {
                                QJsonArray arr = config.value(jsonKey).toArray();
                                arr.append(entry);
                                config[jsonKey] = arr;
                            } else {
                                config[jsonKey] = entry;
                            }

                            adapter->setPipelineConfig(config);
                            sysModel->rebuildFromRoot();
                            pTreeView->expandAll();
                            emit pPConfigModel->pipelineConfigChanged(config);
                        }
                    }
                }
            }
        }
        else if (chosen == removeNode && clickedModel) {
            CGenericModelApi* parentObj = clickedModel->getParentModel();
            if (parentObj) {
                auto& siblings = parentObj->getModels();
                for (int i = 0; i < siblings.size(); ++i) {
                    if (siblings[i].data() == clickedModel) {
                        parentObj->removeModel(siblings[i]);
                        break;
                    }
                }
                sysModel->rebuildFromRoot();
                pPConfigModel->setupModelData();
            }
        }
    });

    m_pDiagramWidget = new PipelineDiagramWidget();
    m_pDiagramDock = new QDockWidget("Diagram View", this->pIbtsView);
    m_pDiagramDock->setWidget(m_pDiagramWidget);
    m_pDiagramDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    this->pIbtsView->addDockWidget(Qt::BottomDockWidgetArea, m_pDiagramDock);

    // Backtest Workspace dock
    m_pBacktestDock = new BacktestUI::BacktestWorkspaceDock(this->pIbtsView);
    this->pIbtsView->addDockWidget(Qt::RightDockWidgetArea, m_pBacktestDock);
    m_pBacktestDock->hide(); // shown on first "Open in Backtest Workspace"

    // Open in Backtest Workspace: context menu → presenter → dock
    QObject::connect(pPConfigModel, &CPortfolioConfigModel::openInBacktestWorkspace,
                     this, &CPresenter::onOpenInBacktestWorkspace,
                     Qt::QueuedConnection);

    // Dock → presenter wiring
    QObject::connect(m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::runRequested,
                     this, [this](const Backtest::BacktestRunConfig& config) {
        if (!m_pBacktestController) return;
        m_pBacktestDock->setRunning(true);
        m_pBacktestController->start(config);
    });

    QObject::connect(m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::loadRunRequested,
                     this, &CPresenter::onLoadRun);

    // Reconnect selection changed to account for the new model that was set in setPGuiModel()
    if (pTreeView->selectionModel()) {
        QObject::connect(pTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
                         this, &CPresenter::onTreeSelectionChanged);
    }

    QObject::connect(pPConfigModel, &CPortfolioConfigModel::pipelineConfigChanged,
                     m_pDiagramWidget, &PipelineDiagramWidget::updateFromConfig);

    QObject::connect(pPConfigModel, SIGNAL(signalUpdateData(QModelIndex)), this->pIbtsView, SLOT(slotUpdateTreeView(QModelIndex)));
    QObject::connect(pPConfigModel, &CPortfolioConfigModel::signalUpdateDataAll, this->pIbtsView, &CIBTradeSystemView::slotUpdateTreeViewAll, Qt::QueuedConnection);


    this->pIbtsView->mapSignals();

    // Wire AlertService to GlobalStatusBar (D2: owned, not singleton)
    if (pGuiModel && pGuiModel->alertService() && pIbtsView->globalStatusBar()) {
        QObject::connect(pGuiModel->alertService(), &AlertService::countChanged,
                         pIbtsView->globalStatusBar(), &GlobalStatusBar::setAlertCount);
    }

    // Wire global actions from GlobalStatusBar
    if (pIbtsView->globalStatusBar()) {
        QObject::connect(pIbtsView->globalStatusBar(), &GlobalStatusBar::reconnectClicked,
                         this, &CPresenter::onClickMyButton);
    }



	/////////---------------
	//ReqManager temp;

	//temp.debugPrintList();
	//temp.addReqIds(1, RT_REQ_REL_DATA);
	//temp.debugPrintList();
	//temp.addReqIds(1, RT_REQ_REL_DATA);
	//temp.debugPrintList();
	//temp.addReqIds(1, RT_REQ_CUR_TIME);
	//temp.debugPrintList();
	//temp.addReqIds(2, RT_REQ_CUR_TIME);
	//temp.addReqIds(3, RT_REQ_CUR_TIME);
	//temp.debugPrintList();
	//temp.removeReqIds(2, RT_REQ_CUR_TIME);
	//temp.removeReqIds(2, RT_REQ_CUR_TIME);
	//temp.debugPrintList();
	//temp.isPresent(1, RT_REQ_REL_DATA);
	//temp.isPresent(1, RT_TICK_PRICE);
	//temp.isPresent(1, RT_MKT_DEPTH);
    // pPairTradingPresenter->init();
    // pAutoDeltaAligPresenter->init();
    // pDBStorePresenter->init();

}

void CPresenter::addView(CIBTradeSystemView * mw)
{
	this->pIbtsView = mw;

}



/*! 
*  Connect button processing
*/
void CPresenter::onClickMyButton()
{
    static bool buttonState = false;
    if ((false == buttonState) && (!m_pDataProvider->getClien()->isConnectedAPI()))
    {
        //Connect to the server
        workerIBClient->setCommand(IBWorker::CONNECT);
        //get time from server (1 sec period)
        workerAlfaTime->StartGetTimeUpdate(1000);

        //send signal to gui for change text of connect button
        emit signalClickConnect(true);
        buttonState = true;
    }
    else
    {
        //Stop getting time from server
        workerAlfaTime->StopTimeUpdate();
        //Disconnect from server
        workerIBClient->setCommand(IBWorker::DISCONNECT);

        //pAboutDlgPresenter->showDlg();

        //send signal to gui for change text of connect button
        emit signalClickConnect(false);

        buttonState = false;
    }
}


void CPresenter::onClickPairTraderButton()
{
    // if (!pPairTradingPresenter.isNull())
    // {
    // 	pPairTradingPresenter->showDlg();
    // }
}

void CPresenter::onClickAutoDeltaButton()
{
    // if (!pAutoDeltaAligPresenter.isNull())
    // {
    //     pAutoDeltaAligPresenter->showDlg();
    // }
}

void CPresenter::onClickDBStoreButton()
{
    // if (!pDBStorePresenter.isNull())
    // {
    //     pDBStorePresenter->showDlg();
    // }
}

QSharedPointer<CBrokerDataProvider> CPresenter::getDataProvider() const
{
    return m_pDataProvider;
}

CMainModel *CPresenter::getPGuiModel() const
{
    return pGuiModel;
}

void CPresenter::setPGuiModel(CMainModel *newPGuiModel)
{
   pGuiModel = newPGuiModel;
   this->pIbtsView->getSettingsTreeView()->setModel(this->pGuiModel->pSettingsModel());
   this->pIbtsView->getSettingsTreeView()->expandAll();

   this->getPGuiModel()->pPortfolioConfigModel()->setBrokerDataProvider(this->m_pDataProvider);
   this->getPGuiModel()->pPortfolioConfigModel()->setupModelData();

   // Set up SystemTreeModel (operations console tree replacing old config tree)
   m_pSystemTreeModel = new SystemTreeModel(this);
   m_pSystemTreeDelegate = new SystemTreeDelegate(this);
   m_pSystemTreeModel->setRoot(this->getPGuiModel()->dataRoot());

   QTreeView* treeView = this->pIbtsView->getPortfolioConfigTreeView();
   treeView->setModel(m_pSystemTreeModel);
   treeView->setItemDelegate(m_pSystemTreeDelegate);
   treeView->setHeaderHidden(false);
   treeView->setAlternatingRowColors(true);
   treeView->setColumnWidth(SystemTreeModel::ColName, 200);
   treeView->setColumnWidth(SystemTreeModel::ColEnabled, 50);
   treeView->setColumnWidth(SystemTreeModel::ColStatus, 110);
   treeView->setColumnWidth(SystemTreeModel::ColPnL, 90);
   treeView->header()->setStretchLastSection(false);
   treeView->header()->setSectionResizeMode(SystemTreeModel::ColName, QHeaderView::Stretch);
   treeView->expandAll();
}


CIBTradeSystemView *CPresenter::getPIbtsView() const
{
    return pIbtsView;
}

// ---------------------------------------------------------------------------
// Backtest workspace slots
// ---------------------------------------------------------------------------

void CPresenter::onOpenInBacktestWorkspace(const QString& strategyId,
                                            const QString& displayName,
                                            const QString& portfolioPath,
                                            const QJsonObject& pipelineConfig)
{
    if (!m_pBacktestDock) return;

    // Re-create the controller for this strategy (one controller per active run).
    // Any previous run is abandoned (no cooperative cancel in v1 — the old thread
    // will finish on its own and its signals will be ignored after reconnect).
    delete m_pBacktestController;
    m_pBacktestController = new Backtest::BacktestController(
        QStringLiteral("myLocalDb.sqlite"), nullptr, this);

    connect(m_pBacktestController, &Backtest::BacktestController::progressChanged,
            m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::setProgress);
    connect(m_pBacktestController, &Backtest::BacktestController::statusChanged,
            m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::setStatus);
    connect(m_pBacktestController, &Backtest::BacktestController::finished,
            this, &CPresenter::onBacktestFinished);
    connect(m_pBacktestController, &Backtest::BacktestController::failed,
            this, &CPresenter::onBacktestFailed);

    // Determine profile from the pipeline config JSON
    Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            pipelineConfig.value("backtestProfile").toObject());

    const QString pipelineConfigJson = QString::fromUtf8(
        QJsonDocument(pipelineConfig).toJson(QJsonDocument::Compact));

    m_pBacktestDock->selectStrategy(strategyId, displayName, portfolioPath,
                                     profile, pipelineConfigJson);

    // Float the dock at a comfortable size so charts are immediately visible
    if (!m_pBacktestDock->isFloating()) {
        m_pBacktestDock->setFloating(true);
        m_pBacktestDock->resize(900, 650);
    }
    m_pBacktestDock->show();
    m_pBacktestDock->raise();

    // Fetch run history for this strategy from DB
    if (m_pBacktestController) {
        const QString conn = m_pBacktestController->dbConnectionName();
        auto q = query_fetchRunsForStrategy(strategyId, conn);
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
            m_pBacktestDock->setRunHistory(summaries);
        }
    }
}

void CPresenter::onLoadRun(const QString& runId) {
    if (!m_pBacktestDock || !m_pBacktestController) return;

    const QString conn = m_pBacktestController->dbConnectionName();
    QSqlDatabase db = QSqlDatabase::database(conn);
    if (!db.isOpen()) return;

    Backtest::BacktestLoadedRun loaded;

    // Run record
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

    // Metrics → result fields
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

    // Equity curve → LedgerSnapshot vector
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

    // Trade log → FilledOrder vector
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

    m_pBacktestDock->displayResult(loaded);
}

void CPresenter::onBacktestFinished(const Backtest::BacktestLoadedRun& run) {
    if (!m_pBacktestDock) return;
    m_pBacktestDock->setRunning(false);
    m_pBacktestDock->displayResult(run);

    // Refresh run history
    const QString conn = m_pBacktestController ? m_pBacktestController->dbConnectionName()
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
        m_pBacktestDock->setRunHistory(summaries);
    }
}

void CPresenter::onBacktestFailed(const QString& reason) {
    if (!m_pBacktestDock) return;
    m_pBacktestDock->setRunning(false);
    m_pBacktestDock->setStatus(QStringLiteral("Failed: ") + reason);
}

// ---------------------------------------------------------------------------
// onTreeSelectionChanged (existing, unchanged)
// ---------------------------------------------------------------------------

void CPresenter::onTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    if (!current.isValid()) return;

    if (!m_pSystemTreeModel) {
        if (m_pDiagramWidget) m_pDiagramWidget->clear();
        return;
    }

    // Handle virtual block nodes first
    if (m_pSystemTreeModel->isVirtualBlock(current)) {
        CGenericModelApi* parentStrategy = m_pSystemTreeModel->parentStrategyOf(current);
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(parentStrategy);

        if (adapter && m_pDiagramWidget)
            m_pDiagramWidget->setPipelineConfig(adapter->pipelineConfig());

        if (adapter && pIbtsView->contextWorkspace()) {
            QString blockId  = m_pSystemTreeModel->virtualBlockId(current);
            QString category = m_pSystemTreeModel->virtualCategory(current);

            QString jsonKey;
            bool isArray = false;
            if (category == "Selection")       { jsonKey = "selection";  isArray = true;  }
            else if (category == "Alpha")      { jsonKey = "alphas";     isArray = true;  }
            else if (category == "Risk")       { jsonKey = "risks";      isArray = true;  }
            else if (category == "Rebalance")  { jsonKey = "rebalance";  isArray = false; }
            else if (category == "Execution")  { jsonKey = "execution";  isArray = false; }

            int arrayIndex = -1;
            if (isArray) {
                QJsonArray arr = adapter->pipelineConfig().value(jsonKey).toArray();
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    QString id = obj.value("blockId").toString();
                    if (id.isEmpty()) id = obj.value("type").toString();
                    if (id == blockId) { arrayIndex = i; break; }
                }
            }

            pIbtsView->contextWorkspace()->showBlockWorkspace(
                adapter, category, blockId, jsonKey, arrayIndex);
        }
        return;
    }

    CGenericModelApi* model = m_pSystemTreeModel->modelAt(current);
    if (!model) {
        if (m_pDiagramWidget) m_pDiagramWidget->clear();
        if (pIbtsView->contextWorkspace()) pIbtsView->contextWorkspace()->showEmpty();
        return;
    }

    ModelType mt = model->modelType();

    if (m_pDiagramWidget) {
        if (mt == ModelType::ACCOUNT) {
            QStringList portfolioNames;
            for (const auto& p : model->getModels())
                portfolioNames.append(p->getName());
            m_pDiagramWidget->setAccountView(model->getName(), portfolioNames);
        }
        else if (mt == ModelType::PORTFOLIO) {
            QVector<StrategyDiagramInfo> strategies;
            for (const auto& s : model->getModels()) {
                StrategyDiagramInfo info;
                info.name = s->getName();
                auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(s.data());
                if (adapter) info.pipelineConfig = adapter->pipelineConfig();
                strategies.append(info);
            }
            m_pDiagramWidget->setPortfolioView(model->getName(), strategies);
        }
        else {
            auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(model);
            if (adapter)
                m_pDiagramWidget->setPipelineConfig(adapter->pipelineConfig());
            else
                m_pDiagramWidget->clear();
        }
    }

    if (pIbtsView->contextWorkspace()) {
        bool isStrategy = (mt == ModelType::STRATEGY ||
                           mt == ModelType::STRATEGY_BASIC_TEST ||
                           mt == ModelType::STRATEGY_MA ||
                           mt == ModelType::STRATEGY_MOMENTUM ||
                           mt == ModelType::STRATEGY_PIPELINE);
        if (mt == ModelType::ACCOUNT) {
            pIbtsView->contextWorkspace()->showAccountWorkspace(model);
        } else if (mt == ModelType::PORTFOLIO) {
            CGenericModelApi* parentModel = m_pSystemTreeModel->parentModelAt(current);
            pIbtsView->contextWorkspace()->showPortfolioWorkspace(model, parentModel);
        } else if (isStrategy) {
            CGenericModelApi* parentModel = m_pSystemTreeModel->parentModelAt(current);
            pIbtsView->contextWorkspace()->showStrategyWorkspace(model, parentModel);
        } else {
            pIbtsView->contextWorkspace()->showEmpty();
        }
    }
}

