#include "cbrokerdataprovider.h"

Q_LOGGING_CATEGORY(dataProviderLog, "dataProvider.General");

CBrokerDataProvider::CBrokerDataProvider()
    : CDispatcher()
    , m_pClien(nullptr)
{

}

CBrokerDataProvider::CBrokerDataProvider(QSharedPointer<IBrokerAPI> _pClien)
    : CDispatcher()
    , m_pClien(_pClien)
{

}

CBrokerDataProvider::~CBrokerDataProvider() = default;

//************************************
// Method:    reqestHistoricalData
// FullName:  CBrokerDataProvider::reqestHistoricalData
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const reqHistConfigData_t & _config
//************************************
bool CBrokerDataProvider::reqestHistoricalData(const CSubscriberPtr _pSubscriber, reqHistConfigData_t & _config)
{
    bool ret = true;
    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (!_config.symbol.isEmpty())
        {
            stReqIds curReq = { 1, RT_HISTORICAL_DATA };

            //----
            if (nullptr != pSubcrItem)
            {
                //curReq.id = pSubcrItem->ReqMenager()->getNextFreeId(_config.symbol, curReq.reqType);
                curReq.id = pSubcrItem->ReqMenager()->getNextFreeId();
            }

            _config.id = curReq.id;
            //request historical data
            Subscribe(_pSubscriber, _config.symbol, curReq);
            getClien()->reqHistoricalDataAPI(_config);
            //----
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    

    return ret;
}

//************************************
// Method:    reqestHistoricalTicksData
// FullName:  CBrokerDataProvider::reqestHistoricalTicksData
// Access:    public
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const reqHistTicksConfigData_t & _config
//************************************
bool CBrokerDataProvider::requestHistoricalTicksData(const CSubscriberPtr _pSubscriber, reqHistTicksConfigData_t &_config)
{
    bool ret = true;
    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (!_config.symbol.isEmpty())
        {
            stReqIds curReq = { 1, RT_HISTORICAL_TICK_DATA };

            //----
            if (nullptr != pSubcrItem)
            {
                //curReq.id = pSubcrItem->ReqMenager()->getNextFreeId(_config.symbol, curReq.reqType);
                curReq.id = pSubcrItem->ReqMenager()->getNextFreeId();
            }

            _config.id = curReq.id;
            //request historical data
            Subscribe(_pSubscriber, _config.symbol, curReq);
            getClien()->reqHistoricalTicksAPI(_config);
            //----
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }


    return ret;
}

//************************************
// Method:    reqestRealTimeData
// FullName:  CBrokerDataProvider::reqestRealTimeData
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::reqestRealTimeData(const CSubscriberPtr _pSubscriber, reqReadlTimeDataConfigData_t &_config)
{
    bool ret = true;
    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());
        QString lsymbol = QString(_config.contract.symbol.c_str());
        if (!lsymbol.isEmpty())
        {
            stReqIds curReq = { 1, RT_REQ_REL_DATA };

            if (nullptr != pSubcrItem)
            {
                curReq.id = pSubcrItem->ReqMenager()->getNextFreeId(lsymbol, curReq.reqType);
            }

            //request data
            Subscribe(_pSubscriber, lsymbol, curReq);
            _config.id = curReq.id;
            getClien()->reqRealTimeDataAPI(curReq.id, _config);
        }
        else
        {
            qCWarning(dataProviderLog(), "Error!_symbol is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

//************************************
// Method:    cancelRealTimeData
// FullName:  CBrokerDataProvider::cancelRealTimeData
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::cancelRealTimeData(const CSubscriberPtr _pSubscriber, const QString& _symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {
        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_REQ_REL_DATA, retData);
            if (ret)
            {
                (void) Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType, _symbol);
                if (true == isReqIdEmpty(_symbol, retData))
                {
                    getClien()->cancelRealTimeDataAPI(retData.id);
                }
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

//************************************
// Method:    requestRealTimeBars
// FullName:  CBrokerDataProvider::requestRealTimeBars
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::requestRealTimeBars(const CSubscriberPtr _pSubscriber, const QString& _symbol)
{
    bool ret = true;

    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (!_symbol.isEmpty())
        {
            stReqIds curReq = { 1, RT_REALTIME_BAR };

            //----
            if (nullptr != pSubcrItem)
            {
                curReq.id = pSubcrItem->ReqMenager()->getNextFreeId(_symbol, curReq.reqType);
            }
            //request historical data
            Subscribe(_pSubscriber, _symbol, curReq);
            getClien()->reqRealTimeBarsAPI(curReq.id, _symbol);
            //----
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

//************************************
// Method:    cancelRealTimeBars
// FullName:  CBrokerDataProvider::cancelRealTimeBars
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::cancelRealTimeBars(const CSubscriberPtr _pSubscriber, const QString& _symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {

        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_REALTIME_BAR, retData);
            if (ret)
            {
                getClien()->cancelRealTimeBarsAPI(retData.id);
                Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType);
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

//************************************
// Method:    requestPosition
// FullName:  CBrokerDataProvider::requestPosition
// Access:    public
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::requestPosition(const CSubscriberPtr _pSubscriber, const QString& _symbol)
{
    bool ret = true;

    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());
        if (!_symbol.isEmpty())
        {
            //Subscribe(_pSubscriber, E_RQ_ID_POSITION, RT_REQ_POSITION);

            stReqIds r = { E_RQ_ID_POSITION, RT_REQ_POSITION };
            Subscribe(_pSubscriber, _symbol, r);

            getClien()->reqPositionAPI(E_RQ_ID_POSITION);
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! _symbol: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

//************************************
// Method:    cancelPosition
// FullName:  CBrokerDataProvider::cancelPosition
// Access:    public
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::cancelPosition(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {

        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_REQ_POSITION, retData);
            if (ret)
            {
                getClien()->cancelPositionAPI(retData.id);
                Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType);
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

bool CBrokerDataProvider::requestResetSubscription(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    //Q_UNUSED(_symbol)
    bool ret = true;

    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());
        //Subscribe(_pSubscriber, E_RQ_ID_RESTART_SUBSCRIPTION, RT_REQ_RESTART_SUBSCRIPTION);
        stReqIds r = { E_RQ_ID_RESTART_SUBSCRIPTION, RT_REQ_RESTART_SUBSCRIPTION };
        Subscribe(_pSubscriber, _symbol, r);
    }
    else
    {
        ret = false;
    }

    return ret;
}

bool CBrokerDataProvider::cancelResetSubscription(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {
        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_REQ_RESTART_SUBSCRIPTION, retData);
            if (ret)
            {
                Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType);
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

bool CBrokerDataProvider::requestOrderStatusSubscription(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    Q_UNUSED(_symbol)
    bool ret = true;

    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());
        //Subscribe(_pSubscriber, E_RQ_ID_ORDER_STATUS, RT_REQ_ORDER_STATUS);
        stReqIds r = { E_RQ_ID_ORDER_STATUS, RT_REQ_ORDER_STATUS };
        Subscribe(_pSubscriber, _symbol, r);
    }
    else
    {
        ret = false;
    }

    return ret;
}

bool CBrokerDataProvider::cancelOrderStatusubscription(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {
        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_REQ_ORDER_STATUS, retData);
            if (ret)
            {
                Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType);
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

//************************************
// Method:    requestCalculateOptionPrice
// FullName:  CBrokerDataProvider::requestCalculateOptionPrice
// Access:    public
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::requestCalculateOptionPrice(const CSubscriberPtr _pSubscriber,
                                                      reqCalcOptPriceConfigData_t &_config)
{
    bool ret = true;
    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (!_config.symbol.isEmpty())
        {
            stReqIds curReq = { 1, RT_REQ_OPTION_PRICE };

            if (nullptr != pSubcrItem)
            {
                curReq.id = pSubcrItem->ReqMenager()->getNextFreeId(_config.symbol, curReq.reqType);
            }

            //request data
            Subscribe(_pSubscriber, _config.symbol, curReq);
            _config.id = curReq.id;
            getClien()->reqCalculateOptionPriceAPI(_config);
        }
        else
        {
            qCWarning(dataProviderLog(), "Error!_symbol is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

//************************************
// Method:    cancelCalculateOptionPrice
// FullName:  CBrokerDataProvider::cancelCalculateOptionPrice
// Access:    public
// Returns:   bool
// Qualifier:
// Parameter: const CSubscriberPtr _pSubscriber
// Parameter: const QString & _symbol
//************************************
bool CBrokerDataProvider::cancelCalculateOptionPrice(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {

        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_REQ_OPTION_PRICE, retData);
            if (ret)
            {
                getClien()->cancelCalculateOptionPriceAPI(retData.id);
                Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType);
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

//_------------------------------------------------------------------------------------
bool CBrokerDataProvider::requestTickByTickData(const CSubscriberPtr _pSubscriber,
                                                const QString& _symbol,
                                                reqTickByTickDataConfigData_t & _config)
{
    bool ret = true;

    if (nullptr != _pSubscriber)
    {
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (!_symbol.isEmpty())
        {
            stReqIds curReq = { 1, RT_TICK_BY_TICK_DATA };

            //----
            if (nullptr != pSubcrItem)
            {
                curReq.id = pSubcrItem->ReqMenager()->getNextFreeId(_symbol, curReq.reqType);
            }
            //request historical data
            Subscribe(_pSubscriber, _symbol, curReq);
            _config.id = curReq.id;
            getClien()->reqTickByTickDataAPI(_config);
            //----
        }
        else
        {
            qCCritical(dataProviderLog(), "Error! _symbol: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
        qCCritical(dataProviderLog(), "Error! subscriber is null");
    }

    return ret;
}

bool CBrokerDataProvider::cancelTickByTickData(const CSubscriberPtr _pSubscriber, const QString &_symbol)
{
    bool ret = false;
    if (nullptr != _pSubscriber)
    {

        stReqIds retData;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_pSubscriber->GetSubscriberId());

        if (nullptr != pSubcrItem)
        {
            ret = pSubcrItem->ReqMenager()->getReqData(_symbol, RT_TICK_BY_TICK_DATA, retData);
            if (ret)
            {
                getClien()->cancelTickByTickDataAPI(retData.id);
                Unsubscribe(_pSubscriber->GetSubscriberId(), retData.id, retData.reqType);
            }
        }
        else
        {
            qCWarning(dataProviderLog(), "Error! pSubcrItem: is Empty string");
            ret = false;
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

bool CBrokerDataProvider::isConnectedToTheServer()
{
    return getClien()->isConnectedAPI();
}
