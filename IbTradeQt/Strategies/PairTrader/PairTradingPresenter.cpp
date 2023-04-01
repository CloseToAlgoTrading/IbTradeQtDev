#include "PairTradingPresenter.h"
#include "ctickprice.h"
#include <QMetaType>


PairTradingPresenter::PairTradingPresenter(QObject *parent, CBrokerDataProvider * _refClient)
	: QObject(parent)
	, m_pPairTraderGui(new PairTradingGui())
    , m_pPairTraderPM(new PairTraderPM(parent, _refClient))
    , m_threadPairTaderPM(new QThread)
{


 
}

PairTradingPresenter::~PairTradingPresenter()
{

}

void PairTradingPresenter::init()
{
    m_pPairTraderPM->moveToThread(m_threadPairTaderPM);
    m_threadPairTaderPM->start();
    m_threadPairTaderPM->setObjectName("PM_Thread");


    QObject::connect(m_pPairTraderGui.data(), &PairTradingGui::signalAddNewPair, m_pPairTraderPM.data(), &PairTraderPM::slotOnAddNewPair, Qt::QueuedConnection);

    QObject::connect(m_pPairTraderGui.data(), &PairTradingGui::signalRemoveSimbol, m_pPairTraderPM.data(), &PairTraderPM::slotOnRemoveSymbol, Qt::QueuedConnection);
    QObject::connect(m_pPairTraderGui.data(), &PairTradingGui::signalGUIClosed, m_pPairTraderPM.data(), &PairTraderPM::slotOnGUIClosed, Qt::QueuedConnection);

    QObject::connect(m_pPairTraderPM.data(), &PairTraderPM::signalOnRealTimeTickData, m_pPairTraderGui.data(), &PairTradingGui::slotOnRealTimeTickData, Qt::QueuedConnection);



    //GUI Request to PM for getting history
    QObject::connect(m_pPairTraderGui.data(), &PairTradingGui::signalRequestHistory, m_pPairTraderPM.data(), &PairTraderPM::slotOnRequestHistory, Qt::QueuedConnection);

    QObject::connect(m_pPairTraderPM.data(), &PairTraderPM::signalOnFinishHistoricalData, m_pPairTraderGui.data(), &PairTradingGui::slotOnFinishHistoricalData, Qt::QueuedConnection);


    //GUI Test Request
    QObject::connect(m_pPairTraderGui.data(), &PairTradingGui::signalRequestSendBuyMkt, m_pPairTraderPM.data(), &PairTraderPM::slotOnRequestSendBuyMkt, Qt::QueuedConnection);
    QObject::connect(m_pPairTraderGui.data(), &PairTradingGui::signalRequestSendSellMkt, m_pPairTraderPM.data(), &PairTraderPM::slotOnRequestSendSellMkt, Qt::QueuedConnection);

}

void PairTradingPresenter::showDlg()
{
	m_pPairTraderGui->show();
}
