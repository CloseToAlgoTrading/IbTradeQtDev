#ifndef CBROKERDATAPROVIDER_H
#define CBROKERDATAPROVIDER_H

#include "./IBComm/IBrokerAPI.h"
#include "./IBComm/Dispatcher.h"
#include <QSharedPointer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dataProviderLog);

using namespace Observer;

class CBrokerDataProvider : public Observer::CDispatcher
{
public:
    CBrokerDataProvider();
    CBrokerDataProvider(QSharedPointer<IBrokerAPI> _pClien);
    ~CBrokerDataProvider();

    QSharedPointer<IBrokerAPI> getClien() const { return m_pClien; }
    void setClien(QSharedPointer<IBrokerAPI> val) { m_pClien = val;}

public: 
    bool reqestHistoricalData(const CSubscriberPtr _pSubscriber, reqHistConfigData_t & _config);

    bool requestHistoricalTicksData(const CSubscriberPtr _pSubscriber, reqHistTicksConfigData_t & _config);

    bool reqestRealTimeData(const CSubscriberPtr _pSubscriber, reqReadlTimeDataConfigData_t &_config);
    bool cancelRealTimeData(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestRealTimeBars(const CSubscriberPtr _pSubscriber, const QString& _symbol);
    bool cancelRealTimeBars(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestPosition(const CSubscriberPtr _pSubscriber, const QString& _symbol);
    bool cancelPosition(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestResetSubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);
    bool cancelResetSubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestOrderStatusSubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);
    bool cancelOrderStatusubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestCalculateOptionPrice(const CSubscriberPtr _pSubscriber, reqCalcOptPriceConfigData_t &_config);
    bool cancelCalculateOptionPrice(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestTickByTickData(const CSubscriberPtr _pSubscriber, const QString& _symbol, reqTickByTickDataConfigData_t & _config);
    bool cancelTickByTickData(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool isConnectedToTheServer();

private:
    QSharedPointer<IBrokerAPI> m_pClien;

    
};

#endif // CBROKERDATAPROVIDER_H
