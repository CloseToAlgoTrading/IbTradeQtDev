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
    QObject::connect(pTreeView->actions().at(0), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddAccount()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(1), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddPortfolio()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(2), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddStrategy()), Qt::QueuedConnection);

    QObject::connect(pTreeView->actions().at(4), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddSelectionModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(5), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddAlphaModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(6), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddRebalanceModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(7), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddRiskModel()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions().at(8), SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddExecutionModel()), Qt::QueuedConnection);

    QObject::connect(pTreeView->actions().at(10), SIGNAL(triggered()), pPConfigModel, SLOT(onClickRemoveNodeButton()), Qt::QueuedConnection);

    pTreeView->setItemDelegateForColumn(1, new PipelineItemDelegate(pTreeView));

    m_pDiagramWidget = new PipelineDiagramWidget();
    m_pDiagramDock = new QDockWidget("Diagram View", this->pIbtsView);
    m_pDiagramDock->setWidget(m_pDiagramWidget);
    m_pDiagramDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    this->pIbtsView->addDockWidget(Qt::BottomDockWidgetArea, m_pDiagramDock);

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
            if (adapter) {
                info.pipelineConfig = adapter->pipelineConfig();
                int count = 0;
                const auto& cfg = info.pipelineConfig;
                if (cfg.contains("selection")) ++count;
                count += cfg.value("alphas").toArray().size();
                if (cfg.contains("rebalance")) ++count;
                count += cfg.value("risks").toArray().size();
                if (cfg.contains("execution")) ++count;
                info.blockCount = count;
            }
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

