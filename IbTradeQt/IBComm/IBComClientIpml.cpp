#include <QDateTime>
#include <QDebug>

#include "IBComClientImpl.h"

//#include "EPosixClientSocket.h"
//#include "EPosixClientSocketPlatform.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "CommonDefs.h"
#include "GlobalDef.h"

#include "cticksize.h"
#include "ctickprice.h"
#include "ctickgeneric.h"
#include "ctickstring.h"
#include "CHistoricalData.h"
#include "crealtimebar.h"
#include "cmktdepth.h"
#include "cmktdepthl2.h"
#include "corderstatus.h"
#include "cposition.h"
#include "coptiontickcomputation.h"
#include "ctickbytickalllast.h"

#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "bar.h"
#include "NHelper.h"

using namespace IBDataTypes;

Q_LOGGING_CATEGORY(IBComClientImplLog, "ibComClientImpl.Callback");


IBComClientImpl::IBComClientImpl(Observer::CDispatcher & _dispatcher)
    : m_osSignal(2000)//2-seconds timeout
    //: m_osSignal(00)//2-seconds timeout
    , m_pClient(new EClientSocket(this, &m_osSignal))
    , m_pReader(nullptr)
    , m_extraAuth(false)
    , m_pLog(LOGGER)
    , m_nexValidId(0)
    , m_DispatcherBrokerData(_dispatcher)
{
}


IBComClientImpl::~IBComClientImpl()
{
    if (m_pReader)
        delete m_pReader;

    delete m_pClient;

}


//IBrokerAPI
bool IBComClientImpl::connectAPI(const char *host, unsigned int port, int clientId)
{

	// trying to connect
	qCInfo(IBComClientImplLog(), "Connecting to %s:%d clientId:%d\n", !(host && *host) ? "127.0.0.1" : host, port, clientId);

    bool bRes = m_pClient->eConnect(host, static_cast<int>(port), clientId, /* extraAuth */ false);

	if (bRes) {
        qCInfo(IBComClientImplLog(), "Connected to %s:%d  clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
        m_pReader = new EReader(m_pClient, &m_osSignal);

        m_pReader->start();

	}
	else {
		qCWarning(IBComClientImplLog(), "Cannot connect to %s:%d  clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
	}

	return bRes;

}
bool IBComClientImpl::isConnectedAPI()
{
	return m_pClient->isConnected();
}
void IBComClientImpl::disconnectAPI()
{
	m_pClient->eDisconnect();

    qCInfo(IBComClientImplLog(), "Disconnected");

}


void IBComClientImpl::setConnectOptions(const std::string& connectOptions)
{
    m_pClient->setConnectOptions(connectOptions);
}



void IBComClientImpl::reqCurrentTimeAPI()
{
	m_pClient->reqCurrentTime();
}


void IBComClientImpl::reqRealTimeDataAPI(const qint32 _id, reqReadlTimeDataConfigData_t &_config)
{
    TagValueListSPtr mktDataOptions;

    m_pClient->reqMktData(_id, _config.contract, _config.genericTickList.toStdString(),
                          _config.snapshot, _config.regulatorySnaphsot, mktDataOptions);
}

void IBComClientImpl::cancelRealTimeDataAPI(const qint32 _id)
{
	m_pClient->cancelMktData(_id);
}



void IBComClientImpl::reqHistoricalDataAPI(const reqHistConfigData_t & _config)
{
    reqHist_t reqHist;
    // Ending date for the time series
    reqHist.strEndDate = QDateTime::currentDateTime().toString("yyyyMMdd HH:mm:ss").toStdString().c_str();

    // Amount of time up to the end date
    reqHist.strDuration = _config.duration;
    // Bar size
    //reqHist.strBarSize = "1 hour";
    reqHist.strBarSize = _config.barSize;
    // Data type TRADES= OHLC Trades with volume
    reqHist.strWhatToShow = "TRADES";

    Contract m_contract;

    reqHist.m_contract.symbol = _config.symbol.toStdString();
    reqHist.m_contract.secType = "STK";
    reqHist.m_contract.strike = 0;
    reqHist.m_contract.currency = "USD";
    reqHist.m_contract.exchange = "SMART";
    reqHist.m_contract.primaryExchange = "ISLAND";

	TagValueListSPtr mktDataOptions;

    m_pClient->reqHistoricalData(_config.id, reqHist.m_contract,
        reqHist.strEndDate.toStdString(), reqHist.strDuration.toStdString(), reqHist.strBarSize.toStdString(), reqHist.strWhatToShow.toStdString(),
        1, 1, false, mktDataOptions);
}

void IBComClientImpl::cancelHistoricalDataAPI(const qint32 id)
{
	m_pClient->cancelHistoricalData(id);
}


//---------------------------------------------------------------
void IBComClientImpl::reqRealTimeBarsAPI(const qint32 _id, const QString& _symbol)
{
    TagValueListSPtr realTimeBarOptions;
    reqRealTimeBars_t reqRTB;

    Contract m_contract;

    reqRTB.contract.symbol = _symbol.toStdString();
    reqRTB.contract.secType = "STK";
    reqRTB.contract.strike = 0;
    reqRTB.contract.currency = "USD";
    reqRTB.contract.exchange = "SMART";
    reqRTB.contract.primaryExchange = "ISLAND";

    m_pClient->reqRealTimeBars(_id, reqRTB.contract, reqRTB.barSize, reqRTB.whatToShow.toStdString(), reqRTB.useRTH, realTimeBarOptions);
}

//---------------------------------------------------------------
void IBComClientImpl::cancelRealTimeBarsAPI(const qint32 _id)
{
    m_pClient->cancelRealTimeBars(_id);
}


//---------------------------------------------------------------
void IBComClientImpl::reqPositionAPI(const qint32 _id)
{
    m_pClient->reqPositions();
}

//---------------------------------------------------------------
void IBComClientImpl::cancelPositionAPI(const qint32 _id)
{
    m_pClient->cancelPositions();
}

//---------------------------------------------------------------
qint32 IBComClientImpl::reqPlaceOrderAPI(const QString& _symbol, const qint32 _quantity, const orderAction _action)
{
    reqPlaceOrder_t orderToPlace;
    qint32 retOrderId = getNexValidId();
    // fill contract
    orderToPlace.contract.symbol = _symbol.toLocal8Bit().data();
    orderToPlace.contract.secType = "STK";
    orderToPlace.contract.exchange = "SMART";
    orderToPlace.contract.primaryExchange = "ISLAND";

    // fill order
    if (OA_BUY == _action)
    {
        orderToPlace.order.action = "BUY";
    }
    else
    {
        orderToPlace.order.action = "SELL";
    }

    orderToPlace.order.orderType = "MKT";
    orderToPlace.order.tif = "DAY";
    orderToPlace.order.totalQuantity = _quantity;
    orderToPlace.order.transmit = true;
    orderToPlace.order.orderId = retOrderId;

    m_pClient->placeOrder(retOrderId, orderToPlace.contract, orderToPlace.order);

    return retOrderId;
}

//---------------------------------------------------------------
void IBComClientImpl::cancelOrderAPI(const qint32 _id)
{
    const std::string manualOrderCancelTime = "100";
    m_pClient->cancelOrder(_id, manualOrderCancelTime);
}

//---------------------------------------------------------------
void IBComClientImpl::reqOpenOrdersAPI()
{
    m_pClient->reqOpenOrders();
}

//---------------------------------------------------------------
void IBComClientImpl::reqAllOpenOrdersAPI()
{
    m_pClient->reqAllOpenOrders();
}

//---------------------------------------------------------------
void IBComClientImpl::reqAutoOpenOrdersAPI(const bool _bAutoBind)
{
    m_pClient->reqAutoOpenOrders(_bAutoBind);
}

//---------------------------------------------------------------
void IBComClientImpl::reqNextValidIDsAPI(const qint32 _numIds)
{
    Q_UNUSED(_numIds);
    //The parameter is always ignored. (info from broker AI documentation)
    //client.reqIds(-1);
    m_pClient->reqIds(-1);
}

//---------------------------------------------------------------
void IBComClientImpl::reqGlobalCancelAPI()
{
    m_pClient->reqGlobalCancel();
}

//---------------------------------------------------------------
void IBComClientImpl::reqCalculateOptionPriceAPI(const reqCalcOptPriceConfigData_t &_config)
{
    TagValueListSPtr lstOptions;
    m_pClient->calculateOptionPrice(_config.id,
                                    _config.contract,
                                    _config.volatility,
                                    _config.underPrice,
                                    lstOptions);
}
//---------------------------------------------------------------
void IBComClientImpl::cancelCalculateOptionPriceAPI(const qint32 _id)
{
    m_pClient->cancelCalculateOptionPrice(_id);
}
//---------------------------------------------------------------
void IBComClientImpl::reqHistoricalTicksAPI(const reqHistTicksConfigData_t &_config)
{
   TagValueListSPtr mktDataOptions;

   m_pClient->reqHistoricalTicks(_config.id,
                                 _config.contract,
                                 _config.startDateTime.toLocal8Bit().data(),
                                 _config.endDateTime.toLocal8Bit().data(),
                                 _config.numberOfTicks,
                                 _config.whatToShow.toLocal8Bit().data(),
                                 _config.useRth,
                                 _config.ignoreSize,
                                 mktDataOptions);
}
//---------------------------------------------------------------
void IBComClientImpl::reqTickByTickDataAPI(const reqTickByTickDataConfigData_t &_config)
{
    m_pClient->reqTickByTickData(_config.id,
                                 _config.contract,
                                 _config.tickType.toLocal8Bit().data(),
                                 _config.numberOfTicks,
                                 _config.ignoreSize);
}
//---------------------------------------------------------------
void IBComClientImpl::cancelTickByTickDataAPI(const qint32 id)
{
    m_pClient->cancelTickByTickData(id);
}

//---------------------------------------------------------------
// implementation of API Callbacks
//---------------------------------------------------------------

//void IBComClientImpl::setUseV100Plus(const std::string& connectOptions)
//{
//	m_pClient->setUseV100Plus(connectOptions);
//}

//---------------------------------------------------------------
void IBComClientImpl::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib)
{
    //TODO:Add attrib to the MyTickProce
    //CTickPrice _tickPrize(tickerId, field, price, canAutoExecute, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    IBDataTypes::CMyTickPrice _tickPrize(tickerId, field, price, attrib.canAutoExecute, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    qCDebug(IBComClientImplLog(), "tickerId = %ld, field = %d, price = %f, canAutoExecute = %d\n", _tickPrize.getId(), _tickPrize.getTickType(), _tickPrize.getPrice(), _tickPrize.getCanAutoExecute());

    m_DispatcherBrokerData.SendMessageToSubscribers(&_tickPrize, tickerId, RT_TICK_PRICE);
    
	return;
};

//---------------------------------------------------------------
void IBComClientImpl::tickSize(TickerId tickerId, TickType field, Decimal size)
{
	//qDebug("tickSize : tickerId = %d, field = %d, size = %d", tickerId, field, size);

    CTickSize _tickSize(tickerId, field, size, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());

    qCDebug(IBComClientImplLog(), "tickerId = %ld, field = %d, size = %d", _tickSize.getId(), _tickSize.getTickType(), _tickSize.getSize());

    m_DispatcherBrokerData.SendMessageToSubscribers(&_tickSize, tickerId, RT_TICK_SIZE);

	return;
};

//---------------------------------------------------------------
void IBComClientImpl::tickGeneric(TickerId tickerId, TickType tickType, double value)
{
    //qCDebug(IBComClientImplLog(), "[%s] tickerId = %d, tickType = %d, value = %f", __FUNCTION__, tickerId, tickType, value);
    CTickGeneric _tickGeneric(tickerId, tickType, value, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    qCDebug(IBComClientImplLog(), "tickerId = %ld, tickType = %d, value = %f", _tickGeneric.getId(), _tickGeneric.getTickType(), _tickGeneric.getValue());
    
    m_DispatcherBrokerData.SendMessageToSubscribers(&_tickGeneric, tickerId, RT_TICK_GENERIC);

    return;
};

//---------------------------------------------------------------
void IBComClientImpl::tickString(TickerId tickerId, TickType tickType, const std::string& value)
{
    //qCDebug(IBComClientImplLog(), "tickString : tickerId = %d, tickType = %d, value = %s", tickerId, tickType, value.c_str());

    CTickString _tickString(tickerId, tickType, QString::fromLocal8Bit(value.data(), value.size()), QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    qCDebug(IBComClientImplLog(), "tickerId = %ld, tickType = %d, value = %s", _tickString.getId(), _tickString.getTickType(), _tickString.getValue().toLocal8Bit().data());

    
    m_DispatcherBrokerData.SendMessageToSubscribers(&_tickString, tickerId, RT_TICK_STRING);

    return;
}


//---------------------------------------------------------------
//TODO:
//void IBComClientImpl::historicalData(TickerId reqId, const std::string& date, double open, double high,
//    double low, double close, int volume, int barCount, double WAP, int hasGaps)
void IBComClientImpl::historicalData(TickerId reqId, const Bar& bar)
{
/*    QString tmpString(QString::fromLocal8Bit(date.data(), date.size()));
    
    
    if (date.find("finished") != std::string::npos) 
    {
        CHistoricalData _historicalData;
        _historicalData.setId(reqId);
        _historicalData.setIsLast(true);
        qCDebug(IBComClientImplLog(), "tickerId = %d : finished", _historicalData.getId());

        m_DispatcherBrokerData.SendMessageToSubscribers(&_historicalData, reqId, RT_HISTORICAL_DATA);

//        qint32 n = date.rfind("-");
//        tmpString.remove(0, n+1);

    }
    else
    {
        CHistoricalData _historicalData(reqId, tmpString, open, high, low, close, volume, barCount, WAP, hasGaps);

        qCDebug(IBComClientImplLog(), "tickerId = %d , date = %ld [%s], open = %f, high = %f, low =%f, close = %f, volume = %d, barCount = %d, WAP = %f, hasGaps = %d", 
            _historicalData.getId(), _historicalData.getDateTime(), date.c_str(), _historicalData.getOpen(), _historicalData.getHigh(), _historicalData.getLow(),
            _historicalData.getClose(), _historicalData.getVolume(), _historicalData.getCount(), _historicalData.getWap(), _historicalData.getHasGaps());

        m_DispatcherBrokerData.SendMessageToSubscribers(&_historicalData, reqId, RT_HISTORICAL_DATA);
    }

*/
}


//---------------------------------------------------------------
void IBComClientImpl::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
    Decimal volume, Decimal wap, int count)
{

    CrealtimeBar _realtimeBar(reqId, static_cast<quint64>(time), open, high, low, close, volume, count, wap);

    qCDebug(IBComClientImplLog(), "tickerId = %ld , date = %d, open = %f, high = %f, low =%f, close = %f, volume = %f, barCount = %d, WAP = %f, ",
		_realtimeBar.getId(), _realtimeBar.getDateTime(), _realtimeBar.getOpen(), _realtimeBar.getHigh(), _realtimeBar.getLow(),
		_realtimeBar.getClose(), _realtimeBar.getVolume(), _realtimeBar.getCount(), _realtimeBar.getWap());

    m_DispatcherBrokerData.SendMessageToSubscribers(&_realtimeBar, reqId, RT_REALTIME_BAR);

    return;
};

//---------------------------------------------------------------
void IBComClientImpl::updateMktDepth(TickerId id, int position, int operation, int side,
    double price, Decimal size)
{
    CMktDepth _mkdDepth(id, position, operation, side, price, size, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());

    qCDebug(IBComClientImplLog(), "tickerId = %ld , pos = %d, operation = %d, side = %d, double price = %f, int size = %d ",
		_mkdDepth.getId(), _mkdDepth.getPosition(), _mkdDepth.getOperation(), _mkdDepth.getSide(), _mkdDepth.getPrice(), _mkdDepth.getSize());

    m_DispatcherBrokerData.SendMessageToSubscribers(&_mkdDepth, id, RT_MKT_DEPTH);

	return;
}

//---------------------------------------------------------------
//TODO:
//void IBComClientImpl::updateMktDepthL2(TickerId id, int position, std::string marketMaker, int operation,
//	int side, double price, int size)
void IBComClientImpl::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
    int side, double price, Decimal size, bool isSmartDepth)

{
    CMktDepthL2 _mkdDepthL2(id, position, QString::fromLocal8Bit(marketMaker.data(), marketMaker.size()), operation, side, price, size, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
//TODO:
//	qCDebug(IBComClientImplLog(), "tickerId = %d , pos = %d, MM = %s, operation = %d, side = , double price, int size ",
//		_mkdDepthL2.getId(), _mkdDepthL2.getPosition(), _mkdDepthL2.getMarketMaker(), _mkdDepthL2.getOperation(), _mkdDepthL2.getSide(), _mkdDepthL2.getPrice(), _mkdDepthL2.getSize());

    m_DispatcherBrokerData.SendMessageToSubscribers(&_mkdDepthL2, id, RT_MKT_DEPTH_L2);

	return;

}



//---------------------------------------------------------------
void IBComClientImpl::tickOptionComputation(TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta,
                                            double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
{
    COptionTickComputation optionTick(tickerId, tickType, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);

    qDebug("tickOptionComputation : tickerId = %ld , tickType = %d, impliedVol = %f, delta = %f, optPrice = %f, pvDividend = %f, gamma = %f, vega = %f, theta = %f, undPrice = %f",
		tickerId, tickType, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);

    m_DispatcherBrokerData.SendMessageToSubscribers(&optionTick, tickerId, RT_REQ_OPTION_PRICE);

    return;
};


//---------------------------------------------------------------
void IBComClientImpl::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
	double totalDividends, int holdDays, const std::string& futureExpiry, double dividendImpact, double dividendsToExpiry)
{
    qDebug("tickEFP : tickerId = %ld , tickType = %d, basisPoints = %f, formattedBasisPoints = %s, totalDividends =%f, holdDays = %d, futureExpiry = %s, dividendImpact = %f, dividendsToExpiry = %f",
		tickerId, tickType, basisPoints, formattedBasisPoints.c_str(), totalDividends, holdDays, futureExpiry.c_str(), dividendImpact, dividendsToExpiry);
	return;
};


//---------------------------------------------------------------
void IBComClientImpl::nextValidId(OrderId orderId)
{
    qCDebug(IBComClientImplLog(), "nextValidId = %ld \n", orderId);
    m_nexValidId = orderId;
	
    m_DispatcherBrokerData.SendMessageToSubscribers(&orderId, orderId, RT_NEXT_VALID_ID);

    return;

}

//---------------------------------------------------------------
void IBComClientImpl::currentTime(long time)
{
    m_DispatcherBrokerData.SendMessageToSubscribers(&time, E_RQ_ID_TIME, RT_REQ_CUR_TIME);

}

//---------------------------------------------------------------
//void IBComClientImpl::error(const int id, const int errorCode, const std::string errorString)
void IBComClientImpl::error(int id, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson)
{
    qCCritical(IBComClientImplLog(),  "Error id=%d, errorCode=%d, msg=%s\n", id, errorCode, errorString.c_str());

    //if (id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
    //	disconnectAPI();

    if (id == -1 && errorCode == 504 && 1100) // if "Connectivity between IB and TWS has been lost"
    {
        connectAPI(CONNECTIONS_SERVER_IP, NHelper::getServerPort(), CONNECTIONS_CLIENT_ID);
    }


    if((1101 == errorCode)
            || (502 == errorCode)
            //|| (2106 == errorCode)
            || (1102 == errorCode))
    {
       //send message to restart command.
       m_DispatcherBrokerData.SendMessageToSubscribers(nullptr, E_RQ_ID_RESTART_SUBSCRIPTION, RT_REQ_RESTART_SUBSCRIPTION);
    }

    
}

//---------------------------------------------------------------
void IBComClientImpl::winError(const std::string& str, int lastError)
{
	qCDebug(IBComClientImplLog(), "lastError=%d, msg=%s\n", lastError, str.c_str());
};


//---------------------------------------------------------------
//TODO:
//void IBComClientImpl::orderStatus(OrderId orderId, const std::string& status, double filled,
//	double remaining, double avgFillPrice, int permId, int parentId,
//	double lastFillPrice, int clientId, const std::string& whyHeld)
void IBComClientImpl::orderStatus( OrderId orderId, const std::string& status, Decimal filled,
        Decimal remaining, double avgFillPrice, int permId, int parentId,
        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
{

	//COrderStatus(qint32	_id,
	//	QString _status,
	//	qint32	_filled,
	//	qint32	_remaining,
	//	qreal	_avgFillPrice,
	//	qint32	_permId,
	//	qint32	_parentId,
	//	qreal	_lastFilledPrice,
	//	qint32	_clientId,
	//	QString _whyHeld

	COrderStatus orderStatusObj((qint32)orderId, QString::fromLocal8Bit(status.data(), status.size()), filled, remaining, avgFillPrice,
		permId, parentId, lastFillPrice, clientId, QString::fromLocal8Bit(whyHeld.data(), whyHeld.size()));


	qCDebug(IBComClientImplLog(), "orderId = %d, status %s, filled = %d, remaining = %f, avgFillPrice = %f, permId = %d, parentId = %d, lastFillPrice = %f, clientId = %d, whyHeld =%s", 
		orderStatusObj.getId(), orderStatusObj.getStatus().toLocal8Bit().data(), orderStatusObj.getFilled(), orderStatusObj.getRemaining(), orderStatusObj.getAvgFillPrice(), orderStatusObj.getPermId(), 
		orderStatusObj.getParentId(), orderStatusObj.getLastFilledPrice(), orderStatusObj.getClientId(), orderStatusObj.getWhyHeld().toLocal8Bit().data());
        
	
    //m_DispatcherBrokerData.SendMessageToSubscribers(&orderStatusObj, id, RT_MKT_DEPTH_L2);


	return;


}
//---------------------------------------------------------------
void IBComClientImpl::openOrder(OrderId orderId, const Contract& _contract, const Order& _order, const OrderState& _orderState)
{
    qCDebug(IBComClientImplLog(), "OpenOrder. ID: %ld %s @ %s %s: %s, %s %f %s", orderId, _contract.symbol.c_str(), _contract.secType.c_str(), _contract.exchange.c_str(),
        _order.action.c_str(), _order.orderType.c_str(), _order.totalQuantity, _orderState.status.c_str());
}

//---------------------------------------------------------------
void IBComClientImpl::openOrderEnd()
{
    qCDebug(IBComClientImplLog(), "- * -");
}

//---------------------------------------------------------------
void IBComClientImpl::execDetailsEnd(int reqId)
{
    qCDebug(IBComClientImplLog(), "id = %d", reqId);

};

//---------------------------------------------------------------
void IBComClientImpl::execDetails(int reqId, const Contract& contract, const Execution& execution) {
	qCDebug(IBComClientImplLog(), "ReqId: %d - %s, %s, %s - %s, %ld, %g\n", reqId, contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(),
		execution.execId.c_str(), execution.orderId, execution.shares);
}

//---------------------------------------------------------------
void IBComClientImpl::commissionReport(const CommissionReport& commissionReport)
{
    qCDebug(IBComClientImplLog(), "%s - %g %s RPNL %g\n", commissionReport.execId.c_str(), commissionReport.commission, commissionReport.currency.c_str(), commissionReport.realizedPNL);

    qreal _commiss = commissionReport.commission;
    m_DispatcherBrokerData.SendMessageToSubscribers(&_commiss, E_RQ_ID_ORDER_STATUS, RT_ORDER_COMMISSION);
}

//---------------------------------------------------------------
void IBComClientImpl::position(const std::string &account, const Contract &contract, Decimal position, double avgCost)
{
    CPosition positionObj(QString::fromLocal8Bit(account.data(), static_cast<qint32>(account.size())), contract, position, avgCost);

    qCDebug(IBComClientImplLog(), "acc: %s contract: %s, %s, %s - pos: %f, ac: %f n", positionObj.getAccount().toLocal8Bit().data(),
            positionObj.getContract().symbol.c_str(), positionObj.getContract().secType.c_str(), positionObj.getContract().currency.c_str(),
            positionObj.getPos(), positionObj.getAvgCost());

    m_DispatcherBrokerData.SendMessageToSubscribers(&positionObj, E_RQ_ID_POSITION, RT_REQ_POSITION);
}

//---------------------------------------------------------------
void IBComClientImpl::positionEnd()
{
    qCDebug(IBComClientImplLog(), "Position End\n");
    m_DispatcherBrokerData.SendMessageToSubscribers(nullptr, E_RQ_ID_POSITION, RT_REQ_POSITION);
    qCDebug(IBComClientImplLog(), "Position End TEST!\n");
}

//---------------------------------------------------------------
void IBComClientImpl::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done)
{
    QDateTime timestamp;
    Q_UNUSED(done);

    for (HistoricalTickLast tick : ticks) {
        timestamp.setMSecsSinceEpoch(static_cast<quint32>(tick.time));

        qCDebug(IBComClientImplLog(), "htlast id[%d] t[%s] p[%f] s[%lld] e[%s] sc[%s] unrep[%d] pl[%d] \n",
                //reqId, timestamp.toString(Qt::SystemLocaleShortDate).toLocal8Bit().data(), tick.price, tick.size, tick.exchange.c_str(),
                reqId, timestamp.toString(Qt::ISODateWithMs).toLocal8Bit().data(), tick.price, tick.size, tick.exchange.c_str(),
                tick.specialConditions.c_str(), tick.tickAttribLast.unreported, tick.tickAttribLast.pastLimit);

        /*TODO: What we should do with it?!*/
        CTickByTickAllLast _tickbytick(reqId,
                                       999u, /* some special ID for historical tick*/
                                       tick.time,
                                       tick.price,
                                       static_cast<qint32>(tick.size),
                                       tick.tickAttribLast,
                                       QString(tick.exchange.c_str()),
                                       QString(tick.specialConditions.c_str())
                                       );
        m_DispatcherBrokerData.SendMessageToSubscribers(&_tickbytick, reqId, RT_TICK_BY_TICK_DATA);

    }
}

void IBComClientImpl::tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size, const TickAttribLast &tickAttribLast, const std::string &exchange, const std::string &specialConditions)
{
    QDateTime timestamp;

    CTickByTickAllLast _tickbytick(reqId,
                                   tickType,
                                   time,
                                   price,
                                   size,
                                   tickAttribLast,
                                   QString(exchange.c_str()),
                                   QString(specialConditions.c_str())
                                   );

    timestamp.setMSecsSinceEpoch(static_cast<quint32>(_tickbytick.getTimestamp()));

    qCDebug(IBComClientImplLog(),"Tick-By-Tick. ReqId: %lld, TickType: %s, Time: %s, Price: %g, Size: %d, PastLimit: %d, Unreported: %d, Exchange: %s, SpecialConditions:%s\n",
            _tickbytick.getId(), (_tickbytick.getTickType() == 1 ? "Last" : "AllLast"),
            //timestamp.toString(Qt::LocalTime  Qt::SystemLocaleLongDate).toLocal8Bit().data(),
            timestamp.toString(Qt::ISODateWithMs).toLocal8Bit().data(),
            _tickbytick.getPrice(), _tickbytick.getSize(),
            _tickbytick.getTickAttribLast().pastLimit, _tickbytick.getTickAttribLast().unreported,
            _tickbytick.getExchange().toLocal8Bit().data(),
            _tickbytick.getSpecialConditions().toLocal8Bit().data());

    m_DispatcherBrokerData.SendMessageToSubscribers(&_tickbytick, reqId, RT_TICK_BY_TICK_DATA);
}
//////////////////////////////////////////////////////////////////
// methods
//! [connectack]
void IBComClientImpl::connectAck() {
    if (!m_extraAuth && m_pClient->asyncEConnect())
        m_pClient->startApi();
}
//! [connectack]

void IBComClientImpl::processMessagesAPI()
{

    //if (EINVAL == errno)
    //{
    //    qCDebug(IBComClientImplLog(), "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 0x%X", errno);
    //    errno = 0;
    //}
    
    //m_pReader->checkClient();
    m_osSignal.waitForSignal();
    m_pReader->processMsgs();


}
