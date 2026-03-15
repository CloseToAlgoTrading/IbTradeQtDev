#include "cpresenter.h"
#include "ReqManager.h"
#include "IBComClientImpl.h"
#include "cmainmodel.h"
#include "CPortfolioConfigModel.h"
#include "PipelineItemDelegate.h"
#include "PipelineDiagramWidget.h"
#include "cpipelinestrategyadapter.h"
#include "PortfolioModelDefines.h"
#include <QDockWidget>
#include <QMenu>
#include <QPoint>
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
    // Named action connections (indices match the order in ibtradesystemview.cpp)
    QObject::connect(pTreeView->actions().at(0),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddAccount()),        Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(1),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddPortfolio()),       Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(2),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddStrategy()),        Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(4),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddSelectionModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(5),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddAlphaModel()),     Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(6),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddRebalanceModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(7),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddRiskModel()),      Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(8),  SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddExecutionModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(10), SIGNAL(triggered()), pPConfigModel, SLOT(onClickRemoveNodeButton()),      Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(12), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickOpenInBacktestWorkspace()), Qt::QueuedConnection);

    // Custom context menu: show/hide actions depending on the selected node type
    QObject::connect(pTreeView, &QTreeView::customContextMenuRequested,
                     this->pIbtsView, [pTreeView, pPConfigModel](const QPoint& pos) {
        QModelIndex index = pTreeView->indexAt(pos);
        if (!index.isValid()) return;

        quint16 nodeType = pPConfigModel->nodeTypeId(index);
        const bool isPipelineStrategy = (nodeType == PM_ITEM_PIPELINE_STRATEGY ||
                                         nodeType == PM_ITEM_STRATEGY);

        // Show/hide the "Open in Backtest Workspace" action (index 12)
        auto actions = pTreeView->actions();
        if (actions.size() > 12) {
            actions.at(11)->setVisible(isPipelineStrategy); // separator before backtest
            actions.at(12)->setVisible(isPipelineStrategy);
        }

        QMenu menu;
        for (QAction* a : pTreeView->actions()) {
            if (a->isVisible()) menu.addAction(a);
        }
        menu.exec(pTreeView->viewport()->mapToGlobal(pos));
    });

    pTreeView->setItemDelegateForColumn(1, new PipelineItemDelegate(pTreeView));

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

    QObject::connect(pTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
                     this, &CPresenter::onTreeSelectionChanged);

    QObject::connect(pPConfigModel, &CPortfolioConfigModel::pipelineConfigChanged,
                     m_pDiagramWidget, &PipelineDiagramWidget::updateFromConfig);

    QObject::connect(pPConfigModel, SIGNAL(signalUpdateData(QModelIndex)), this->pIbtsView, SLOT(slotUpdateTreeView(QModelIndex)));
    QObject::connect(pPConfigModel, &CPortfolioConfigModel::signalUpdateDataAll, this->pIbtsView, &CIBTradeSystemView::slotUpdateTreeViewAll, Qt::QueuedConnection);


    this->pIbtsView->mapSignals();



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

   this->pIbtsView->getPortfolioConfigTreeView()->setModel(this->pGuiModel->pPortfolioConfigModel());
   //this->pIbtsView->getPortfolioConfigTreeView()->expandAll();

   this->getPGuiModel()->pPortfolioConfigModel()->setBrokerDataProvider(this->m_pDataProvider);
   this->getPGuiModel()->pPortfolioConfigModel()->setupModelData();
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
    if (!m_pDiagramWidget || !current.isValid()) return;

    auto* configModel = getPGuiModel()->pPortfolioConfigModel();
    quint16 nodeType = configModel->nodeTypeId(current);

    if (nodeType == PM_ITEM_ACCOUNT) {
        auto accountModel = configModel->getTopLevelModelByIdex2(current).model;
        if (!accountModel) { m_pDiagramWidget->clear(); return; }

        QStringList portfolioNames;
        for (const auto& p : accountModel->getModels())
            portfolioNames.append(p->getName());

        m_pDiagramWidget->setAccountView(accountModel->getName(), portfolioNames);
        return;
    }

    if (nodeType == PM_ITEM_PORTFOLIO) {
        auto portfolioModel = configModel->getTopLevelModelByIdex2(current).model;
        if (!portfolioModel) { m_pDiagramWidget->clear(); return; }

        QVector<StrategyDiagramInfo> strategies;
        for (const auto& s : portfolioModel->getModels()) {
            StrategyDiagramInfo info;
            info.name = s->getName();
            auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(s.data());
            if (adapter)
                info.pipelineConfig = adapter->pipelineConfig();
            strategies.append(info);
        }

        m_pDiagramWidget->setPortfolioView(portfolioModel->getName(), strategies);
        return;
    }

    if (nodeType == PM_ITEM_STRATEGY || nodeType == PM_ITEM_PIPELINE_STRATEGY ||
        nodeType == PM_ITEM_SELECTION_MODEL || nodeType == PM_ITEM_ALFA_MODEL ||
        nodeType == PM_ITEM_REBALANCE_MODEL || nodeType == PM_ITEM_RISK_MODEL ||
        nodeType == PM_ITEM_EXECUTION_MODEL) {
        auto model = configModel->getTopLevelModelByIdex2(current).model;
        if (!model) { m_pDiagramWidget->clear(); return; }

        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(model.data());
        if (adapter) {
            m_pDiagramWidget->setPipelineConfig(adapter->pipelineConfig());
            return;
        }
    }

    m_pDiagramWidget->clear();
}

