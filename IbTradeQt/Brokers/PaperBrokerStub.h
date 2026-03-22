#pragma once

#include "IBrokerAPI.h"

#include <QObject>

namespace Brokers {

/// Non-IB placeholder implementing IBrokerAPI for local/paper flows or tests.
/// No live connection; no-op or stubbed API calls.
class PaperBrokerStub : public IBrokerAPI {
    Q_OBJECT
public:
    explicit PaperBrokerStub(QObject* parent = nullptr);

    void processMessagesAPI() override;

    bool connectAPI(const char* host, unsigned int port, int clientId) override;
    bool isConnectedAPI() override;
    void disconnectAPI() override;

    void reqCurrentTimeAPI() override;
    void reqRealTimeDataAPI(qint32 _id, reqReadlTimeDataConfigData_t& _config) override;
    void cancelRealTimeDataAPI(qint32 _id) override;

    void reqHistoricalDataAPI(const reqHistConfigData_t& _config) override;
    void cancelHistoricalDataAPI(qint32 id) override;

    void reqRealTimeBarsAPI(qint32 _id, const QString& _symbol) override;
    void cancelRealTimeBarsAPI(qint32 _id) override;

    void reqPositionAPI(qint32 _id) override;
    void cancelPositionAPI(qint32 _id) override;

    void reqCalculateOptionPriceAPI(const reqCalcOptPriceConfigData_t& _config) override;
    void cancelCalculateOptionPriceAPI(qint32 _id) override;

    void reqHistoricalTicksAPI(const reqHistTicksConfigData_t& _config) override;

    void reqTickByTickDataAPI(const reqTickByTickDataConfigData_t& _config) override;
    void cancelTickByTickDataAPI(qint32 id) override;

    void reqAccountSummary() override;
    void cancelAccountSummary(qint32 id) override;

    qint32 reqPlaceOrderAPI(const QString& _symbol, qint32 _quantity, eOrderAction_t _action) override;
    void cancelOrderAPI(qint32 _id) override;
    void reqOpenOrdersAPI() override;
    void reqAllOpenOrdersAPI() override;
    void reqAutoOpenOrdersAPI(bool _bAutoBind) override;
    void reqNextValidIDsAPI(qint32 _numIds) override;
    void reqGlobalCancelAPI() override;
};

} // namespace Brokers
