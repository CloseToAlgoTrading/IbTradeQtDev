﻿#include "cpresenter.h"
#include "ReqManager.h"

#include "Dispatcher.h"

#include "IBComClientImpl.h"

CPresenter::CPresenter(QObject *parent)
	: QObject(parent)
    , m_pLog(LOGGER)
    , m_DataProvider()
    , threadIBClient(new QThread)
    , workerIBClient(new IBWorker::Worker(parent, m_DataProvider))
    , threadAlfaTime(new QThread)
    , workerAlfaTime(new AlphaModGetTime(parent, m_DataProvider))
    , pAboutDlgPresenter(new AboutDlgPresener(parent))
    , pPairTradingPresenter(new PairTradingPresenter(parent, m_DataProvider))
    , pAutoDeltaAligPresenter(new AutoDeltaAligPresenter(parent, m_DataProvider))
    , pDBStorePresenter(new DBStorePresenter(parent, m_DataProvider))
{
	
    QSharedPointer<IBComClientImpl> pClient = QSharedPointer<IBComClientImpl>(new IBComClientImpl(m_DataProvider));

   //Define Data Provider
    m_DataProvider.setClien(pClient);


	workerIBClient->moveToThread(threadIBClient);
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
	//MAP Thread signal
	QObject::connect(threadIBClient, SIGNAL(started()), workerIBClient, SLOT(process()));

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

void CPresenter::addView(IBTradeSystem * mw)
{
	this->pIbtsView = mw;
}



void CPresenter::MessageHandler(void* pContext, tEReqType _reqType)
{

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




