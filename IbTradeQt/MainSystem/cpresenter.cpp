#include "cpresenter.h"
#include "ReqManager.h"
#include "Dispatcher.h"
#include "IBComClientImpl.h"
#include "cmainmodel.h"


CPresenter::CPresenter(QObject *parent)
	: QObject(parent)
    , m_pLog(LOGGER)
    , m_DataProvider()
    , pIbtsView(nullptr)
    , pGuiModel(nullptr)
    , threadIBClient(new QThread)
    , workerIBClient(new IBWorker::Worker(parent, m_DataProvider))
    , threadAlfaTime(new QThread)
    , workerAlfaTime(new AlphaModGetTime(parent, m_DataProvider))
    , pAboutDlgPresenter(new AboutDlgPresener(parent))
    , pPairTradingPresenter(new PairTradingPresenter(parent, m_DataProvider))
    , pAutoDeltaAligPresenter(new AutoDeltaAligPresenter(parent, m_DataProvider))
    , pDBStorePresenter(new DBStorePresenter(parent, m_DataProvider))
{
	
    //

    QSharedPointer<IBComClientImpl> pClient = QSharedPointer<IBComClientImpl>(new IBComClientImpl(m_DataProvider));

   //Define Data Provider
    m_DataProvider.setClien(QSharedPointer<IBComClientImpl>::create(m_DataProvider));

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
	QObject::connect(pIbtsView->getUi().pushButton, SIGNAL(clicked()), this, SLOT(onClickMyButton()));
	//click Pair Trader button
	QObject::connect(pIbtsView->getUi().pushButtonPairTrader, SIGNAL(clicked()), this, SLOT(onClickPairTraderButton()));
    //click Auto Delta button
    QObject::connect(pIbtsView->getUi().autoDeltaButton, SIGNAL(clicked()), this, SLOT(onClickAutoDeltaButton()));
    //click DBStore button
    QObject::connect(pIbtsView->getUi().pushButtonDBStore, SIGNAL(clicked()), this, SLOT(onClickDBStoreButton()));


	//Click received Time button
	QObject::connect(workerAlfaTime, SIGNAL(signalTimeReceived(long)), pIbtsView, SLOT(slotOnTimeReceived(long)));
	//Received info about connection button state
	QObject::connect(this, SIGNAL(signalClickConnect(bool)), pIbtsView, SLOT(slotRecvConnectButtonState(bool)));

	//Logger connection to main gui
	QObject::connect(&LOGGER, SIGNAL(signalAddLogMsg(QString)), pIbtsView, SLOT(slotOnLogMsgReceived(QString)));

    QObject::connect(workerAlfaTime, &AlphaModGetTime::signalPlanResetSubscribtion,
                     pDBStorePresenter.data(), &DBStorePresenter::signalResetSubscribtion, Qt::QueuedConnection);

    QObject::connect(workerAlfaTime, &AlphaModGetTime::signalPlanResetSubscribtion,
                     pAutoDeltaAligPresenter->getPM().data(), &CProcessingBase::signalRestartSubscription, Qt::QueuedConnection);


    QTreeView * pTreeView = this->pIbtsView->getPortfolioConfigTreeView();
    CPortfolioConfigModel *pPConfigModel = this->getPGuiModel()->pPortfolioConfigModel();
    QObject::connect(pTreeView->actions()[0], SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddAccount()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions()[1], SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddPortfolio()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions()[2], SIGNAL(triggered()), pPConfigModel, SLOT(slotOnClickAddStrategy()), Qt::QueuedConnection);
    QObject::connect(pTreeView->actions()[4], SIGNAL(triggered()), pPConfigModel, SLOT(onClickRemoveNodeButton()), Qt::QueuedConnection);

    QObject::connect(pPConfigModel, SIGNAL(signalUpdateData(QModelIndex)), this->pIbtsView, SLOT(slotUpdateTreeView(QModelIndex)));

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
    pPairTradingPresenter->init();
    pAutoDeltaAligPresenter->init();
    pDBStorePresenter->init();

}

void CPresenter::UnsubscribeHandler()
{

}

void CPresenter::addView(CIBTradeSystemView * mw)
{
	this->pIbtsView = mw;

}



void CPresenter::MessageHandler(void* pContext, tEReqType _reqType)
{
    Q_UNUSED(pContext);
    Q_UNUSED(_reqType);
};

/*! 
*  Connect button processing
*/
void CPresenter::onClickMyButton()
{
    static bool buttonState = false;
    if ((false == buttonState) && (!m_DataProvider.getClien()->isConnectedAPI()))
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
	if (!pPairTradingPresenter.isNull())
	{
		pPairTradingPresenter->showDlg();
	}
}

void CPresenter::onClickAutoDeltaButton()
{
    if (!pAutoDeltaAligPresenter.isNull())
    {
        pAutoDeltaAligPresenter->showDlg();
    }
}

void CPresenter::onClickDBStoreButton()
{
    if (!pDBStorePresenter.isNull())
    {
        pDBStorePresenter->showDlg();
    }
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
   this->pIbtsView->getPortfolioConfigTreeView()->expandAll();


}


CIBTradeSystemView *CPresenter::getPIbtsView() const
{
    return pIbtsView;
}




