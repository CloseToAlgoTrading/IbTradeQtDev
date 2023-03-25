#include "AutoDeltAlignmentProcessing.h"
#include "./IBComm/Dispatcher.h"
#include <QSharedPointer>
#include <QtGlobal>
#include <QtMath>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>
#include "contractsdefs.h"
#include <QTime>
#include <QNetworkReply>

Q_LOGGING_CATEGORY(autoDeltaAligPMLog, "AutoDeltaAlig.PM");

using namespace Observer;
using namespace IBDataTypes;
//----------------------------------------------------------
autoDeltaAligPM::autoDeltaAligPM(QObject *parent, CBrokerDataProvider & _refClient)
    : CProcessingBase(parent, _refClient)
    , CBasicStrategy()
    , Ticker_id(0)
    , m_delta()
    , m_TicketName()
    , m_commSum(0)
    , m_rpnlSum(0)
    , m_deltaThresold(0)
    , m_IsOrderBusy(false)
    , m_GuiInfo()
//    , m_pTimer(new QTimer(this))
    , m_modelInput(3u) //Model needs 3 days, but we will fill with 4, because we need 4 day for correct preprocessing
    //, m_networkManager()
    , startWorkingTime(START_H,START_M,START_S)
    , endWorkingTime(END_H,END_M,END_S)
    , m_sem(1)
    , request(QNetworkRequest(QUrl("http://localhost:49154/predict")))
{

      this->request.setRawHeader("Content-Type","application/json");

      QObject::connect(this, &CProcessingBase::signalRecvOptionTickComputation, this, &autoDeltaAligPM::slotRecvOptionTickComputation, Qt::QueuedConnection);
      QObject::connect(this, &CProcessingBase::signalEndRecvPosition, this, &autoDeltaAligPM::slotEndRecvPosition, Qt::QueuedConnection);

      QObject::connect(this, &CProcessingBase::signalRecvOrderCommission, this, &autoDeltaAligPM::slotOrderCommission, Qt::QueuedConnection);

      QObject::connect(this, &CProcessingBase::signalRestartSubscription, this, &autoDeltaAligPM::slotRestartSubscription, Qt::QueuedConnection);

//      QObject::connect(m_pTimer.data(), &QTimer::timeout, this, &autoDeltaAligPM::slotTimerTriggered, Qt::QueuedConnection);

      QObject::connect(&m_networkManager, &QNetworkAccessManager::finished, this, &autoDeltaAligPM::onManagerFinished);

      QObject::connect(this, &autoDeltaAligPM::signalPostRequest, this, &autoDeltaAligPM::slotPostRequest);



}

autoDeltaAligPM::autoDeltaAligPM(QObject *parent):
      CProcessingBase(parent, m_DataProvider)
    , CBasicStrategy()
    , Ticker_id(0)
    , m_delta()
    , m_TicketName()
    , m_commSum(0)
    , m_rpnlSum(0)
    , m_deltaThresold(0)
    , m_IsOrderBusy(false)
    , m_GuiInfo()
//    , m_pTimer(new QTimer(this))
    , m_modelInput(3u) //Model needs 3 days, but we will fill with 4, because we need 4 day for correct preprocessing
    //, m_networkManager()
    , startWorkingTime(START_H,START_M,START_S)
    , endWorkingTime(END_H,END_M,END_S)
    , m_sem(1)
    , request(QNetworkRequest(QUrl("http://localhost:49154/predict")))
{
    this->m_InfoMap["name"] = "autoDeltaAligPM";
    this->m_ParametersMap["Rebalance delta"] = 10;
    this->m_ParametersMap["StopLoss"] = 15;

}


//----------------------------------------------------------
autoDeltaAligPM::~autoDeltaAligPM() = default;

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
void autoDeltaAligPM::initStrategy(const QString & s1)
{
    Q_UNUSED(s1);
    m_delta.resetDelta();
    m_commSum = 0;
    m_rpnlSum = 0;
    m_IsOrderBusy = false;
    m_TicketName = s1;
}

bool autoDeltaAligPM::start()
{
    CBasicStrategy::start();
    return true;
}

bool autoDeltaAligPM::stop()
{
    CBasicStrategy::stop();
    return true;
}

//----------------------------------------------------------
void autoDeltaAligPM::startStrategy(const tAutoDeltaOptDataType &_opt, const qint32 &_delta)
{
    if(true == isConnectedTotheServer())
    {
        reqestResetSubscription();
        initStrategy(_opt._optName);

        m_deltaThresold = _delta;

        m_GuiInfo = _opt;

        Contract optContract = ContractsDefs::USOptionContract(m_TicketName, _opt._expDate, _opt._target, _opt.isCall);
        reqReadlTimeDataConfigData_t optCfg{0, optContract, "", false, false};
        reqestRealTimeData(optCfg);


        requestRealTimeBars(m_TicketName);

        m_delta.resetDelta();
        requestPosition();


        //DBConnector dbc(this);

        //dbc.connectDB();

        //CModelInputData test_data(3);
/*
        this->m_modelInput.addObservation({1.0,0.0}, {110.11,0.12,0.11,0.23,0.444});
        this->m_modelInput.addObservation({0.0,1.0}, {100.11,1.12,1.11,1.23,1.444});

        QNetworkRequest request = QNetworkRequest(QUrl("http://localhost:49153/predict"));
        request.setRawHeader("Content-Type","application/json");

        QByteArray ba = this->m_modelInput.toJson();
        this->m_networkManager.post(request, ba);
*/
        this->m_networkManager.post(this->request, this->m_modelInput.toJson());
        reqestOrderStatusSubscription();
    }
    else {
        qCWarning(autoDeltaAligPMLog(), "No connection to the server!");
    }
}

//----------------------------------------------------------
void autoDeltaAligPM::slotStartNewDeltaHedge(const tAutoDeltaOptDataType &_opt, const qint32 & _delta)
{
    m_GuiInfo = _opt;
    m_deltaThresold = _delta;

    startStrategy(m_GuiInfo, m_deltaThresold);
}


//----------------------------------------------------------
void autoDeltaAligPM::slotOnStopPressed()
{
   cancelRealTimeData("NVDA");
   cancelOrderStatusSubscription();
}


//----------------------------------------------------------
void autoDeltaAligPM::slotOnGUIClosed()
{
    cancelAllActiveRequests();
    UnsubscribeHandler();
}

void autoDeltaAligPM::slotMessageHandler(void *pContext, tEReqType _reqType)
{
    Q_UNUSED(pContext);
    Q_UNUSED(_reqType);
   // QThread::sleep(5);
}

void autoDeltaAligPM::slotRecvOptionTickComputation(const COptionTickComputation &obj)
{

//    qCDebug(autoDeltaAligPMLog,"slotRecvOptionTickComputation : tickerId = %d , tickType = %d, impliedVol = %f, "
//                               "delta = %f, optPrice = %f, pvDividend = %f, gamma = %f, vega = %f, theta = %f, undPrice = %f",
//        obj.get_tickerId(), obj.get_field(), obj.get_impliedVolatility(),
//           obj.get_delta(), obj.get_optPrice(), obj.get_pvDividend(),
//            obj.get_gamma(), obj.get_vega(), obj.get_theta(), obj.get_undPrice()  );


    QTime _curTime = QTime::currentTime();
    bool isWorkingHours = (_curTime >= startWorkingTime) && (_curTime <= endWorkingTime);

    //bool isWorkingHours = (_cur)
    if((13u == obj.get_field())  //Tick type == 13 (Model Option Computation) https://interactivebrokers.github.io/tws-api/tick_types.html
       && (true == isWorkingHours))
    {

        qint8 _multp = 1;
        if(((true == m_GuiInfo.isCall) && (false == m_GuiInfo.isBuy))
          || ((false == m_GuiInfo.isCall) && (false == m_GuiInfo.isBuy)))
        {
            _multp = -1;
        }


        m_delta.setDeltaOpt(static_cast<qint16>(qRound(obj.get_delta() * 100.0 * _multp)));

        emit signalUpdateOptionDeltaGUI(m_delta.getDeltaOpt());

        if(m_delta.isValid())
        {
            emit signalUpdateSumDeltaGUI(m_delta.getDeltaSum());
            //Todo: implement algorithm here
            if( false == m_IsOrderBusy )
            {
                qint32 absDeltaSum = static_cast<qint32>(qFabs(m_delta.getDeltaSum()));
                if(absDeltaSum >= m_deltaThresold)
                {
                    m_IsOrderBusy = true;
                    if(0 < m_delta.getDeltaSum())
                    {
                        requestPlaceMarketOrder(m_TicketName, absDeltaSum, OA_SELL);
                    }
                    else {
                        requestPlaceMarketOrder(m_TicketName, absDeltaSum, OA_BUY);
                    }
                    m_delta.resetDelta();
                }
            }
        }
    }
}


//----------------------------------------------------------
void autoDeltaAligPM::slotEndRecvPosition()
{
    bool isBaseActiveFaound = false;

    m_delta.clearObjects();

    // find all stock position according to ticketName
    auto it = m_positionMap.find(m_TicketName);
    while ((it != m_positionMap.end())
           && (it.key() == m_TicketName)) {

        if(it.value().getContract().secType == "STK")
        {
            m_delta.setDeltaBasis(m_delta.getDeltaBasis() + static_cast<qint16>(it.value().getPos()));
            isBaseActiveFaound = true;

            DeltaObjectTypePtr pObj = DeltaObjectTypePtr(new tDeltaObjectType{1,
                                                                              static_cast<qint16>(it.value().getPos()),
                                                                              E_AT_SOCK, "", true, 1, true});

            m_delta.addObjectEx(pObj);

            m_delta.addObject(*pObj.data());
        }
        else if (it.value().getContract().secType == "OPT") {

            DeltaObjectTypePtr pObj = DeltaObjectTypePtr(new tDeltaObjectType{100, static_cast<qint16>(it.value().getPos()), E_AT_OPTION,
                                                                          it.value().getContract().lastTradeDateOrContractMonth.c_str(),
                                                                          (it.value().getContract().right == "P" ? false : true),
                                                                          it.value().getContract().strike,
                                                                          false});
            m_delta.addObjectEx(pObj);
            m_delta.addObject(*pObj.data());
        }

        ++it;

    }

    if(false == isBaseActiveFaound)
    {
        m_delta.setDeltaBasis(0.0f);
    }

    //send signal to update GUI
    emit signalUpdateBasisDeltaGUI(m_delta.getDeltaBasis());

    //check if all delta data received
    if(m_delta.isValid())
    {
        //send signal to update gui with calculated delta
        emit signalUpdateSumDeltaGUI(m_delta.getDeltaSum());

        m_IsOrderBusy = false;
    }
}

//----------------------------------------------------------
void autoDeltaAligPM::slotOrderCommission(const qreal _commiss, const qreal _rpnl)
{
    m_commSum += _commiss * (-1.0);
    emit signalUpdateCommissionGUI(m_commSum);

    m_rpnlSum += _rpnl;
    emit signalUpdateRPNLGUI(m_rpnlSum);

    requestPosition();
    //isOrderBusy = false;
}

//----------------------------------------------------------
void autoDeltaAligPM::slotRestartSubscription()
{
    qCInfo(autoDeltaAligPMLog(),"slot RECV RESTART SUBSCRIPTION");
    bool isConnected = isConnectedTotheServer();
    if((true == isConnected) && (0 != getRequestMapSize()))
    {
        qInfo(autoDeltaAligPMLog(), "Restart");
        QThread::sleep(10);
        cancelAllActiveRequests();
        //UnsubscribeHandler();

        QThread::sleep(10);

        startStrategy(m_GuiInfo, m_deltaThresold);
    }
    else {
        qWarning(autoDeltaAligPMLog(), "No restart: severConnected = %d, requestsize = %d", isConnected, getRequestMapSize());
    }
}

//----------------------------------------------------------
void autoDeltaAligPM::slotTmpSendOrderBuy()
{
    requestPlaceMarketOrder("NVDA", 10.0, OA_BUY);
}

//----------------------------------------------------------
void autoDeltaAligPM::slotTmpSendOrderSell()
{
    requestPlaceMarketOrder("NVDA", 10.0, OA_SELL);
}

void autoDeltaAligPM::onManagerFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        reply->deleteLater();
        return;
    }
    QByteArray  content = reply->readAll();

    qCDebug(autoDeltaAligPMLog, "-----------------------------------> %s", QString(content).toLocal8Bit().data());

}

void autoDeltaAligPM::slotPostRequest()
{
    this->m_networkManager.post(this->request, this->m_modelInput.toJson());
}

//----------------------------------------------------------
//void autoDeltaAligPM::slotTimerTriggered()
//{
//
//}


//----------------------------------------------------------
void autoDeltaAligPM::callback_recvTickPrize(const IBDataTypes::CMyTickPrice _tickPrize, const QString& _symbol)
{
    Q_UNUSED(_tickPrize);
    Q_UNUSED(_symbol);
    //emit signalOnRealTimeTickData(_tickPrize, _symbol);
}

//----------------------------------------------------------
void autoDeltaAligPM::calllback_recvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol)
{
    Q_UNUSED(_histMap)
    Q_UNUSED(_symbol)

    //empty implementation
}

//----------------------------------------------------------

void autoDeltaAligPM::callback_recvPositionEnd()
{
    //TODO: implement something! ...
//    QThread::sleep(50);

//    QFuture<void> future = QtConcurrent::run(this, &autoDeltaAligPM::TestFunc);
    TestFunc();
}

void autoDeltaAligPM::recvRealtimeBar(void *pContext, tEReqType _reqType)
{
    Q_UNUSED(_reqType)
    //m_sem.acquire(1);
    if((NULL != pContext))
    {
        CrealtimeBar *_pRealTimeBar = (CrealtimeBar *)pContext;

        //Open	High	Low	Close	Volume
        this->m_modelInput.addObservation(
                    {0.0, 0.0},  // we don't need any previous information about position here
                    {_pRealTimeBar->getOpen(),
                     _pRealTimeBar->getHigh(),
                     _pRealTimeBar->getLow(),
                     _pRealTimeBar->getClose(),
                     _pRealTimeBar->getVolume()});

        emit signalPostRequest();
    }
    //m_sem.release(1);
}

void autoDeltaAligPM::TestFunc()
{
    QThread::sleep(2);
}

