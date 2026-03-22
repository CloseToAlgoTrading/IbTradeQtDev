#ifndef TST_ADAPTERS_H
#define TST_ADAPTERS_H

#include <QtTest>
#include "Pipeline/Contracts.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Testing/MockMarketDataRouter.h"

class TestAdapters : public QObject
{
    Q_OBJECT

private slots:
    // --- MockExecutionAdapter ---
    void mockExec_placeOrderSuccess()
    {
        MockExecutionAdapter exec;
        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 100.0;
        intent.orderType = Pipeline::ExecutionIntent::Market;

        auto result = exec.placeOrder(intent);
        QVERIFY(result.has_value());
        QCOMPARE(result->symbol, QString("AAPL"));
        QCOMPARE(result->quantity, 100.0);
        QCOMPARE(result->status, QString("Filled"));
        QCOMPARE(result->orderId, 1);
        QCOMPARE(exec.orderCount(), 1);
    }

    void mockExec_placeOrderFail()
    {
        MockExecutionAdapter exec;
        exec.setFailOnPlace(ErrorCode::BrokerConnectionFailed, "No connection");

        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 100.0;

        auto result = exec.placeOrder(intent);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::BrokerConnectionFailed);
        QCOMPARE(exec.orderCount(), 0);
    }

    void mockExec_cancelOrder()
    {
        MockExecutionAdapter exec;
        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 50.0;

        auto placed = exec.placeOrder(intent);
        QVERIFY(placed.has_value());

        auto cancelResult = exec.cancelOrder(placed->orderId);
        QVERIFY(cancelResult.has_value());
        QCOMPARE(exec.cancelledOrderIds().size(), 1);

        auto status = exec.getOrderStatus(placed->orderId);
        QVERIFY(status.has_value());
        QCOMPARE(status->status, QString("Cancelled"));
    }

    void mockExec_cancelNonexistentOrder()
    {
        MockExecutionAdapter exec;
        auto result = exec.cancelOrder(999);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::NotFound);
    }

    void mockExec_getOrderStatus()
    {
        MockExecutionAdapter exec;
        Pipeline::ExecutionIntent intent;
        intent.symbol = "MSFT";
        intent.quantity = -25.0;

        auto placed = exec.placeOrder(intent);
        auto status = exec.getOrderStatus(placed->orderId);
        QVERIFY(status.has_value());
        QCOMPARE(status->symbol, QString("MSFT"));
    }

    void mockExec_getStatusNotFound()
    {
        MockExecutionAdapter exec;
        auto result = exec.getOrderStatus(999);
        QVERIFY(!result.has_value());
    }

    void mockExec_reset()
    {
        MockExecutionAdapter exec;
        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 10.0;
        exec.placeOrder(intent);

        exec.reset();
        QCOMPARE(exec.orderCount(), 0);
        QVERIFY(exec.cancelledOrderIds().isEmpty());
    }

    void mockExec_multipleOrders()
    {
        MockExecutionAdapter exec;
        for (int i = 0; i < 5; ++i) {
            Pipeline::ExecutionIntent intent;
            intent.symbol = QString("SYM%1").arg(i);
            intent.quantity = 10.0 * (i + 1);
            auto r = exec.placeOrder(intent);
            QVERIFY(r.has_value());
            QCOMPARE(r->orderId, i + 1);
        }
        QCOMPARE(exec.orderCount(), 5);
    }

    // --- MockPositionRepository ---
    void mockRepo_getPositionNotFound()
    {
        MockPositionRepository repo;
        auto result = repo.getPosition(1, "AAPL");
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::NotFound);
    }

    void mockRepo_setAndGetPosition()
    {
        MockPositionRepository repo;
        repo.setPosition(1, "AAPL", 100.0, 150.0);

        auto result = repo.getPosition(1, "AAPL");
        QVERIFY(result.has_value());
        QCOMPARE(result->symbol, QString("AAPL"));
        QCOMPARE(result->quantity, 100.0);
        QCOMPARE(result->avgCost, 150.0);
    }

    void mockRepo_getAllPositions()
    {
        MockPositionRepository repo;
        repo.setPosition(1, "AAPL", 100.0);
        repo.setPosition(1, "MSFT", 50.0);
        repo.setPosition(2, "GOOG", 25.0);

        auto result = repo.getAllPositions(1);
        QVERIFY(result.has_value());
        QCOMPARE(result->size(), 2);
    }

    void mockRepo_updatePosition()
    {
        MockPositionRepository repo;
        Ports::PositionRow row;
        row.strategyId = 1;
        row.symbol = "AAPL";
        row.quantity = 200.0;
        row.avgCost = 155.0;

        auto updateResult = repo.updatePosition(row);
        QVERIFY(updateResult.has_value());

        auto getResult = repo.getPosition(1, "AAPL");
        QVERIFY(getResult.has_value());
        QCOMPARE(getResult->quantity, 200.0);
    }

    void mockRepo_reset()
    {
        MockPositionRepository repo;
        repo.setPosition(1, "AAPL", 100.0);
        repo.reset();
        QCOMPARE(repo.positionCount(), 0);
    }

    // --- MockMarketDataRouter ---
    void mockRouter_emitsTick()
    {
        MockMarketDataRouter router;
        qRegisterMetaType<Pipeline::MarketTick>("Pipeline::MarketTick");
        QSignalSpy spy(&router, &MockMarketDataRouter::tick);

        router.simulateTick("AAPL", 149.0, 150.0);

        QCOMPARE(spy.count(), 1);
        auto t = spy.at(0).at(0).value<Pipeline::MarketTick>();
        QCOMPARE(t.symbol, QString("AAPL"));
        QCOMPARE(t.bid, 149.0);
        QCOMPARE(t.ask, 150.0);
    }

    void mockRouter_emitsBarClose()
    {
        MockMarketDataRouter router;
        qRegisterMetaType<Pipeline::OHLCVBar>("Pipeline::OHLCVBar");
        QSignalSpy spy(&router, &MockMarketDataRouter::ohlcvBar);

        QDateTime now = QDateTime::currentDateTimeUtc();
        router.simulateBarClose("AAPL", now);

        QCOMPARE(spy.count(), 1);
        auto b = spy.at(0).at(0).value<Pipeline::OHLCVBar>();
        QCOMPARE(b.symbol, QString("AAPL"));
        QCOMPARE(b.timestamp, now);
    }

    void mockRouter_replayTicks()
    {
        MockMarketDataRouter router;
        qRegisterMetaType<Pipeline::MarketTick>("Pipeline::MarketTick");
        QSignalSpy spy(&router, &MockMarketDataRouter::tick);

        QVector<Pipeline::MarketTick> ticks;
        for (int i = 0; i < 3; ++i) {
            Pipeline::MarketTick t;
            t.symbol = "AAPL"; t.bid = 100.0 + i; t.ask = 100.5 + i;
            ticks.append(t);
        }

        router.replayTicks(ticks);
        QCOMPARE(spy.count(), 3);
        QCOMPARE(router.emittedTicks().size(), 3);
    }

    void mockRouter_reset()
    {
        MockMarketDataRouter router;
        router.simulateTick("AAPL", 149.0, 150.0);
        QCOMPARE(router.emittedTicks().size(), 1);
        router.reset();
        QCOMPARE(router.emittedTicks().size(), 0);
    }

    void mockRouter_customTimestamp()
    {
        MockMarketDataRouter router;
        qRegisterMetaType<Pipeline::MarketTick>("Pipeline::MarketTick");
        QSignalSpy spy(&router, &MockMarketDataRouter::tick);

        QDateTime ts(QDate(2026, 1, 15), QTime(12, 0, 0), QTimeZone::utc());
        router.simulateTick("AAPL", 150.0, 151.0, ts);

        auto t = spy.at(0).at(0).value<Pipeline::MarketTick>();
        QCOMPARE(t.timestamp, ts);
    }
};

#endif // TST_ADAPTERS_H
