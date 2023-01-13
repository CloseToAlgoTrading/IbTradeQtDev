#ifndef IBROKERAPI_H
#define IBROKERAPI_H

#include "GlobalDef.h"

enum orderAction
{
    OA_BUY,
    OA_SELL
};

class IBrokerAPI
{
public:
    //virtual ~IBrokerAPI();

	//interface
    virtual void processMessagesAPI() = 0;

	virtual bool connectAPI(const char *host, unsigned int port, int clientId) = 0;
	virtual bool isConnectedAPI() = 0;
	virtual void disconnectAPI() = 0;

	virtual void reqCurrentTimeAPI() = 0;
    virtual void reqRealTimeDataAPI(const qint32 _id, reqReadlTimeDataConfigData_t &_config) = 0;
	virtual void cancelRealTimeDataAPI(const qint32 _id) = 0;

	virtual void reqHistoricalDataAPI(const reqHistConfigData_t & _config) = 0;
	virtual void cancelHistoricalDataAPI(const qint32 id) = 0;

    virtual void reqRealTimeBarsAPI(const qint32 _id, const QString& _symbol) = 0;
    virtual void cancelRealTimeBarsAPI(const qint32 _id) = 0;

    virtual void reqPositionAPI(const qint32 _id) = 0;
    virtual void cancelPositionAPI(const qint32 _id) = 0;

    virtual void reqCalculateOptionPriceAPI(const reqCalcOptPriceConfigData_t & _config) = 0;
    virtual void cancelCalculateOptionPriceAPI(const qint32 _id) = 0;

    virtual void reqHistoricalTicksAPI(const reqHistTicksConfigData_t & _config) = 0;

    virtual void reqTickByTickDataAPI(const reqTickByTickDataConfigData_t & _config) = 0;
    virtual void cancelTickByTickDataAPI(const qint32 id) = 0;





    //orders
    virtual qint32 reqPlaceOrderAPI(const QString& _symbol, const qint32 _quantity, const orderAction _action) = 0;
    virtual void cancelOrderAPI(const qint32 _id) = 0;
    virtual void reqOpenOrdersAPI() = 0;
    virtual void reqAllOpenOrdersAPI() = 0;
    virtual void reqAutoOpenOrdersAPI(const bool _bAutoBind) = 0;
    virtual void reqNextValidIDsAPI(const qint32 _numIds) = 0;
    virtual void reqGlobalCancelAPI() = 0;
    //exerciseOptions()

    //account & portfolio
    //virtual void reqAccountSummary(const qint32 _id, const QString _groups, const QString _tags) = 0;
    //virtual void reqAccountUpdates(const bool _subscribe, const QString _accCode) = 0;
    //virtual void reqAccountUpdatesMulti(const qint32 _id, const QString _account, cosnt QString _modelCode, const bool _ledgerAndNLV) = 0;

};


#endif //IBROKERAPI_H

