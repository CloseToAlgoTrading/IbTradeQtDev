#include "cbrokerdataprovider.h"
#include "IBComm/HistoricalDataRouter.h"

Q_LOGGING_CATEGORY(dataProviderLog, "dataProvider.General");

CBrokerDataProvider::CBrokerDataProvider()
    : m_pClien(nullptr)
{
}

CBrokerDataProvider::CBrokerDataProvider(QSharedPointer<IBrokerAPI> _pClien)
    : m_pClien(_pClien)
{
}

bool CBrokerDataProvider::reqestHistoricalData(reqHistConfigData_t & _config)
{
    if (_config.symbol.isEmpty()) {
        qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
        return false;
    }

    stReqIds curReq = { 1, RT_HISTORICAL_DATA };
    curReq.id = m_reqManager.getNextFreeId();
    m_reqManager.addReqIdsExt(_config.symbol, curReq);

    _config.id = curReq.id;
    if (m_historicalDataRouter)
        m_historicalDataRouter->setReqIdSymbol(curReq.id, _config.symbol);
    getClien()->reqHistoricalDataAPI(_config);
    return true;
}

bool CBrokerDataProvider::cancelHistoricalData(qint32 id)
{
    if (!getClien() || id <= 0)
        return false;
    getClien()->cancelHistoricalDataAPI(id);
    return true;
}

bool CBrokerDataProvider::requestHistoricalTicksData(reqHistTicksConfigData_t &_config)
{
    if (_config.symbol.isEmpty()) {
        qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
        return false;
    }

    stReqIds curReq = { 1, RT_HISTORICAL_TICK_DATA };
    curReq.id = m_reqManager.getNextFreeId();
    m_reqManager.addReqIdsExt(_config.symbol, curReq);

    _config.id = curReq.id;
    getClien()->reqHistoricalTicksAPI(_config);
    return true;
}

bool CBrokerDataProvider::reqestRealTimeData(reqReadlTimeDataConfigData_t &_config)
{
    QString lsymbol = QString(_config.contract.symbol.c_str());
    if (lsymbol.isEmpty()) {
        qCWarning(dataProviderLog(), "Error!_symbol is Empty string");
        return false;
    }

    stReqIds curReq = { 1, RT_REQ_REL_DATA };
    curReq.id = m_reqManager.getNextFreeId(lsymbol, curReq.reqType);
    m_reqManager.addReqIdsExt(lsymbol, curReq);

    _config.id = curReq.id;
    getClien()->registerSymbolForReqId(curReq.id, lsymbol);
    getClien()->reqRealTimeDataAPI(curReq.id, _config);
    return true;
}

bool CBrokerDataProvider::cancelRealTimeData(const QString& _symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_REL_DATA, retData)) {
        m_reqManager.removeReqIdsExt(_symbol, retData);
        getClien()->cancelRealTimeDataAPI(retData.id);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestRealTimeBars(const QString& _symbol)
{
    if (_symbol.isEmpty()) {
        qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
        return false;
    }

    stReqIds curReq = { 1, RT_REALTIME_BAR };
    curReq.id = m_reqManager.getNextFreeId(_symbol, curReq.reqType);
    m_reqManager.addReqIdsExt(_symbol, curReq);

    getClien()->registerSymbolForReqId(curReq.id, _symbol);
    getClien()->reqRealTimeBarsAPI(curReq.id, _symbol);
    return true;
}

bool CBrokerDataProvider::cancelRealTimeBars(const QString& _symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REALTIME_BAR, retData)) {
        getClien()->cancelRealTimeBarsAPI(retData.id);
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestPosition(const QString& _symbol)
{
    if (_symbol.isEmpty()) {
        qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
        return false;
    }

    stReqIds r = { E_RQ_ID_POSITION, RT_REQ_POSITION };
    m_reqManager.addReqIdsExt(_symbol, r);
    getClien()->reqPositionAPI(E_RQ_ID_POSITION);
    return true;
}

bool CBrokerDataProvider::cancelPosition(const QString& _symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_POSITION, retData)) {
        getClien()->cancelPositionAPI(retData.id);
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestResetSubscription(const QString &_symbol)
{
    stReqIds r = { E_RQ_ID_RESTART_SUBSCRIPTION, RT_REQ_RESTART_SUBSCRIPTION };
    m_reqManager.addReqIdsExt(_symbol, r);
    return true;
}

bool CBrokerDataProvider::cancelResetSubscription(const QString &_symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_RESTART_SUBSCRIPTION, retData)) {
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestErrorNotificationSubscription(const QString &_symbol)
{
    stReqIds r = { E_RQ_ID_ERROR_SUBSCRIPTION, RT_REQ_ERROR_SUBSRIPTION };
    m_reqManager.addReqIdsExt(_symbol, r);
    return true;
}

bool CBrokerDataProvider::cancelErrorNotificationSubscription(const QString &_symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_ERROR_SUBSRIPTION, retData)) {
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestOrderStatusSubscription(const QString &_symbol)
{
    Q_UNUSED(_symbol)
    stReqIds r = { E_RQ_ID_ORDER_STATUS, RT_REQ_ORDER_STATUS };
    m_reqManager.addReqIdsExt(_symbol, r);
    return true;
}

bool CBrokerDataProvider::cancelOrderStatusubscription(const QString &_symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_ORDER_STATUS, retData)) {
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestCalculateOptionPrice(reqCalcOptPriceConfigData_t &_config)
{
    if (_config.symbol.isEmpty()) {
        qCWarning(dataProviderLog(), "Error!_symbol is Empty string");
        return false;
    }

    stReqIds curReq = { 1, RT_REQ_OPTION_PRICE };
    curReq.id = m_reqManager.getNextFreeId(_config.symbol, curReq.reqType);
    m_reqManager.addReqIdsExt(_config.symbol, curReq);

    _config.id = curReq.id;
    getClien()->reqCalculateOptionPriceAPI(_config);
    return true;
}

bool CBrokerDataProvider::cancelCalculateOptionPrice(const QString &_symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_OPTION_PRICE, retData)) {
        getClien()->cancelCalculateOptionPriceAPI(retData.id);
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::requestTickByTickData(const QString& _symbol, reqTickByTickDataConfigData_t & _config)
{
    if (_symbol.isEmpty()) {
        qCCritical(dataProviderLog(), "Error! _symbol: is Empty string");
        return false;
    }

    stReqIds curReq = { 1, RT_TICK_BY_TICK_DATA };
    curReq.id = m_reqManager.getNextFreeId(_symbol, curReq.reqType);
    m_reqManager.addReqIdsExt(_symbol, curReq);

    _config.id = curReq.id;
    getClien()->reqTickByTickDataAPI(_config);
    return true;
}

bool CBrokerDataProvider::cancelTickByTickData(const QString &_symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_TICK_BY_TICK_DATA, retData)) {
        getClien()->cancelTickByTickDataAPI(retData.id);
        m_reqManager.removeReqIdsExt(_symbol, retData);
        return true;
    }
    return false;
}

bool CBrokerDataProvider::bpReqAccountSummary(const QString &_symbol)
{
    stReqIds r = { E_RQ_ID_ACCOUNT_SUMMARY, RT_REQ_ACCOUNT_SUMMURY };
    m_reqManager.addReqIdsExt(_symbol, r);
    getClien()->reqAccountSummary();
    return true;
}

bool CBrokerDataProvider::bpCancelAccountSummary(const QString &_symbol)
{
    stReqIds retData;
    if (m_reqManager.getReqData(_symbol, RT_REQ_ACCOUNT_SUMMURY, retData)) {
        getClien()->cancelAccountSummary(retData.id);
        m_reqManager.removeReqIdsExt(_symbol, retData);
    }
    return true;
}

bool CBrokerDataProvider::isConnectedToTheServer()
{
    return getClien()->isConnectedAPI();
}
