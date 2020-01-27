#include "cprocessingbase.h"
#include "./IBComm/Dispatcher.h"
#include <QSharedPointer>
#include <QtConcurrent/QtConcurrentRun>

Q_LOGGING_CATEGORY(processingBaseLog, "processing.Base");

using namespace Observer;
//----------------------------------------------------------
CProcessingBase::CProcessingBase(QObject *parent, CBrokerDataProvider & _refClient)
    : QObject(parent)
    , m_Client(_refClient)
    , m_aciveReqestsMap()
    , m_historyMap()
	, m_histMap()
	, m_tickPriceMap()
	, m_tickSizeMap()
	, m_realTImeBarMap()
	, m_mkDepthMap()
    , m_positionMap()
    , m_nextValidId(0)
{
    //QObject::connect(this, &CProcessingBase::signalMessageHandler, this, &CProcessingBase::slotMessageHandler, Qt::QueuedConnection);
}

//----------------------------------------------------------
CProcessingBase::~CProcessingBase()
{
    (void)m_Client.UnsubscribeAllItemsOfSubscriber(this->GetSubscriberId());
}

//----------------------------------------------------------
void CProcessingBase::MessageHandler(void* pContext, tEReqType _reqType)
{
    emit signalMessageHandler(pContext, _reqType);
	switch (_reqType)
	{
	case RT_TICK_SIZE:
        recvTickSize(pContext, _reqType);
		break;
	case RT_TICK_PRICE:
        recvTickPrize(pContext, _reqType);
		break;
	case RT_HISTORICAL_DATA:
        recvHistoricalData(pContext, _reqType);
		break;
	case RT_REALTIME_BAR:
        recvRealtimeBar(pContext, _reqType);
		break;
    case RT_TICK_BY_TICK_DATA:
        recvTickByTickAllLastData(pContext, _reqType);
        break;
	case RT_MKT_DEPTH:
        recvMktDepth(pContext, _reqType);
        break;
    case RT_NEXT_VALID_ID:
        setNextValidId(*(reinterpret_cast<qint32*>(pContext)));
        break;
    case RT_REQ_POSITION:
        recvPosition(pContext, _reqType);
        break;
    case RT_REQ_RESTART_SUBSCRIPTION:
        recvRestartSubscription();
        break;
    case RT_REQ_OPTION_PRICE:
        recvOptionTickComputation(pContext, _reqType);
        break;
    case RT_ORDER_COMMISSION:
        recvOrdersCommission(pContext, _reqType);
        break;


	}
}

//----------------------------------------------------------
bool CProcessingBase::reqestHistoricalData(reqHistConfigData_t & _config)
{
    //m_aciveReqestsMap.insert(_symbol, RT_HISTORICAL_DATA);
    return m_Client.reqestHistoricalData(this, _config);
}

bool CProcessingBase::requestHistoricalTicksData(reqHistTicksConfigData_t &_config)
{
    return m_Client.requestHistoricalTicksData(this, _config);
}

//----------------------------------------------------------
bool CProcessingBase::reqestRealTimeData(reqReadlTimeDataConfigData_t &_config)
{
    m_aciveReqestsMap.insert(QString(_config.contract.symbol.c_str()), RT_REQ_REL_DATA);
    return m_Client.reqestRealTimeData(this, _config);
}

//----------------------------------------------------------
bool CProcessingBase::cancelRealTimeData(const QString& _symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_REQ_REL_DATA);
    return m_Client.cancelRealTimeData(this, _symbol);
}

bool CProcessingBase::requestRealTimeBars(const QString& _symbol)
{
    m_aciveReqestsMap.insert(_symbol, RT_REALTIME_BAR);
    return m_Client.requestRealTimeBars(this, _symbol);
}

//----------------------------------------------------------
bool CProcessingBase::cancelRealTimeBars(const QString& _symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_REALTIME_BAR);
    return m_Client.cancelRealTimeBars(this, _symbol);
}

//----------------------------------------------------------
bool CProcessingBase::requestPosition()
{
    m_positionMap.clear();
    //m_aciveReqestsMap.insert(PositionSymbol, RT_REQ_POSITION);
    return m_Client.requestPosition(this, PositionSymbol);
}

//----------------------------------------------------------
bool CProcessingBase::cancelPosition()
{
    //m_aciveReqestsMap.remove(PositionSymbol, RT_REQ_POSITION);
    return m_Client.cancelPosition(this, PositionSymbol);
}

bool CProcessingBase::reqestResetSubscription()
{
    m_aciveReqestsMap.insert(RestartRequestSymbol, RT_REQ_RESTART_SUBSCRIPTION);
    return m_Client.requestResetSubscription(this, RestartRequestSymbol);
}

bool CProcessingBase::cancelResetSubscription()
{
    m_aciveReqestsMap.remove(RestartRequestSymbol, RT_REQ_RESTART_SUBSCRIPTION);
    return m_Client.cancelResetSubscription(this, RestartRequestSymbol);
}

bool CProcessingBase::reqestOrderStatusSubscription()
{
    m_aciveReqestsMap.insert(OrderStatusSymbol, RT_REQ_ORDER_STATUS);
    return m_Client.requestOrderStatusSubscription(this, OrderStatusSymbol);

}

bool CProcessingBase::cancelOrderStatusSubscription()
{
    m_aciveReqestsMap.remove(OrderStatusSymbol, RT_REQ_ORDER_STATUS);
    return m_Client.cancelOrderStatusubscription(this, OrderStatusSymbol);
}

bool CProcessingBase::requestTickByTickData(reqTickByTickDataConfigData_t &_config)
{
    m_aciveReqestsMap.insert(QString(_config.contract.symbol.c_str()), RT_TICK_BY_TICK_DATA);
    return m_Client.requestTickByTickData(this, QString(_config.contract.symbol.c_str()), _config);
}

bool CProcessingBase::cancelTickByTickData(const QString &_symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_TICK_BY_TICK_DATA);
    return m_Client.cancelRealTimeBars(this, _symbol);
}

bool CProcessingBase::requestCalculateOptionPrice(reqCalcOptPriceConfigData_t &_config)
{
    m_aciveReqestsMap.insert(QString(_config.contract.symbol.c_str()), RT_REQ_OPTION_PRICE);
    return m_Client.requestCalculateOptionPrice(this, _config);
}

bool CProcessingBase::cancelCalculateOptionPrice(const QString &_symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_REQ_OPTION_PRICE);
    return m_Client.cancelRealTimeBars(this, _symbol);
}

//----------------------------------------------------------
qint32 CProcessingBase::requestPlaceMarketOrder(const QString& _symbol, const qint32 _quantity, const orderAction _action)
{
    //send order
    return m_Client.getClien()->reqPlaceOrderAPI(_symbol, _quantity, _action);
}

//----------------------------------------------------------
void CProcessingBase::requestOpenOrders()
{
    m_Client.getClien()->reqOpenOrdersAPI();
}

//----------------------------------------------------------
void CProcessingBase::cancelAllActiveRequests()
{
    QString symbol = "";
    tEReqType reqest = RT_REQ_NONE;

    ActiveReqestsMap_t::iterator it = m_aciveReqestsMap.begin();

    while (it != m_aciveReqestsMap.end())
    {
        symbol = it.key();
        reqest = it.value();
        if ((!symbol.isNull()) && (!symbol.isEmpty()))
        {
            switch (reqest)
            {
            case RT_REQ_NONE:
                break;
            case RT_REQ_REL_DATA:
                m_Client.cancelRealTimeData(this, symbol);
                break;
            case RT_REQ_CUR_TIME:
                break;
            case RT_TICK_SIZE:
                break;
            case RT_TICK_PRICE:
                break;
            case RT_TICK_GENERIC:
                break;
            case RT_TICK_STRING:
                break;
            case RT_HISTORICAL_DATA:
                break;
            case RT_HISTORICAL_TICK_DATA:
                break;
            case RT_TICK_BY_TICK_DATA:
                m_Client.cancelTickByTickData(this, symbol);
                break;
            case RT_REALTIME_BAR:
                m_Client.cancelRealTimeBars(this, symbol);
                break;
            case RT_MKT_DEPTH:
                break;
            case RT_MKT_DEPTH_L2:
                break;
            case RT_NEXT_VALID_ID:
                break;
            case RT_REQ_POSITION:
                m_Client.cancelResetSubscription(this, PositionSymbol);
                break;
            case RT_POSITION:
                break;
            case RT_REQ_OPTION_PRICE:
                m_Client.cancelCalculateOptionPrice(this, symbol);
                break;
            case RT_REQ_RESTART_SUBSCRIPTION:
                m_Client.cancelPosition(this, RestartRequestSymbol);
                break;
            case RT_REQ_ORDER_STATUS:
                m_Client.cancelOrderStatusubscription(this, OrderStatusSymbol);
                break;



            }
        }
        ++it;
    }

    m_aciveReqestsMap.clear();
}

bool CProcessingBase::isConnectedTotheServer()
{
    return m_Client.isConnectedToTheServer();
}

//

//----------------------------------------------------------
qint64 CProcessingBase::getIdFromRM(const QString _symbol, const tEReqType _reqType /*= RT_REQ_REL_DATA*/)
{
    qint64 retId = 0;
    CDispatcher::CSubscriberItemPtr pSubcrItem = m_Client.getSubscriberItem(this->GetSubscriberId());
    if (nullptr != pSubcrItem)
    {
        retId = pSubcrItem->ReqMenager()->getIdbySymbol(_symbol, _reqType);
    }
    return retId;
}

//----------------------------------------------------------
const QString CProcessingBase::getSymbolFromRM(const qint64 _Id, const tEReqType _reqType /*= RT_REQ_REL_DATA*/)
{
    QString retSymbol = "";
    CDispatcher::CSubscriberItemPtr pSubcrItem = m_Client.getSubscriberItem(this->GetSubscriberId());
    if (nullptr != pSubcrItem)
    {
        retSymbol = pSubcrItem->ReqMenager()->getSymbolById(_Id, _reqType);
    }
    return retSymbol;
}



//----------------------------------------------------------
void CProcessingBase::removeOldHistoricalRequest(const QString & _s1)
{
    CDispatcher::CSubscriberItemPtr pSubcrItem = m_Client.getSubscriberItem(this->GetSubscriberId());
    if (nullptr != pSubcrItem)
    {
        pSubcrItem->ReqMenager()->removeReqData(_s1, RT_HISTORICAL_DATA);
    }
    return;
}

void CProcessingBase::removeOldHistoricalTickRequest(const QString &_s1)
{
    CDispatcher::CSubscriberItemPtr pSubcrItem = m_Client.getSubscriberItem(this->GetSubscriberId());
    if (nullptr != pSubcrItem)
    {
        pSubcrItem->ReqMenager()->removeReqData(_s1, RT_HISTORICAL_TICK_DATA);
    }
    return;
}

void CProcessingBase::callback_recvTickPrize(const CMyTickPrice _tickPrize, const QString &_symbol)
{
    Q_UNUSED(_tickPrize);
    Q_UNUSED(_symbol);
}

void CProcessingBase::calllback_recvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    Q_UNUSED(_histMap);
    Q_UNUSED(_symbol);
}

void CProcessingBase::callback_recvPositionEnd()
{

}

//void CProcessingBase::slotMessageHandler(void *pContext, tEReqType _reqType)
//{
//    qCDebug(processingBaseLog(), "PosEnd!! -> execte callback to implementation");
//}

//----------------------------------------------------------
void CProcessingBase::UnsubscribeHandler()
{
    (void)m_Client.UnsubscribeAllItemsOfSubscriber(this->GetSubscriberId());

   // cancelAllActiveRequests();
}

//----------------------------------------------------------
void CProcessingBase::recvHistoricalData(void* pContext, tEReqType _reqType)
{
    //QString a = QThread::currentThread()->objectName();
    //qCDebug(processingBaseLog(), "---> %s", a.toLocal8Bit().data());
    CHistoricalData *_pHistoricalData = (CHistoricalData *)pContext;

    QString retSymbol = getSymbolFromRM(_pHistoricalData->getId(), RT_HISTORICAL_DATA);

    qCDebug(processingBaseLog(), "[%s] tickerId = %d , date = %d, open = %f, high = %f, low =%f, close = %f, volume = %d, barCount = %d, WAP = %f, hasGaps = %d", retSymbol.toLocal8Bit().data(),
        _pHistoricalData->getId(), _pHistoricalData->getDateTime(), _pHistoricalData->getOpen(), _pHistoricalData->getHigh(), _pHistoricalData->getLow(),
        _pHistoricalData->getClose(), _pHistoricalData->getVolume(), _pHistoricalData->getCount(), _pHistoricalData->getWap(), _pHistoricalData->getHasGaps());

    qint64 realtimeDataId = getIdFromRM(retSymbol, RT_HISTORICAL_DATA);
    if (0 != realtimeDataId)
    {
        bool isFound = false;
        HistoricalDataMap_t::iterator it = m_historyMap.find(realtimeDataId);
        while ((it != m_historyMap.end()) && (it.key() == realtimeDataId) && (!isFound)) {
            isFound = true;
        }

        if (_pHistoricalData->getIsLast())
        {
            //End of historical data
            removeOldHistoricalRequest(retSymbol);

            //set data as available
            it->isAvaliable = true;

            // temp
            // signalOnFinishHistoricalData(it->listHistData, retSymbol);
            //calllback_recvHistoricalData(it->listHistData, retSymbol);
            signalCbkRecvHistoricalData(it->listHistData, retSymbol);
        }
        else
        {
            if (isFound)
            {
                //found in map
                if (!it->isAvaliable)
                {
                    // historical data is not fully available
                    //add to the list of historical data
                    it->listHistData.append(*_pHistoricalData);
                }
                else
                {
                    // historical data is fully available
                    // clear data to avoid double data in map
                    it->listHistData.clear();
                    it->isAvaliable = false;

                    it->listHistData.append(*_pHistoricalData);
                }

            }
            else
            {
                // not found in map
                //add to the map
                HistoricalData_st tmpHistStruct;
                tmpHistStruct.isAvaliable = false;
                tmpHistStruct.listHistData.append(*_pHistoricalData);
                m_historyMap.insert(realtimeDataId, tmpHistStruct);

            }
        }
    }
    else
    {
        //m_pLog.AddLogMsg("ignore/remove from RM");
        qCInfo(processingBaseLog(), "ignore/remove from RM");
        removeOldHistoricalRequest(retSymbol);
    }
}

//----------------------------------------------------------
void CProcessingBase::recvTickSize(void* pContext, tEReqType _reqType)
{
    CTickSize *_pTickSize = (CTickSize *)pContext;
    m_tickSizeMap.insert(_pTickSize->getId(), *_pTickSize);

    qCDebug(processingBaseLog(), "id = %d", _pTickSize->getId());

}

//----------------------------------------------------------
void CProcessingBase::recvTickPrize(void* pContext, tEReqType _reqType)
{
    IBDataTypes::CMyTickPrice* _pTickPrice = (IBDataTypes::CMyTickPrice *)pContext;
    m_tickPriceMap.insert(_pTickPrice->getId(), *_pTickPrice);

    QString retSymbol = getSymbolFromRM(_pTickPrice->getId());

    qCDebug(processingBaseLog(), "id = %d, price = %f", __FUNCTION__, _pTickPrice->getId(), _pTickPrice->getPrice());

    IBDataTypes::CMyTickPrice dd(*_pTickPrice);
    
    //temp
    //emit signalOnRealTimeTickData(dd, retSymbol);
    callback_recvTickPrize(dd, retSymbol);

}

//----------------------------------------------------------
void CProcessingBase::recvRealtimeBar(void* pContext, tEReqType _reqType)
{
    CrealtimeBar *_pRealTimeBar = (CrealtimeBar *)pContext;
    m_realTImeBarMap.insert(_pRealTimeBar->getId(), *_pRealTimeBar);

    qCDebug(processingBaseLog(), "executed");
    calculateOneMinBar(m_realTImeBarMap, _pRealTimeBar->getId());
}

//----------------------------------------------------------
void CProcessingBase::recvTickByTickAllLastData(void *pContext, tEReqType _reqType)
{

}

//----------------------------------------------------------
void CProcessingBase::recvMktDepth(void* pContext, tEReqType _reqType)
{
    Q_UNUSED(_reqType);
    CMktDepth *_pMKDepth = static_cast<CMktDepth *>(pContext);
    m_mkDepthMap.insert(_pMKDepth->getId(), *_pMKDepth);

    qCDebug(processingBaseLog(), "id = %d", _pMKDepth->getId());
}

//----------------------------------------------------------
void CProcessingBase::recvPosition(void *pContext, tEReqType _reqType)
{
    Q_UNUSED(_reqType);
    if(nullptr == pContext)
    {
        recvPositionEnd();
    }
    else
    {
        CPosition *_pPos = static_cast<CPosition *>(pContext);
        m_positionMap.insert(QString::fromLocal8Bit(_pPos->getContract().symbol.data(), static_cast<qint32>(_pPos->getContract().symbol.size()))
                             , *_pPos);
        qCDebug(processingBaseLog(), "Pos = %s", _pPos->getContract().symbol.c_str());
    }
}

//----------------------------------------------------------
void CProcessingBase::recvPositionEnd()
{
    qCDebug(processingBaseLog(), "PosEnd!! -> execte callback to implementation");
    //callback_recvPositionEnd();
    emit signalEndRecvPosition();
}

//COptionTickComputation obj;
//----------------------------------------------------------
void CProcessingBase::recvOptionTickComputation(void *pContext, tEReqType _reqType)
{
    //temp
    if(RT_REQ_OPTION_PRICE == _reqType)
    {
        COptionTickComputation obj(*(static_cast<COptionTickComputation*>(pContext)));
        emit signalRecvOptionTickComputation(obj);
    }

}

//----------------------------------------------------------
void CProcessingBase::recvRestartSubscription()
{
    emit signalRestartSubscription();
    //nothing here
    qCDebug(processingBaseLog(), "------->>> recvRestartSubscription emit RESTART SUBSCRIPTION");
}

void CProcessingBase::recvOrdersCommission(void* pContext, tEReqType _reqType)
{
    Q_UNUSED(_reqType);
    //TODO: add normal commission object!
    emit signalRecvOrderCommission(*static_cast<qreal*>(pContext), -1.04);
}


//-----------------------------------------------------------
bool CProcessingBase::calculateOneMinBar(RealTimeBarMap_t & _realTimeBar, TickerId _id)
{
    bool isFound = false;
    RealTimeBarMap_t::iterator it = _realTimeBar.begin();
    
    while ((it != _realTimeBar.begin()) && (it.key() == _id) && (!isFound)) {
        isFound = true;
    }

    //--it;
    QDateTime mtime = QDateTime::fromTime_t(it->getDateTime());
    qCDebug(processingBaseLog(), "time = %s", mtime.toString("yyyy/MM/dd hh:mm:ss").toLocal8Bit().data());

    static QDateTime oldtime;
    //bool isBarFinished = false;


    double o, c, h, l = 0.0;

    //--------------------------- 
    enum tEBarState
    {
        BS_INIT = 0,
        BS_PROCESSING,
        BS_FINISH
    };

    //form temporary bar
    static CrealtimeBar rtBar; 
    //static bool isFirstBar = true;

    static tEBarState localBarState = BS_INIT;

    switch (localBarState)
    {
    case BS_INIT:
        rtBar.setOpen(it->getOpen());
        rtBar.setClose(it->getClose());
        rtBar.setHigh(it->getHigh());
        rtBar.setLow(it->getLow());

        localBarState = BS_PROCESSING;
        break;
    case BS_PROCESSING:
        if ((oldtime.date() != mtime.date()) || (mtime.time().minute() != oldtime.time().minute()))//(mtime.time().second() < 1))
        {
            localBarState = BS_INIT;
        }
        rtBar.setClose(it->getClose());
        if (rtBar.getHigh() < it->getHigh())
        {
            rtBar.setHigh(it->getHigh());
        }
        if (rtBar.getLow() > it->getLow())
        {
            rtBar.setLow(it->getLow());
        }
        break;
    case BS_FINISH:
        break;
//    default:
//        break;
    }

    oldtime = mtime;
//    rtBar.setVolume(rtBar.getVolume() + it->getVolume());
    //quint8 s = static_cast<quint8>(oldtime.time().second());
    o = rtBar.getOpen();
    c = rtBar.getClose();
    l = rtBar.getLow();
    h = rtBar.getHigh();

    if (localBarState == BS_INIT)
    {
        qCDebug(processingBaseLog(), "o = %f, c = %f, h = %f, l = %f", o, c, h, l);
        o = 0;
        h = 0;
        l = 0;
        c = 0;
    }



    return true;
}

qint32 CProcessingBase::getRequestMapSize() const
{
    return m_aciveReqestsMap.size();
}


