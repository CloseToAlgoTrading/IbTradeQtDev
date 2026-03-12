#ifndef TST_LIVE_EXECUTION_WIRING_H
#define TST_LIVE_EXECUTION_WIRING_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>

#include "IBComm/MarketDataRouter.h"
#include "Adapters/IBOrderExecutionAdapter.h"
#include "Adapters/OrderEventBridge.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Ports/IOrderExecutionPort.h"
#include "Ports/IPositionRepositoryPort.h"
#include "Pipeline/Contracts.h"

class MockBrokerAPI : public IBrokerAPI {
    Q_OBJECT
public:
    void processMessagesAPI() override {}
    bool connectAPI(const char*, unsigned int, int) override { return true; }
    bool isConnectedAPI() override { return m_connected; }
    void disconnectAPI() override { m_connected = false; }
    void reqCurrentTimeAPI() override {}
    void reqRealTimeDataAPI(const qint32, reqReadlTimeDataConfigData_t&) override {}
    void cancelRealTimeDataAPI(const qint32) override {}
    void reqHistoricalDataAPI(const reqHistConfigData_t&) override {}
    void cancelHistoricalDataAPI(const qint32) override {}
    void reqRealTimeBarsAPI(const qint32, const QString&) override {}
    void cancelRealTimeBarsAPI(const qint32) override {}
    void reqPositionAPI(const qint32) override {}
    void cancelPositionAPI(const qint32) override {}
    void reqCalculateOptionPriceAPI(const reqCalcOptPriceConfigData_t&) override {}
    void cancelCalculateOptionPriceAPI(const qint32) override {}
    void reqHistoricalTicksAPI(const reqHistTicksConfigData_t&) override {}
    void reqTickByTickDataAPI(const reqTickByTickDataConfigData_t&) override {}
    void cancelTickByTickDataAPI(const qint32) override {}
    void reqAccountSummary() override {}
    void cancelAccountSummary(const qint32) override {}
    qint32 reqPlaceOrderAPI(const QString& symbol, const qint32 quantity, const eOrderAction_t action) override {
        Q_UNUSED(symbol) Q_UNUSED(quantity) Q_UNUSED(action)
        return ++m_nextOrderId;
    }
    void cancelOrderAPI(const qint32 id) override { m_lastCancelledId = id; }
    void reqOpenOrdersAPI() override {}
    void reqAllOpenOrdersAPI() override {}
    void reqAutoOpenOrdersAPI(const bool) override {}
    void reqNextValidIDsAPI(const qint32) override {}
    void reqGlobalCancelAPI() override {}

    bool m_connected = true;
    qint32 m_nextOrderId = 100;
    qint32 m_lastCancelledId = -1;
};

class TestLiveExecutionWiring : public QObject {
    Q_OBJECT

private slots:
    void ibAdapter_placesOrderThroughMockBroker()
    {
        MockBrokerAPI broker;
        IBOrderExecutionAdapter adapter(&broker);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 50;
        intent.timestamp = QDateTime::currentDateTime();

        auto result = adapter.placeOrder(intent);
        QVERIFY(result.has_value());
        QCOMPARE(result->symbol, QString("AAPL"));
        QCOMPARE(result->quantity, 50);
        QCOMPARE(result->orderId, 101);
        QCOMPARE(result->status, QString("Submitted"));
    }

    void ibAdapter_failsWhenDisconnected()
    {
        MockBrokerAPI broker;
        broker.m_connected = false;
        IBOrderExecutionAdapter adapter(&broker);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "MSFT";
        intent.quantity = 10;

        auto result = adapter.placeOrder(intent);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::BrokerConnectionFailed);
    }

    void orderEventBridge_forwardsStatusToAdapter()
    {
        MockBrokerAPI broker;
        IBOrderExecutionAdapter adapter(&broker);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "GOOG";
        intent.quantity = 25;
        auto result = adapter.placeOrder(intent);
        QVERIFY(result.has_value());
        int orderId = result->orderId;

        Adapters::OrderEventBridge bridge(&adapter);
        bridge.onOrderStatus(orderId, "Filled", 25.0, 0.0, 150.0);

        auto statusResult = adapter.getOrderStatus(orderId);
        QVERIFY(statusResult.has_value());
        QCOMPARE(statusResult->status, QString("Filled"));
    }

    void orderEventBridge_forwardsExecDetailsToAdapter()
    {
        MockBrokerAPI broker;
        IBOrderExecutionAdapter adapter(&broker);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "TSLA";
        intent.quantity = 10;
        auto result = adapter.placeOrder(intent);
        QVERIFY(result.has_value());
        int orderId = result->orderId;

        Adapters::OrderEventBridge bridge(&adapter);
        bridge.onExecDetails(orderId, "TSLA", 200.0, 10.0);

        auto statusResult = adapter.getOrderStatus(orderId);
        QVERIFY(statusResult.has_value());
        QCOMPARE(statusResult->status, QString("Filled"));
    }

    void dryRunMode_usesMockExecution()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        Ports::IOrderExecutionPort* execPort = &mockExec;

        Pipeline::ExecutionIntent intent;
        intent.symbol = "SPY";
        intent.quantity = 100;
        intent.timestamp = QDateTime::currentDateTime();

        auto result = execPort->placeOrder(intent);
        QVERIFY(result.has_value());
        QCOMPARE(result->status, QString("Filled"));
        QCOMPARE(mockExec.orderCount(), 1);
    }

    void liveMode_usesIBAdapter()
    {
        MockBrokerAPI broker;
        IBOrderExecutionAdapter liveAdapter(&broker);

        Ports::IOrderExecutionPort* execPort = &liveAdapter;

        Pipeline::ExecutionIntent intent;
        intent.symbol = "QQQ";
        intent.quantity = 50;
        intent.timestamp = QDateTime::currentDateTime();

        auto result = execPort->placeOrder(intent);
        QVERIFY(result.has_value());
        QCOMPARE(result->status, QString("Submitted"));
        QCOMPARE(result->orderId, 101);
    }

    void marketDataRouter_tickSizeUpdatesVolume()
    {
        IBComm::MarketDataRouter router;

        router.onTickPrice(1, "AAPL", 150.0, 151.0);

        auto tick = router.lastPrice("AAPL");
        QCOMPARE(tick.volume, 0.0);

        router.onTickSize(1, "AAPL", 1000000.0);

        tick = router.lastPrice("AAPL");
        QCOMPARE(tick.volume, 1000000.0);
    }

    void marketDataRouter_tickSizeEmitsSignal()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickSizeUpdate);

        router.onTickPrice(1, "MSFT", 300.0, 301.0);
        router.onTickSize(1, "MSFT", 500000.0);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("MSFT"));
        QCOMPARE(spy.at(0).at(1).toDouble(), 500000.0);
    }

    void marketDataRouter_tickSizeIgnoresUnknownSymbol()
    {
        IBComm::MarketDataRouter router;

        router.onTickSize(99, "UNKNOWN", 100.0);

        auto tick = router.lastPrice("UNKNOWN");
        QCOMPARE(tick.volume, 0.0);
    }

    void executionModePortSelection()
    {
        MockBrokerAPI broker;
        IBOrderExecutionAdapter liveAdapter(&broker);
        MockExecutionAdapter mockAdapter;
        MockPositionRepository mockRepo;

        enum class ExecutionMode { DryRun, Live };
        ExecutionMode mode = ExecutionMode::DryRun;

        auto selectPort = [&]() -> Ports::IOrderExecutionPort* {
            return (mode == ExecutionMode::Live) ?
                static_cast<Ports::IOrderExecutionPort*>(&liveAdapter) :
                static_cast<Ports::IOrderExecutionPort*>(&mockAdapter);
        };

        Pipeline::ExecutionIntent intent;
        intent.symbol = "AMZN";
        intent.quantity = 5;
        intent.timestamp = QDateTime::currentDateTime();

        auto* port = selectPort();
        auto result1 = port->placeOrder(intent);
        QVERIFY(result1.has_value());
        QCOMPARE(result1->status, QString("Filled"));
        QCOMPARE(mockAdapter.orderCount(), 1);

        mode = ExecutionMode::Live;
        port = selectPort();
        auto result2 = port->placeOrder(intent);
        QVERIFY(result2.has_value());
        QCOMPARE(result2->status, QString("Submitted"));
        QCOMPARE(result2->orderId, 101);
    }

    void endToEnd_pipelineWithLiveAdapter()
    {
        MockBrokerAPI broker;
        IBOrderExecutionAdapter liveAdapter(&broker);
        Adapters::OrderEventBridge bridge(&liveAdapter);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "META";
        intent.quantity = 30;
        intent.timestamp = QDateTime::currentDateTime();

        auto result = liveAdapter.placeOrder(intent);
        QVERIFY(result.has_value());
        int orderId = result->orderId;
        QCOMPARE(result->status, QString("Submitted"));

        bridge.onOrderStatus(orderId, "PreSubmitted", 0.0, 30.0, 0.0);
        auto s1 = liveAdapter.getOrderStatus(orderId);
        QVERIFY(s1.has_value());
        QCOMPARE(s1->status, QString("PreSubmitted"));

        bridge.onExecDetails(orderId, "META", 350.0, 30.0);
        auto s2 = liveAdapter.getOrderStatus(orderId);
        QVERIFY(s2.has_value());
        QCOMPARE(s2->status, QString("Filled"));
    }
};

#endif // TST_LIVE_EXECUTION_WIRING_H
