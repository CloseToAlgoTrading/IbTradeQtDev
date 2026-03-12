#ifndef CBROKERDATAPROVIDER_H
#define CBROKERDATAPROVIDER_H

#include "./IBComm/IBrokerAPI.h"
#include "./ReqManager/globalreqmanager.h"
#include <QSharedPointer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dataProviderLog);

namespace IBComm { class OrderRouter; class AccountRouter; class PositionRouter; class HistoricalDataRouter; class TimeRouter; }

class CBrokerDataProvider
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

    IBComm::TimeRouter* timeRouter() const { return m_timeRouter; }
    void setTimeRouter(IBComm::TimeRouter* r) { m_timeRouter = r; }

    GlobalReqManager& reqManager() { return m_reqManager; }

public:
    bool reqestHistoricalData(reqHistConfigData_t & _config);
    bool requestHistoricalTicksData(reqHistTicksConfigData_t & _config);

    bool reqestRealTimeData(reqReadlTimeDataConfigData_t &_config);
    bool cancelRealTimeData(const QString& _symbol);

    bool requestRealTimeBars(const QString& _symbol);
    bool cancelRealTimeBars(const QString& _symbol);

    bool requestPosition(const QString& _symbol);
    bool cancelPosition(const QString& _symbol);

    bool requestResetSubscription(const QString& _symbol);
    bool cancelResetSubscription(const QString& _symbol);

    bool requestErrorNotificationSubscription(const QString& _symbol);
    bool cancelErrorNotificationSubscription(const QString& _symbol);

    bool requestOrderStatusSubscription(const QString& _symbol);
    bool cancelOrderStatusubscription(const QString& _symbol);

    bool requestCalculateOptionPrice(reqCalcOptPriceConfigData_t &_config);
    bool cancelCalculateOptionPrice(const QString& _symbol);

    bool requestTickByTickData(const QString& _symbol, reqTickByTickDataConfigData_t & _config);
    bool cancelTickByTickData(const QString& _symbol);

    bool bpReqAccountSummary(const QString &_symbol);
    bool bpCancelAccountSummary(const QString &_symbol);

    bool isConnectedToTheServer();

private:
    QSharedPointer<IBrokerAPI> m_pClien;
    GlobalReqManager m_reqManager;

    IBComm::OrderRouter* m_orderRouter = nullptr;
    IBComm::AccountRouter* m_accountRouter = nullptr;
    IBComm::PositionRouter* m_positionRouter = nullptr;
    IBComm::HistoricalDataRouter* m_historicalDataRouter = nullptr;
    IBComm::TimeRouter* m_timeRouter = nullptr;
};

#endif // CBROKERDATAPROVIDER_H
