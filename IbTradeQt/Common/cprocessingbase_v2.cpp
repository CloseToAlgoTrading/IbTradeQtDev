#include "cprocessingbase_v2.h"
#include <QSharedPointer>
#include <QtConcurrent/QtConcurrentRun>
#include "NHelper.h"

Q_LOGGING_CATEGORY(processingBaseV2Log, "processing.Base");

//----------------------------------------------------------
CProcessingBase_v2::CProcessingBase_v2(QObject *parent)
    : QObject(parent)
    , m_Client(nullptr)
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
}

//----------------------------------------------------------
CProcessingBase_v2::~CProcessingBase_v2()
{
}

//----------------------------------------------------------
bool CProcessingBase_v2::reqestHistoricalData(reqHistConfigData_t & _config)
{
    return m_Client->reqestHistoricalData(_config);
}

bool CProcessingBase_v2::requestHistoricalTicksData(reqHistTicksConfigData_t &_config)
{
    return m_Client->requestHistoricalTicksData(_config);
}

//----------------------------------------------------------
bool CProcessingBase_v2::reqestRealTimeData(reqReadlTimeDataConfigData_t &_config)
{
    m_aciveReqestsMap.insert(QString(_config.contract.symbol.c_str()), RT_REQ_REL_DATA);
    return m_Client->reqestRealTimeData(_config);
}

//----------------------------------------------------------
bool CProcessingBase_v2::cancelRealTimeData(const QString& _symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_REQ_REL_DATA);
    return m_Client->cancelRealTimeData(_symbol);
}

bool CProcessingBase_v2::requestRealTimeBars(const QString& _symbol)
{
    m_aciveReqestsMap.insert(_symbol, RT_REALTIME_BAR);
    return m_Client->requestRealTimeBars(_symbol);
}

//----------------------------------------------------------
bool CProcessingBase_v2::cancelRealTimeBars(const QString& _symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_REALTIME_BAR);
    return m_Client->cancelRealTimeBars(_symbol);
}

//----------------------------------------------------------
bool CProcessingBase_v2::pbRequestPosition()
{
    m_positionMap.clear();
    return m_Client->requestPosition(PositionSymbol);
}

//----------------------------------------------------------
bool CProcessingBase_v2::pbCancelPosition()
{
    return m_Client->cancelPosition(PositionSymbol);
}

bool CProcessingBase_v2::reqestResetSubscription()
{
    m_aciveReqestsMap.insert(RestartRequestSymbol, RT_REQ_RESTART_SUBSCRIPTION);
    return m_Client->requestResetSubscription(RestartRequestSymbol);
}

bool CProcessingBase_v2::cancelResetSubscription()
{
    m_aciveReqestsMap.remove(RestartRequestSymbol, RT_REQ_RESTART_SUBSCRIPTION);
    return m_Client->cancelResetSubscription(RestartRequestSymbol);
}

bool CProcessingBase_v2::reqestErrorNotificationSubscription()
{
    m_aciveReqestsMap.insert(ErrorSymbol, RT_REQ_ERROR_SUBSRIPTION);
    return m_Client->requestErrorNotificationSubscription(ErrorSymbol);
}

bool CProcessingBase_v2::cancelErrorNotificationSubscription()
{
    m_aciveReqestsMap.remove(ErrorSymbol, RT_REQ_ERROR_SUBSRIPTION);
    return m_Client->cancelErrorNotificationSubscription(ErrorSymbol);
}

bool CProcessingBase_v2::reqestOrderStatusSubscription()
{
    m_aciveReqestsMap.insert(OrderStatusSymbol, RT_REQ_ORDER_STATUS);
    return m_Client->requestOrderStatusSubscription(OrderStatusSymbol);
}

bool CProcessingBase_v2::cancelOrderStatusSubscription()
{
    m_aciveReqestsMap.remove(OrderStatusSymbol, RT_REQ_ORDER_STATUS);
    return m_Client->cancelOrderStatusubscription(OrderStatusSymbol);
}

bool CProcessingBase_v2::requestTickByTickData(reqTickByTickDataConfigData_t &_config)
{
    QString sym = QString(_config.contract.symbol.c_str());
    m_aciveReqestsMap.insert(sym, RT_TICK_BY_TICK_DATA);
    return m_Client->requestTickByTickData(sym, _config);
}

bool CProcessingBase_v2::cancelTickByTickData(const QString &_symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_TICK_BY_TICK_DATA);
    return m_Client->cancelTickByTickData(_symbol);
}

bool CProcessingBase_v2::requestCalculateOptionPrice(reqCalcOptPriceConfigData_t &_config)
{
    m_aciveReqestsMap.insert(QString(_config.contract.symbol.c_str()), RT_REQ_OPTION_PRICE);
    return m_Client->requestCalculateOptionPrice(_config);
}

bool CProcessingBase_v2::cancelCalculateOptionPrice(const QString &_symbol)
{
    m_aciveReqestsMap.remove(_symbol, RT_REQ_OPTION_PRICE);
    return m_Client->cancelCalculateOptionPrice(_symbol);
}

bool CProcessingBase_v2::pbReqAccountSummary()
{
    m_aciveReqestsMap.insert(AccountSummurySymbol, RT_REQ_ACCOUNT_SUMMURY);
    return m_Client->bpReqAccountSummary(AccountSummurySymbol);
}

bool CProcessingBase_v2::pbCancelAccountSummary()
{
    m_aciveReqestsMap.remove(AccountSummurySymbol, RT_REQ_ACCOUNT_SUMMURY);
    return m_Client->bpCancelAccountSummary(AccountSummurySymbol);
}

//----------------------------------------------------------
qint32 CProcessingBase_v2::requestPlaceMarketOrder(const QString& _symbol, const qint32 _quantity, const eOrderAction_t _action)
{
    return m_Client->getClien()->reqPlaceOrderAPI(_symbol, _quantity, _action);
}

//----------------------------------------------------------
void CProcessingBase_v2::requestOpenOrders()
{
    m_Client->getClien()->reqOpenOrdersAPI();
}

//----------------------------------------------------------
void CProcessingBase_v2::cancelAllActiveRequests()
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
            case RT_REQ_REL_DATA:
                m_Client->cancelRealTimeData(symbol);
                break;
            case RT_TICK_BY_TICK_DATA:
                m_Client->cancelTickByTickData(symbol);
                break;
            case RT_REALTIME_BAR:
                m_Client->cancelRealTimeBars(symbol);
                break;
            case RT_REQ_POSITION:
                m_Client->cancelPosition(PositionSymbol);
                break;
            case RT_REQ_OPTION_PRICE:
                m_Client->cancelCalculateOptionPrice(symbol);
                break;
            case RT_REQ_RESTART_SUBSCRIPTION:
                m_Client->cancelResetSubscription(RestartRequestSymbol);
                break;
            case RT_REQ_ORDER_STATUS:
                m_Client->cancelOrderStatusubscription(OrderStatusSymbol);
                break;
            case RT_REQ_ACCOUNT_SUMMURY:
                m_Client->bpCancelAccountSummary(AccountSummurySymbol);
                break;
            default:
                break;
            }
        }
        ++it;
    }

    m_aciveReqestsMap.clear();
}

bool CProcessingBase_v2::isConnectedTotheServer()
{
    if (!m_Client) return false;
    return m_Client->isConnectedToTheServer();
}

void CProcessingBase_v2::callback_recvTickPrize(const CMyTickPrice _tickPrize, const QString &_symbol)
{
    Q_UNUSED(_tickPrize);
    Q_UNUSED(_symbol);
}

void CProcessingBase_v2::calllback_recvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    Q_UNUSED(_histMap);
    Q_UNUSED(_symbol);
}

void CProcessingBase_v2::callback_recvPositionEnd()
{
}

QSharedPointer<CBrokerDataProvider> CProcessingBase_v2::getIBrokerDataProvider() const
{
    return m_Client;
}

void CProcessingBase_v2::setIBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient)
{
    disconnectFromTypedRouters();
    m_Client = newClient;
    connectToTypedRouters();
}

//----------------------------------------------------------
void CProcessingBase_v2::connectToTypedRouters()
{
    if (!m_Client) return;

    if (auto* r = m_Client->orderRouter()) {
        connect(r, &IBComm::OrderRouter::nextValidIdReceived,
                this, &CProcessingBase_v2::slotRouterNextValidId, Qt::QueuedConnection);
        connect(r, &IBComm::OrderRouter::executionReceived,
                this, &CProcessingBase_v2::slotRouterExecution, Qt::QueuedConnection);
        connect(r, &IBComm::OrderRouter::commissionReceived,
                this, &CProcessingBase_v2::slotRouterCommission, Qt::QueuedConnection);
    }
    if (auto* r = m_Client->accountRouter()) {
        connect(r, &IBComm::AccountRouter::accountSummaryUpdated,
                this, &CProcessingBase_v2::slotRouterAccountSummary, Qt::QueuedConnection);
    }
    if (auto* r = m_Client->positionRouter()) {
        connect(r, &IBComm::PositionRouter::positionChanged,
                this, &CProcessingBase_v2::slotRouterPositionChanged, Qt::QueuedConnection);
        connect(r, &IBComm::PositionRouter::positionSnapshotComplete,
                this, &CProcessingBase_v2::slotRouterPositionSnapshotComplete, Qt::QueuedConnection);
    }
    if (auto* r = m_Client->historicalDataRouter()) {
        connect(r, &IBComm::HistoricalDataRouter::barsReceived,
                this, &CProcessingBase_v2::slotRouterBarsReceived, Qt::QueuedConnection);
    }
}

//----------------------------------------------------------
void CProcessingBase_v2::disconnectFromTypedRouters()
{
    if (!m_Client) return;

    if (auto* r = m_Client->orderRouter())
        disconnect(r, nullptr, this, nullptr);
    if (auto* r = m_Client->accountRouter())
        disconnect(r, nullptr, this, nullptr);
    if (auto* r = m_Client->positionRouter())
        disconnect(r, nullptr, this, nullptr);
    if (auto* r = m_Client->historicalDataRouter())
        disconnect(r, nullptr, this, nullptr);
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterNextValidId(int orderId)
{
    setNextValidId(static_cast<qint32>(orderId));
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterAccountSummary(const IBComm::AccountSummaryData& data)
{
    CAccountSummary obj;
    obj.setAccount(data.account);
    obj.setAccountType(data.accountType);
    obj.setCurrency(data.currency);
    obj.setBuyingPower(data.buyingPower);
    obj.setTotalCashValue(data.totalCashValue);
    obj.setNetLiquidation(data.netLiquidation);
    obj.setEquityWithLoanValue(data.equityWithLoanValue);
    emit signalRecvAccountSummary(obj);
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterPositionChanged(const IBComm::PositionUpdate& update)
{
    Contract c;
    c.symbol = update.symbol.toStdString();
    CPosition pos(update.account, c, update.quantity, update.avgCost);
    m_positionMap.insert(update.symbol, pos);
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterPositionSnapshotComplete()
{
    emit signalEndRecvPosition();
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterBarsReceived(int requestId, const QString& symbol,
                                                 const QVector<IBComm::HistoricalBar>& bars)
{
    Q_UNUSED(requestId)
    QList<CHistoricalData> histList;
    for (const auto& bar : bars) {
        CHistoricalData hd(0, "", bar.open, bar.high, bar.low, bar.close,
                           static_cast<int>(bar.volume), bar.count, 0.0, 0, false);
        hd.setDateTime(bar.timestamp.toMSecsSinceEpoch());
        histList.append(hd);
    }
    if (!histList.isEmpty()) {
        histList.last().setIsLast(true);
    }
    emit signalCbkRecvHistoricalData(histList, symbol);
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterExecution(const IBComm::ExecutionReport& report)
{
    CExecutionReport obj(report.orderId, report.symbol, report.avgPrice, report.shares, report.execId);
    emit signalRecvExecutionReport(obj);
}

//----------------------------------------------------------
void CProcessingBase_v2::slotRouterCommission(const IBComm::CommissionUpdate& update)
{
    CCommissionReport obj(update.execId, update.commission, update.currency, update.realizedPnL, 0.0, 0);
    emit signalRecvCommissionReport(obj);
}

qint32 CProcessingBase_v2::getRequestMapSize() const
{
    return m_aciveReqestsMap.size();
}
