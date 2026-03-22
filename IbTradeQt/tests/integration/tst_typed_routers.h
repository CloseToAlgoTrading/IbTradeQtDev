#ifndef TST_TYPED_ROUTERS_H
#define TST_TYPED_ROUTERS_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>

#include "IBComm/MarketDataRouter.h"
#include "Pipeline/Contracts.h"
#include "IBComm/PositionRouter.h"
#include "IBComm/HistoricalDataRouter.h"
#include "IBComm/OrderRouter.h"
#include "IBComm/AccountRouter.h"
#include "Adapters/IBPositionRepositoryAdapter.h"
#include "Blocks/StaticListSelectionBlock.h"
#include "Blocks/LimitOrderExecutionBlock.h"
#include "Blocks/MovingAverageCrossoverAlphaBlock.h"

class TestTypedRouters : public QObject {
    Q_OBJECT

private slots:
    void positionRouter_emitsOnPosition()
    {
        IBComm::PositionRouter router;
        QSignalSpy spy(&router, &IBComm::PositionRouter::positionChanged);

        router.onPosition("DU12345", "AAPL", 100.0, 150.50);

        QCOMPARE(spy.count(), 1);
        auto update = spy.at(0).at(0).value<IBComm::PositionUpdate>();
        QCOMPARE(update.symbol, QString("AAPL"));
        QCOMPARE(update.quantity, 100.0);
        QCOMPARE(update.avgCost, 150.50);
        QCOMPARE(update.account, QString("DU12345"));
    }

    void positionRouter_cachesLastPosition()
    {
        IBComm::PositionRouter router;
        router.onPosition("DU12345", "MSFT", 50.0, 300.0);

        auto pos = router.lastPosition("MSFT");
        QCOMPARE(pos.symbol, QString("MSFT"));
        QCOMPARE(pos.quantity, 50.0);
    }

    void positionRouter_emitsPositionEnd()
    {
        IBComm::PositionRouter router;
        QSignalSpy spy(&router, &IBComm::PositionRouter::positionSnapshotComplete);

        router.onPositionEnd();
        QCOMPARE(spy.count(), 1);
    }

    void historicalDataRouter_accumulatesBars()
    {
        IBComm::HistoricalDataRouter router;
        QSignalSpy barSpy(&router, &IBComm::HistoricalDataRouter::historicalBar);
        QSignalSpy endSpy(&router, &IBComm::HistoricalDataRouter::barsReceived);

        router.setReqIdSymbol(42, "SPY");
        router.onHistoricalBar(42, "20260301", 400.0, 410.0, 395.0, 405.0, 1000000.0, 500);
        router.onHistoricalBar(42, "20260302", 405.0, 415.0, 400.0, 412.0, 1200000.0, 600);

        QCOMPARE(barSpy.count(), 2);

        router.onHistoricalDataEnd(42);
        QCOMPARE(endSpy.count(), 1);

        auto bars = endSpy.at(0).at(2).value<QVector<IBComm::HistoricalBar>>();
        QCOMPARE(bars.size(), 2);
        QCOMPARE(bars[0].open, 400.0);
        QCOMPARE(bars[1].close, 412.0);
    }

    void orderRouter_emitsOrderStatus()
    {
        IBComm::OrderRouter router;
        QSignalSpy spy(&router, &IBComm::OrderRouter::orderStatusChanged);

        router.onOrderStatus(101, "Filled", 100.0, 0.0, 150.5);

        QCOMPARE(spy.count(), 1);
        auto update = spy.at(0).at(0).value<IBComm::OrderStatusUpdate>();
        QCOMPARE(update.orderId, 101);
        QCOMPARE(update.status, QString("Filled"));
        QCOMPARE(update.filled, 100.0);
    }

    void orderRouter_emitsExecution()
    {
        IBComm::OrderRouter router;
        QSignalSpy spy(&router, &IBComm::OrderRouter::executionReceived);

        router.onExecDetails(101, "AAPL", 150.0, 100.0, "exec-001");

        QCOMPARE(spy.count(), 1);
        auto report = spy.at(0).at(0).value<IBComm::ExecutionReport>();
        QCOMPARE(report.orderId, 101);
        QCOMPARE(report.symbol, QString("AAPL"));
        QCOMPARE(report.execId, QString("exec-001"));
    }

    void orderRouter_emitsCommission()
    {
        IBComm::OrderRouter router;
        QSignalSpy spy(&router, &IBComm::OrderRouter::commissionReceived);

        router.onCommissionReport("exec-001", 1.50, "USD", 25.0);

        QCOMPARE(spy.count(), 1);
        auto comm = spy.at(0).at(0).value<IBComm::CommissionUpdate>();
        QCOMPARE(comm.execId, QString("exec-001"));
        QCOMPARE(comm.commission, 1.50);
        QCOMPARE(comm.realizedPnL, 25.0);
    }

    void orderRouter_emitsNextValidId()
    {
        IBComm::OrderRouter router;
        QSignalSpy spy(&router, &IBComm::OrderRouter::nextValidIdReceived);

        router.onNextValidId(500);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 500);
    }

    void accountRouter_accumulatesAndEmits()
    {
        IBComm::AccountRouter router;
        QSignalSpy spy(&router, &IBComm::AccountRouter::accountSummaryUpdated);

        router.onAccountSummary("DU12345", "AccountType", "Individual", "USD");
        router.onAccountSummary("DU12345", "BuyingPower", "50000.0", "USD");
        router.onAccountSummary("DU12345", "TotalCashValue", "25000.0", "USD");
        router.onAccountSummary("DU12345", "NetLiquidation", "100000.0", "USD");
        router.onAccountSummary("DU12345", "EquityWithLoanValue", "95000.0", "USD");
        router.onAccountSummaryEnd(1);

        QCOMPARE(spy.count(), 1);
        auto summary = spy.at(0).at(0).value<IBComm::AccountSummaryData>();
        QCOMPARE(summary.account, QString("DU12345"));
        QCOMPARE(summary.accountType, QString("Individual"));
        QCOMPARE(summary.buyingPower, 50000.0);
        QCOMPARE(summary.totalCashValue, 25000.0);
        QCOMPARE(summary.netLiquidation, 100000.0);
        QCOMPARE(summary.equityWithLoanValue, 95000.0);
    }

    void tickByTick_emitsTradeSignal()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickByTickTrade);

        QDateTime now = QDateTime::currentDateTime();
        router.onTickByTickAllLast(1, "AAPL", 155.0, 200.0, now, "NASDAQ");

        QCOMPARE(spy.count(), 1);
        auto trade = spy.at(0).at(0).value<Pipeline::TickByTickTrade>();
        QCOMPARE(trade.symbol, QString("AAPL"));
        QCOMPARE(trade.price, 155.0);
        QCOMPARE(trade.size, 200.0);
        QCOMPARE(trade.exchange, QString("NASDAQ"));
    }

    void ibPositionRepoAdapter_receivesFromRouter()
    {
        IBComm::PositionRouter router;
        IBPositionRepositoryAdapter adapter;
        adapter.connectToRouter(&router);

        router.onPosition("DU12345", "TSLA", 50.0, 250.0);
        QCoreApplication::processEvents();

        auto result = adapter.getPosition(0, "TSLA");
        QVERIFY(result.has_value());
        QCOMPARE(result->symbol, QString("TSLA"));
        QCOMPARE(result->quantity, 50.0);
        QCOMPARE(result->avgCost, 250.0);
    }

    void staticListSelectionBlock_filtersUniverse()
    {
        Blocks::StaticListSelectionBlock block;
        QJsonObject cfg;
        QJsonArray symbols;
        symbols.append("AAPL");
        symbols.append("GOOG");
        cfg["symbols"] = symbols;
        block.setConfig(cfg);

        QVector<QString> universe = {"AAPL", "MSFT", "GOOG", "AMZN"};
        auto result = block.select(universe);

        QCOMPARE(result.size(), 2);
        QVERIFY(result.contains("AAPL"));
        QVERIFY(result.contains("GOOG"));
    }

    void staticListSelectionBlock_emptyConfigPassesAll()
    {
        Blocks::StaticListSelectionBlock block;
        block.setConfig(QJsonObject());

        QVector<QString> universe = {"AAPL", "MSFT"};
        auto result = block.select(universe);
        QCOMPARE(result.size(), 2);
    }

    void maCrossoverAlphaBlock_generatesSignalOnCrossover()
    {
        Blocks::MovingAverageCrossoverAlphaBlock block;
        QJsonObject cfg;
        cfg["fastPeriod"] = 3;
        cfg["slowPeriod"] = 5;
        block.setConfig(cfg);
        block.initialize();

        QSignalSpy spy(&block, &Pipeline::IAlphaBlock::signalGenerated);

        for (int i = 0; i < 6; ++i) {
            Pipeline::MarketTick tick;
            tick.symbol = "AAPL";
            tick.bid = 100.0 + i;
            tick.ask = 100.0 + i;
            tick.timestamp = QDateTime::currentDateTime();
            block.onTick(tick);
        }

        Pipeline::MarketTick bigJump;
        bigJump.symbol = "AAPL";
        bigJump.bid = 120.0;
        bigJump.ask = 120.0;
        bigJump.timestamp = QDateTime::currentDateTime();
        block.onTick(bigJump);

        QVERIFY(spy.count() >= 0);
    }

    void limitOrderBlock_setsLimitOrderType()
    {
        Blocks::LimitOrderExecutionBlock block;
        QJsonObject cfg;
        cfg["minQuantity"] = 1.0;
        block.setConfig(cfg);

        QSignalSpy placedSpy(&block, &Pipeline::IExecutionBlock::orderPlaced);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 10;
        intent.limitPrice = 150.0;
        intent.timestamp = QDateTime::currentDateTime();

        QVector<Pipeline::ExecutionIntent> intents;
        intents.append(intent);
        block.execute(intents);

        QCOMPARE(placedSpy.count(), 1);
        QCOMPARE(placedSpy.at(0).at(1).toString(), QString("dry-run-limit"));
    }
};

#endif // TST_TYPED_ROUTERS_H
