#ifndef CBROKERDATAPROVIDER_H
#define CBROKERDATAPROVIDER_H

#include "./IBComm/IBrokerAPI.h"
#include "./IBComm/Dispatcher.h"
#include <QSharedPointer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dataProviderLog);

namespace IBComm { class OrderRouter; class AccountRouter; class PositionRouter; class HistoricalDataRouter; }

using namespace Observer;

class CBrokerDataProvider : public Observer::CDispatcher
{
public:
    CBrokerDataProvider();
    CBrokerDataProvider(QSharedPointer<IBrokerAPI> _pClien);
    virtual ~CBrokerDataProvider() {};

    QSharedPointer<IBrokerAPI> getClien() const { return m_pClien; }
    void setClien(QSharedPointer<IBrokerAPI> val) { m_pClien = val;}

    IBComm::OrderRouter* orderRouter() const { return m_orderRouter; }
    void setOrderRouter(IBComm::OrderRouter* r) { m_orderRouter = r; }

    IBComm::AccountRouter* accountRouter() const { return m_accountRouter; }
    void setAccountRouter(IBComm::AccountRouter* r) { m_accountRouter = r; }

    IBComm::PositionRouter* positionRouter() const { return m_positionRouter; }
    void setPositionRouter(IBComm::PositionRouter* r) { m_positionRouter = r; }

    IBComm::HistoricalDataRouter* historicalDataRouter() const { return m_historicalDataRouter; }
    void setHistoricalDataRouter(IBComm::HistoricalDataRouter* r) { m_historicalDataRouter = r; }

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

    bool requestErrorNotificationSubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);
    bool cancelErrorNotificationSubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestOrderStatusSubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);
    bool cancelOrderStatusubscription(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestCalculateOptionPrice(const CSubscriberPtr _pSubscriber, reqCalcOptPriceConfigData_t &_config);
    bool cancelCalculateOptionPrice(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    bool requestTickByTickData(const CSubscriberPtr _pSubscriber, const QString& _symbol, reqTickByTickDataConfigData_t & _config);
    bool cancelTickByTickData(const CSubscriberPtr _pSubscriber, const QString& _symbol);

    /* Account Information */
    bool bpReqAccountSummary(const CSubscriberPtr _pSubscriber, const QString &_symbol);
    bool bpCancelAccountSummary(const CSubscriberPtr _pSubscriber, const QString &_symbol);

    bool isConnectedToTheServer();


private:
    QSharedPointer<IBrokerAPI> m_pClien;

    IBComm::OrderRouter* m_orderRouter = nullptr;
    IBComm::AccountRouter* m_accountRouter = nullptr;
    IBComm::PositionRouter* m_positionRouter = nullptr;
    IBComm::HistoricalDataRouter* m_historicalDataRouter = nullptr;
};

#endif // CBROKERDATAPROVIDER_H
