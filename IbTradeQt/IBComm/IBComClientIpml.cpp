#include <QDateTime>
#include <QDebug>

#include "IBComClientImpl.h"

//#include "EPosixClientSocket.h"
//#include "EPosixClientSocketPlatform.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"
#include "AccountSummaryTags.h"

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
#include "cexecutionreport.h"
#include "ccommissionreport.h"

#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "bar.h"
#include "NHelper.h"
#include "Decimal.h"
#include "../Pipeline/Contracts.h"

using namespace IBDataTypes;

Q_LOGGING_CATEGORY(IBComClientImplLog, "ibComClientImpl.Callback");


IBComClientImpl::IBComClientImpl()
    : m_osSignal(2000)
    , m_pClient(new EClientSocket(this, &m_osSignal))
    , m_pReader(nullptr)
    , m_extraAuth(false)
    , m_pLog(LOGGER)
    , m_nexValidId(0)
    , m_accountSummaryData()
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


        emit signalServerStateUpdate(true);
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
    emit signalServerStateUpdate(false);
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
    reqHist.strEndDate = _config.endDateTimeUtc.trimmed().isEmpty()
        ? QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HH:mm:ss")
        : _config.endDateTimeUtc.trimmed();

    // Amount of time up to the end date
    reqHist.strDuration = _config.duration;
    // Bar size
    reqHist.strBarSize = _config.barSize;
    reqHist.strWhatToShow = _config.whatToShow.trimmed().isEmpty()
        ? QStringLiteral("TRADES")
        : _config.whatToShow.trimmed();

    Contract m_contract;

    reqHist.m_contract.symbol = _config.symbol.toStdString();
    reqHist.m_contract.secType = _config.secType.trimmed().isEmpty()
        ? "STK"
        : _config.secType.trimmed().toStdString();
    reqHist.m_contract.strike = 0;
    reqHist.m_contract.currency = _config.currency.trimmed().isEmpty()
        ? "USD"
        : _config.currency.trimmed().toStdString();
    reqHist.m_contract.exchange = _config.exchange.trimmed().isEmpty()
        ? "SMART"
        : _config.exchange.trimmed().toStdString();
    reqHist.m_contract.primaryExchange = _config.primaryExchange.trimmed().toStdString();

	TagValueListSPtr mktDataOptions;

    m_pClient->reqHistoricalData(_config.id, reqHist.m_contract,
        reqHist.strEndDate.toStdString(), reqHist.strDuration.toStdString(), reqHist.strBarSize.toStdString(), reqHist.strWhatToShow.toStdString(),
        _config.useRth, _config.formatDate, false, mktDataOptions);
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
    Q_UNUSED(_id)
    m_pClient->reqPositions();
}

//---------------------------------------------------------------
void IBComClientImpl::cancelPositionAPI(const qint32 _id)
{
    Q_UNUSED(_id)
    m_pClient->cancelPositions();
}

//---------------------------------------------------------------
qint32 IBComClientImpl::reqPlaceOrderAPI(const QString& _symbol, const qint32 _quantity, const eOrderAction_t _action)
{
    reqPlaceOrder_t orderToPlace;
    qint32 retOrderId = getNexValidId();
    // fill contract
    orderToPlace.contract.symbol = _symbol.toLocal8Bit().data();
    orderToPlace.contract.secType = "STK";
    orderToPlace.contract.exchange = "SMART";
    //orderToPlace.contract.primaryExchange = "ISLAND";

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
    orderToPlace.order.totalQuantity = DecimalFunctions::doubleToDecimal(_quantity);
    orderToPlace.order.transmit = true;
    orderToPlace.order.orderId = retOrderId;

    m_pClient->placeOrder(retOrderId, orderToPlace.contract, orderToPlace.order);

    return retOrderId;
}

//---------------------------------------------------------------
qint32 IBComClientImpl::reqPlaceLimitOrderAPI(const QString& _symbol, const qint32 _quantity, const eOrderAction_t _action, double limitPrice)
{
    reqPlaceOrder_t orderToPlace;
    qint32 retOrderId = getNexValidId();

    orderToPlace.contract.symbol = _symbol.toLocal8Bit().data();
    orderToPlace.contract.secType = "STK";
    orderToPlace.contract.exchange = "SMART";

    orderToPlace.order.action = (OA_BUY == _action) ? "BUY" : "SELL";
    orderToPlace.order.orderType = "LMT";
    orderToPlace.order.tif = "DAY";
    orderToPlace.order.totalQuantity = DecimalFunctions::doubleToDecimal(_quantity);
    orderToPlace.order.lmtPrice = limitPrice;
    orderToPlace.order.transmit = true;
    orderToPlace.order.orderId = retOrderId;

    m_pClient->placeOrder(retOrderId, orderToPlace.contract, orderToPlace.order);
    return retOrderId;
}

//---------------------------------------------------------------
qint32 IBComClientImpl::reqPlaceStopOrderAPI(const QString& _symbol, const qint32 _quantity, const eOrderAction_t _action, double stopPrice)
{
    reqPlaceOrder_t orderToPlace;
    qint32 retOrderId = getNexValidId();

    orderToPlace.contract.symbol = _symbol.toLocal8Bit().data();
    orderToPlace.contract.secType = "STK";
    orderToPlace.contract.exchange = "SMART";

    orderToPlace.order.action = (OA_BUY == _action) ? "BUY" : "SELL";
    orderToPlace.order.orderType = "STP";
    orderToPlace.order.tif = "DAY";
    orderToPlace.order.totalQuantity = DecimalFunctions::doubleToDecimal(_quantity);
    orderToPlace.order.auxPrice = stopPrice;
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
void IBComClientImpl::reqAccountSummary()
{
    m_pClient->reqAccountSummary(9001, "All", AccountSummaryTags::getAllTags());
}

//---------------------------------------------------------------
void IBComClientImpl::cancelAccountSummary(const qint32 id)
{
    Q_UNUSED(id)
    m_pClient->cancelAccountSummary(9001);
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
    IBDataTypes::CMyTickPrice _tickPrize(tickerId, field, price, attrib.canAutoExecute, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    qCDebug(IBComClientImplLog(), "tickerId = %ld, field = %d, price = %f, canAutoExecute = %d\n", _tickPrize.getId(), _tickPrize.getTickType(), _tickPrize.getPrice(), _tickPrize.getCanAutoExecute());

    if (m_marketDataRouter && m_reqIdToSymbol.contains(tickerId)) {
        // IB TickType: 1=bid, 2=ask, 4=last
        if (field == 1) {
            m_lastBid[tickerId] = price;
        } else if (field == 2) {
            m_lastAsk[tickerId] = price;
        }

        double bid = m_lastBid.value(tickerId, 0.0);
        double ask = m_lastAsk.value(tickerId, 0.0);
        if (bid > 0.0 && ask > 0.0) {
            m_marketDataRouter->onTickPrice(
                tickerId, m_reqIdToSymbol[tickerId], bid, ask);
        }
    }

	return;
};

//---------------------------------------------------------------
void IBComClientImpl::tickSize(TickerId tickerId, TickType field, Decimal size)
{
	//qDebug("tickSize : tickerId = %d, field = %d, size = %d", tickerId, field, size);

    CTickSize _tickSize(tickerId, field, size, QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());

    qCDebug(IBComClientImplLog(), "tickerId = %ld, field = %d, size = %d", _tickSize.getId(), _tickSize.getTickType(), _tickSize.getSize());

    if (m_marketDataRouter && m_reqIdToSymbol.contains(tickerId)) {
        if (field == 8) {
            m_marketDataRouter->onTickSize(
                tickerId, m_reqIdToSymbol[tickerId],
                DecimalFunctions::decimalToDouble(size));
        }
    }

	return;
};

//---------------------------------------------------------------
void IBComClientImpl::tickGeneric(TickerId tickerId, TickType tickType, double value)
{
    if (m_marketDataRouter)
        m_marketDataRouter->onTickGeneric(static_cast<int>(tickerId), static_cast<int>(tickType), value);
}

//---------------------------------------------------------------
void IBComClientImpl::tickString(TickerId tickerId, TickType tickType, const std::string& value)
{
    if (m_marketDataRouter)
        m_marketDataRouter->onTickString(static_cast<int>(tickerId), static_cast<int>(tickType),
                                          QString::fromStdString(value));
}


//---------------------------------------------------------------
//TODO:
//void IBComClientImpl::historicalData(TickerId reqId, const std::string& date, double open, double high,
//    double low, double close, int volume, int barCount, double WAP, int hasGaps)
void IBComClientImpl::historicalData(TickerId reqId, const Bar& bar)
{
    //TODO: Decimal!!
    CHistoricalData _historicalData(reqId, bar.time.c_str(), bar.open, bar.high, bar.low, bar.close, static_cast<int>(DecimalFunctions::decimalToDouble(bar.volume)), bar.count, static_cast<int>(DecimalFunctions::decimalToDouble(bar.wap)), false);

   qCDebug(IBComClientImplLog(), "tickerId = %ld , date = %s, open = %f, high = %f, low =%f, close = %f, volume = %f, barCount = %d, WAP = %f, hasGaps = %d",
           _historicalData.getId(), NHelper::convertQTDataTimeToString(_historicalData.getDateTime()).toStdString().c_str(), _historicalData.getOpen(), _historicalData.getHigh(), _historicalData.getLow(),
           _historicalData.getClose(), _historicalData.getVolume(), _historicalData.getCount(), _historicalData.getWap(), _historicalData.getHasGaps());
    if (m_historicalDataRouter) {
        m_historicalDataRouter->onHistoricalBar(
            reqId, QString::fromStdString(bar.time),
            bar.open, bar.high, bar.low, bar.close,
            DecimalFunctions::decimalToDouble(bar.volume), bar.count);
    }
}

void IBComClientImpl::historicalDataEnd(int reqId, const std::string &startDateStr, const std::string &endDateStr)
{
    if (m_historicalDataRouter) {
        m_historicalDataRouter->onHistoricalDataEnd(reqId);
    }
}


//---------------------------------------------------------------
void IBComClientImpl::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
    Decimal volume, Decimal wap, int count)
{

    CrealtimeBar _realtimeBar(reqId, static_cast<quint64>(time), open, high, low, close, volume, count, wap);

    qCDebug(IBComClientImplLog(), "tickerId = %ld , date = %llu, open = %f, high = %f, low =%f, close = %f, volume = %f, barCount = %d, WAP = %f, ",
		_realtimeBar.getId(), _realtimeBar.getDateTime(), _realtimeBar.getOpen(), _realtimeBar.getHigh(), _realtimeBar.getLow(),
		_realtimeBar.getClose(), _realtimeBar.getVolume(), _realtimeBar.getCount(), _realtimeBar.getWap());

    if (m_marketDataRouter && m_reqIdToSymbol.contains(reqId)) {
        Pipeline::OHLCVBar bar;
        bar.symbol = m_reqIdToSymbol[reqId];
        bar.open = open;
        bar.high = high;
        bar.low = low;
        bar.close = close;
        bar.volume = DecimalFunctions::decimalToDouble(volume);
        bar.timestamp = QDateTime::fromSecsSinceEpoch(time);
        m_marketDataRouter->onOhlcvBarComplete(bar);
    }

    return;
};

//---------------------------------------------------------------
void IBComClientImpl::updateMktDepth(TickerId id, int position, int operation, int side,
    double price, Decimal size)
{
    if (m_marketDepthRouter)
        m_marketDepthRouter->onDepthUpdate(static_cast<int>(id), position, operation, side,
                                            price, DecimalFunctions::decimalToDouble(size));
}

//---------------------------------------------------------------
//TODO:
//void IBComClientImpl::updateMktDepthL2(TickerId id, int position, std::string marketMaker, int operation,
//	int side, double price, int size)
void IBComClientImpl::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
    int side, double price, Decimal size, bool isSmartDepth)
{
    if (m_marketDepthRouter)
        m_marketDepthRouter->onDepthL2Update(static_cast<int>(id), position,
                                              QString::fromStdString(marketMaker),
                                              operation, side, price,
                                              DecimalFunctions::decimalToDouble(size), isSmartDepth);
}



//---------------------------------------------------------------
void IBComClientImpl::tickOptionComputation(TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta,
                                            double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
{
    Q_UNUSED(tickAttrib)
    if (m_marketDataRouter)
        m_marketDataRouter->onTickOptionComputation(static_cast<int>(tickerId), static_cast<int>(tickType),
                                                     impliedVol, delta, optPrice, pvDividend,
                                                     gamma, vega, theta, undPrice);
}


//---------------------------------------------------------------
void IBComClientImpl::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
	double totalDividends, int holdDays, const std::string& futureExpiry, double dividendImpact, double dividendsToExpiry)
{
    qCDebug(IBComClientImplLog) << "tickEFP: tickerId =" << tickerId << "tickType =" << tickType
                                << "basisPoints =" << basisPoints << "formattedBasisPoints ="
                                << QString::fromStdString(formattedBasisPoints) << "totalDividends ="
                                << totalDividends << "holdDays =" << holdDays << "futureExpiry ="
                                << QString::fromStdString(futureExpiry) << "dividendImpact ="
                                << dividendImpact << "dividendsToExpiry =" << dividendsToExpiry;
	return;
};


//---------------------------------------------------------------
void IBComClientImpl::nextValidId(OrderId orderId)
{
    qCDebug(IBComClientImplLog(), "nextValidId = %ld \n", orderId);
    m_nexValidId = orderId;

    if (m_orderRouter) {
        m_orderRouter->onNextValidId(static_cast<int>(orderId));
    }

    return;
}

//---------------------------------------------------------------
void IBComClientImpl::currentTime(long time)
{
    if (m_timeRouter) {
        m_timeRouter->onCurrentTime(time);
    }
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
            || (1102 == errorCode))
    {
        if (m_marketDataRouter)
            m_marketDataRouter->onSubscriptionRestarted();
    }
    else if(200 == errorCode)
    {
        if (m_marketDataRouter)
            m_marketDataRouter->onSubscriptionError(id, errorCode, QString::fromStdString(errorString));
    }
}

//---------------------------------------------------------------
void IBComClientImpl::winError(const std::string& str, int lastError)
{
    qCDebug(IBComClientImplLog(), "lastError=%d, msg=%s\n", lastError, str.c_str());
}

//---------------------------------------------------------------
void IBComClientImpl::updateAccountValue(const std::string &key, const std::string &val, const std::string &currency, const std::string &accountName)
{
    qCDebug(IBComClientImplLog(), "Account Info: key: %s, value: %s, currency: %s, account: %s\n", key.c_str(), val.c_str(), currency.c_str(), accountName.c_str());
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

    COrderStatus orderStatusObj((qint32)orderId, "", QString::fromLocal8Bit(status.data(), status.size()), DecimalFunctions::decimalToDouble(filled), DecimalFunctions::decimalToDouble(remaining), avgFillPrice,
        permId, parentId, lastFillPrice, clientId, QString::fromLocal8Bit(whyHeld.data(), whyHeld.size()), "", OA_BUY);


	qCDebug(IBComClientImplLog(), "orderId = %d, status %s, filled = %d, remaining = %f, avgFillPrice = %f, permId = %d, parentId = %d, lastFillPrice = %f, clientId = %d, whyHeld =%s", 
		orderStatusObj.getId(), orderStatusObj.getStatus().toLocal8Bit().data(), orderStatusObj.getFilled(), orderStatusObj.getRemaining(), orderStatusObj.getAvgFillPrice(), orderStatusObj.getPermId(), 
		orderStatusObj.getParentId(), orderStatusObj.getLastFilledPrice(), orderStatusObj.getClientId(), orderStatusObj.getWhyHeld().toLocal8Bit().data());
        
	
    if (m_orderRouter) {
        m_orderRouter->onOrderStatus(
            static_cast<int>(orderId),
            QString::fromStdString(status),
            DecimalFunctions::decimalToDouble(filled),
            DecimalFunctions::decimalToDouble(remaining),
            avgFillPrice);
    }

	return;
}
//---------------------------------------------------------------
void IBComClientImpl::openOrder(OrderId orderId, const Contract& _contract, const Order& _order, const OrderState& _orderState)
{
    qCDebug(IBComClientImplLog(), "OpenOrder. ID: %ld %s @ %s %s: %s, %s %f %s", orderId, _contract.symbol.c_str(), _contract.secType.c_str(), _contract.exchange.c_str(),
            _order.action.c_str(), _order.orderType.c_str(), DecimalFunctions::decimalToDouble(_order.totalQuantity), _orderState.status.c_str());
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
    qCDebug(IBComClientImplLog(), "ReqId: %d - %s, %s, %s - %s, %ld, %f, %f, %s, %f \n", reqId, contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(),
            execution.execId.c_str(), execution.orderId, DecimalFunctions::DecimalFunctions::decimalToDouble(execution.shares), execution.avgPrice, execution.side.c_str(), execution.price);

    if (m_orderRouter) {
        m_orderRouter->onExecDetails(
            static_cast<int>(execution.orderId),
            QString::fromStdString(contract.symbol),
            execution.avgPrice,
            DecimalFunctions::decimalToDouble(execution.shares),
            QString::fromStdString(execution.execId));
    }
}

//---------------------------------------------------------------
void IBComClientImpl::commissionReport(const CommissionReport& commissionReport)
{
    qCDebug(IBComClientImplLog(), "%s - %f %s RPNL %f\n", commissionReport.execId.c_str(), commissionReport.commission, commissionReport.currency.c_str(), commissionReport.realizedPNL);

    if (m_orderRouter) {
        m_orderRouter->onCommissionReport(
            QString::fromStdString(commissionReport.execId),
            commissionReport.commission,
            QString::fromStdString(commissionReport.currency),
            commissionReport.realizedPNL);
    }
}

//---------------------------------------------------------------
void IBComClientImpl::position(const std::string &account, const Contract &contract, Decimal position, double avgCost)
{
    CPosition positionObj(QString::fromLocal8Bit(account.data(), static_cast<qint32>(account.size())), contract, DecimalFunctions::decimalToDouble(position), avgCost);

    qCDebug(IBComClientImplLog(), "acc: %s contract: %s, %s, %s - pos: %f, ac: %f n", positionObj.getAccount().toLocal8Bit().data(),
            positionObj.getContract().symbol.c_str(), positionObj.getContract().secType.c_str(), positionObj.getContract().currency.c_str(),
            positionObj.getPos(), positionObj.getAvgCost());

    if (m_positionRouter) {
        m_positionRouter->onPosition(
            positionObj.getAccount(),
            QString::fromStdString(contract.symbol),
            DecimalFunctions::decimalToDouble(position),
            avgCost);
    }
}

//---------------------------------------------------------------
void IBComClientImpl::positionEnd()
{
    qCDebug(IBComClientImplLog(), "Position End\n");

    if (m_positionRouter) {
        m_positionRouter->onPositionEnd();
    }
    qCDebug(IBComClientImplLog(), "Position End TEST!\n");
}

//---------------------------------------------------------------
void IBComClientImpl::accountSummary(int reqId, const std::string &account, const std::string &tag, const std::string &value, const std::string &curency)
{
    qCDebug(IBComClientImplLog(), "acc sum: id - %d, %s, [%s : %s], %s", reqId, account.c_str(), tag.c_str(), value.c_str(), curency.c_str());
    if(tag == "AccountType")
    {
        m_accountSummaryData.setAccountType(value.c_str());
    } else if (tag == "BuyingPower") {
        m_accountSummaryData.setBuyingPower(std::stod(value));
    } else if (tag == "TotalCashValue") {
        m_accountSummaryData.setTotalCashValue(std::stod(value));
    } else if (tag == "NetLiquidation") {
        m_accountSummaryData.setNetLiquidation(std::stod(value));
    } else if (tag == "EquityWithLoanValue") {
        m_accountSummaryData.setEquityWithLoanValue(std::stod(value));
    }
    m_accountSummaryData.setAccount(account.c_str());
    m_accountSummaryData.setCurrency(curency.c_str());

    if (m_accountRouter) {
        m_accountRouter->onAccountSummary(
            QString::fromStdString(account),
            QString::fromStdString(tag),
            QString::fromStdString(value),
            QString::fromStdString(curency));
    }
}

//---------------------------------------------------------------
void IBComClientImpl::accountSummaryEnd(int reqId)
{
    qCDebug(IBComClientImplLog(), "Account Summary End - id [%d] \n", reqId);

    if (m_accountRouter) {
        m_accountRouter->onAccountSummaryEnd(reqId);
    }
}

//---------------------------------------------------------------
void IBComClientImpl::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done)
{
    if (m_historicalDataRouter) {
        QVector<IBComm::HistoricalTickLast> converted;
        converted.reserve(static_cast<int>(ticks.size()));
        for (const auto& tick : ticks) {
            IBComm::HistoricalTickLast ht;
            ht.price = tick.price;
            ht.size = static_cast<double>(tick.size);
            ht.time = tick.time;
            ht.exchange = QString::fromStdString(tick.exchange);
            ht.specialConditions = QString::fromStdString(tick.specialConditions);
            converted.append(ht);
        }
        m_historicalDataRouter->onHistoricalTicksLast(reqId, converted, done);
    }
}
//---------------------------------------------------------------
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

    if (m_marketDataRouter && m_reqIdToSymbol.contains(reqId)) {
        QDateTime ts;
        ts.setSecsSinceEpoch(time);
        m_marketDataRouter->onTickByTickAllLast(
            reqId, m_reqIdToSymbol[reqId], price,
            DecimalFunctions::decimalToDouble(size),
            ts, QString::fromStdString(exchange));
    }
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
