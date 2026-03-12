#ifndef TST_PHASE_D_DISPATCHER_REMOVAL_H
#define TST_PHASE_D_DISPATCHER_REMOVAL_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>

#include "IBComm/MarketDataRouter.h"
#include "IBComm/OrderRouter.h"
#include "IBComm/TimeRouter.h"
#include "CObjects/cexecutionreport.h"
#include "CObjects/ccommissionreport.h"

using namespace IBDataTypes;

class TestPhaseD_DispatcherRemoval : public QObject {
    Q_OBJECT

private slots:

    void tickPrice_flowsExclusivelyThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tick);

        router.onTickPrice(1, "AAPL", 150.0, 150.5);
        router.onTickPrice(2, "MSFT", 380.0, 380.5);

        QCOMPARE(spy.count(), 2);

        auto tick1 = spy.at(0).at(0).value<IBComm::MarketTick>();
        QCOMPARE(tick1.symbol, QString("AAPL"));
        QCOMPARE(tick1.bid, 150.0);
        QCOMPARE(tick1.ask, 150.5);

        auto tick2 = spy.at(1).at(0).value<IBComm::MarketTick>();
        QCOMPARE(tick2.symbol, QString("MSFT"));
        QCOMPARE(tick2.bid, 380.0);
    }

    void tickSize_flowsExclusivelyThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickSizeUpdate);

        router.onTickPrice(1, "AAPL", 150.0, 150.5);
        router.onTickSize(1, "AAPL", 5000.0);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("AAPL"));
        QCOMPARE(spy.at(0).at(1).toDouble(), 5000.0);

        auto cached = router.lastPrice("AAPL");
        QCOMPARE(cached.volume, 5000.0);
    }

    void realtimeBar_flowsExclusivelyThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::barClose);

        QDateTime ts = QDateTime(QDate(2026, 3, 4), QTime(10, 30, 0));
        router.onBarComplete(1, "GOOG", ts);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("GOOG"));
        QCOMPARE(spy.at(0).at(1).toDateTime(), ts);
    }

    void tickByTick_flowsExclusivelyThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickByTickTrade);

        QDateTime ts = QDateTime(QDate(2026, 3, 4), QTime(14, 0, 0));
        router.onTickByTickAllLast(1, "TSLA", 250.0, 10.0, ts, "ARCA");

        QCOMPARE(spy.count(), 1);
        auto trade = spy.at(0).at(0).value<IBComm::TickByTickTrade>();
        QCOMPARE(trade.symbol, QString("TSLA"));
        QCOMPARE(trade.price, 250.0);
        QCOMPARE(trade.size, 10.0);
        QCOMPARE(trade.exchange, QString("ARCA"));
    }

    void execution_flowsThroughOrderRouter()
    {
        IBComm::OrderRouter router;
        QSignalSpy spy(&router, &IBComm::OrderRouter::executionReceived);

        router.onExecDetails(42, "AAPL", 150.25, 100.0, "EXEC001");

        QCOMPARE(spy.count(), 1);
        auto received = spy.at(0).at(0).value<IBComm::ExecutionReport>();
        QCOMPARE(received.orderId, 42);
        QCOMPARE(received.symbol, QString("AAPL"));
        QCOMPARE(received.avgPrice, 150.25);
        QCOMPARE(received.shares, 100.0);
        QCOMPARE(received.execId, QString("EXEC001"));
    }

    void commission_flowsThroughOrderRouter()
    {
        IBComm::OrderRouter router;
        QSignalSpy spy(&router, &IBComm::OrderRouter::commissionReceived);

        router.onCommissionReport("EXEC001", 1.50, "USD", 42.0);

        QCOMPARE(spy.count(), 1);
        auto received = spy.at(0).at(0).value<IBComm::CommissionUpdate>();
        QCOMPARE(received.execId, QString("EXEC001"));
        QCOMPARE(received.commission, 1.50);
        QCOMPARE(received.currency, QString("USD"));
        QCOMPARE(received.realizedPnL, 42.0);
    }

    void executionReport_convertsToLegacyType()
    {
        IBComm::ExecutionReport report;
        report.orderId = 7;
        report.symbol = "NVDA";
        report.avgPrice = 900.0;
        report.shares = 50.0;
        report.execId = "EXEC007";

        CExecutionReport legacy(report.orderId, report.symbol,
                                report.avgPrice, report.shares, report.execId);

        QCOMPARE(legacy.getExecId(), QString("EXEC007"));
    }

    void commissionReport_convertsToLegacyType()
    {
        IBComm::CommissionUpdate update;
        update.execId = "EXEC007";
        update.commission = 2.50;
        update.currency = "USD";
        update.realizedPnL = 100.0;

        CCommissionReport legacy(update.execId, update.commission,
                                 update.currency, update.realizedPnL, 0.0, 0);

        QCOMPARE(legacy.getId(), QString("EXEC007"));
        QCOMPARE(legacy.getCommission(), 2.50);
    }

    void timeRouter_deliversCurrentTime()
    {
        IBComm::TimeRouter router;
        QSignalSpy spy(&router, &IBComm::TimeRouter::currentTimeReceived);

        router.onCurrentTime(1709568000L);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toLongLong(), 1709568000LL);
    }

    void subscriptionRestart_flowsThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::subscriptionRestarted);

        router.onSubscriptionRestarted();

        QCOMPARE(spy.count(), 1);
    }

    void subscriptionError_flowsThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::subscriptionError);

        router.onSubscriptionError(42, 200, "No security definition");

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);
        QCOMPARE(spy.at(0).at(1).toInt(), 200);
        QCOMPARE(spy.at(0).at(2).toString(), QString("No security definition"));
    }
};

#endif // TST_PHASE_D_DISPATCHER_REMOVAL_H
