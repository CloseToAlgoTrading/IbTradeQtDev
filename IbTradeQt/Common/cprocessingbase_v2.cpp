#include "cprocessingbase_v2.h"
#include "IBComm/ProcessingRouterSink.h"
#include <QSharedPointer>
#include <QtConcurrent/QtConcurrentRun>
#include "NHelper.h"

Q_LOGGING_CATEGORY(processingBaseV2Log, "processing.Base");

//----------------------------------------------------------
CProcessingBase_v2::CProcessingBase_v2(QObject *parent)
    : QObject(parent)
    , m_Client(nullptr)
    , m_routerSink(std::make_unique<IBComm::ProcessingRouterSink>(this))
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
    if (m_routerSink)
        m_routerSink->unbind();
    m_Client = newClient;
    if (m_routerSink)
        m_routerSink->bindTo(newClient);
}

qint32 CProcessingBase_v2::getRequestMapSize() const
{
    return m_aciveReqestsMap.size();
}
