#include "DBStoreProcessing.h"
#include "./IBComm/Dispatcher.h"
#include <QSharedPointer>

#include <QThread>

#include "DBConnector.h"



Q_LOGGING_CATEGORY(DBStorePMLog, "DBStore.PM");

using namespace Observer;

//----------------------------------------------------------
DBStorePM::DBStorePM(QObject *parent, CBrokerDataProvider & _refClient)
    : CProcessingBase(parent, _refClient)
    , Ticker_id(0)
    , m_dbc(parent)
    , m_threadDBConnector(new QThread)
    , m_data()
{
    m_dbc.moveToThread(m_threadDBConnector);
    m_threadDBConnector->start();
    m_threadDBConnector->setObjectName("DBConnecor_Thread");

   // qRegisterMetaType<QVector<QString>>();
    QObject::connect(this, &DBStorePM::signalInsertNewRealTimeBarItem, &m_dbc, &DBConnector::slotInsertNewRealTimeBarItem, Qt::QueuedConnection);
    QObject::connect(this, &DBStorePM::signalInsertNewTickByTickLastItem, &m_dbc, &DBConnector::slotInsertTickByTickLastItem, Qt::QueuedConnection);
    QObject::connect(this, &DBStorePM::signalRestartSubscription, this, &DBStorePM::slotRestartSubscription, Qt::QueuedConnection);

}

//----------------------------------------------------------
DBStorePM::~DBStorePM() = default;

//----------------------------------------------------------
//************************************
// Method:    initStrategy
// FullName:  PairTraderPM::initStrategy
// Access:    public
// Returns:   void
// Qualifier:
//
// make your strategy initialization and configuration
//************************************
void DBStorePM::initStrategy(const QString & s1)
{
    Q_UNUSED(s1);
}

void DBStorePM::sendRequestData(const DBStoreTickerArrayType &_data)
{
    Contract contract;
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";

    qCInfo(DBStorePMLog(), "_data.size = %d", _data.size());
    for (const auto &_Ticker: qAsConst(_data))
    {
        qCInfo(DBStorePMLog(), "%s request", _Ticker.toLocal8Bit().data());

        contract.symbol = _Ticker.toLocal8Bit().data();
        reqTickByTickDataConfigData_t tickbytickConfig{0, contract, "AllLast", 10, false};
        if(false == requestTickByTickData(tickbytickConfig))
        {
            qCWarning(DBStorePMLog(), "%s Not Possible to request", _Ticker.toLocal8Bit().data());
        }

    }
}

//----------------------------------------------------------
void DBStorePM::slotStartDBStore(const DBStoreTickerArrayType &_data)
{
    m_data = _data;


    if(true == isConnectedTotheServer())
    {
        reqestResetSubscription();

        m_dbc.disconnectDB();
        m_dbc.connectDB();

//    requestRealTimeBars(QString("SPY"));
//    requestRealTimeBars(QString("NVDA"));
//    requestRealTimeBars(QString("QQQ"));

// Request historical data !
//    Contract contract;
//    contract.symbol = "SPY";
//    contract.secType = "STK";
//    contract.currency = "USD";
//    contract.exchange = "SMART";
//    reqHistTicksConfigData_t histConfiguration{0, "SPY", contract,
//                                               "20170701 12:01:00", "",
//                                               1000, "TRADES", 1, true };
//    requestHistoricalTicksData(histConfiguration);
//---------------------------------

        sendRequestData(_data);
    }
    else {
        qCWarning(DBStorePMLog(), "No connection to the server!");
    }
}

void DBStorePM::slotStopDBStore()
{
    m_dbc.disconnectDB();
    cancelAllActiveRequests();
    //UnsubscribeHandler();
}


//----------------------------------------------------------
void DBStorePM::slotOnRemoveSymbol(const QString & s1)
{
    Q_UNUSED(s1);
   // cancelRealTimeData(s1);
}


//----------------------------------------------------------
void DBStorePM::slotOnGUIClosed()
{
    cancelAllActiveRequests();
    UnsubscribeHandler();
}

void DBStorePM::slotRestartSubscription()
{
    qCInfo(DBStorePMLog(),"slot RECV RESTART SUBSCRIPTION");
    bool isConnected = isConnectedTotheServer();
    if((true == isConnected) && (0 != getRequestMapSize()))
    {
        qInfo(DBStorePMLog(), "Restart");
        QThread::sleep(10);
        cancelAllActiveRequests();
        //UnsubscribeHandler();

        QThread::sleep(10);

        reqestResetSubscription();
        sendRequestData(m_data);
    }
    else {
        qWarning(DBStorePMLog(), "No restart: severConnected = %d, requestsize = %d", isConnected, getRequestMapSize());
    }
}


void DBStorePM::recvRealtimeBar(void *pContext, tEReqType _reqType)
{
    auto *pRealTimeBar = static_cast<CrealtimeBar *>(pContext);
    CrealtimeBar rtb = *pRealTimeBar;
    QString retSymbol = getSymbolFromRM(pRealTimeBar->getId(), _reqType);

    emit signalInsertNewRealTimeBarItem(*pRealTimeBar, retSymbol);
    //qDebug() << "D--" << QThread::currentThreadId();

}

void DBStorePM::recvTickByTickAllLastData(void *pContext, tEReqType _reqType)
{
    auto *pTickByTickLast = static_cast<CTickByTickAllLast *>(pContext);
    CTickByTickAllLast TickByTickLast = *pTickByTickLast;
    QString retSymbol = getSymbolFromRM(pTickByTickLast->getId(), _reqType);

    signalInsertNewTickByTickLastItem(TickByTickLast, retSymbol);
}

void DBStorePM::recvRestartSubscription()
{
    emit signalRestartSubscription();
    qCInfo(DBStorePMLog()," emit signal RECV RESTART SUBSCRIPTION");
}


