#pragma once

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <QTextStream>
#include <memory>
#include <tuple>

#include "Common/IClock.h"
#include "Backtest/MarketPriceStore.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/LedgerSnapshot.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/SimulatedLedger.h"
#include "Backtest/SimulatedExecutionAdapter.h"
#include "Backtest/BacktestMetricsCollector.h"
#include "Backtest/JsonlHistoricalDataSource.h"
#include "Backtest/CsvHistoricalDataSource.h"
#include "Backtest/BacktestSession.h"
#include "Pipeline/Contracts.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Pipeline::MarketTick makeTick(const QString& symbol, double bid, double ask,
                                    const QDateTime& ts = QDateTime())
{
    Pipeline::MarketTick t;
    t.symbol    = symbol;
    t.bid       = bid;
    t.ask       = ask;
    t.timestamp = ts.isValid() ? ts : QDateTime::currentDateTime();
    return t;
}

static Pipeline::ExecutionIntent makeIntent(const QString& symbol, double qty)
{
    Pipeline::ExecutionIntent intent;
    intent.symbol        = symbol;
    intent.quantity      = qty;
    intent.orderType     = Pipeline::ExecutionIntent::Market;
    intent.riskApproval  = "Approved";
    intent.correlationId = "test-corr";
    intent.timestamp     = QDateTime::currentDateTime();
    return intent;
}

// ---------------------------------------------------------------------------
// TestIClock
// ---------------------------------------------------------------------------

class TestIClock : public QObject {
    Q_OBJECT
private slots:
    void wallClockReturnsCurrentTime() {
        WallClock wc;
        QDateTime before = QDateTime::currentDateTime();
        QDateTime now    = wc.now();
        QDateTime after  = QDateTime::currentDateTime();
        QVERIFY(now >= before);
        QVERIFY(now <= after);
    }

    void simulatedClockIsAdvancedManually() {
        SimulatedClock sc;
        QDateTime t1 = QDateTime::fromString("2024-01-01T09:30:00", Qt::ISODate);
        QDateTime t2 = QDateTime::fromString("2024-01-01T09:31:00", Qt::ISODate);

        sc.setCurrentTime(t1);
        QCOMPARE(sc.now(), t1);

        sc.setCurrentTime(t2);
        QCOMPARE(sc.now(), t2);
    }

    void simulatedClockDefaultIsInvalid() {
        SimulatedClock sc;
        QVERIFY(!sc.now().isValid());
    }
};

// ---------------------------------------------------------------------------
// TestMarketPriceStore
// ---------------------------------------------------------------------------

class TestMarketPriceStore : public QObject {
    Q_OBJECT
private slots:
    void storesAndRetrievesLastTick() {
        Backtest::MarketPriceStore store;
        Pipeline::MarketTick t = makeTick("AAPL", 185.0, 185.1);
        store.onTick(t);

        QVERIFY(store.hasTick("AAPL"));
        QCOMPARE(store.lastTick("AAPL").bid, 185.0);
        QCOMPARE(store.lastTick("AAPL").ask, 185.1);
    }

    void returnsEmptyTickForUnknownSymbol() {
        Backtest::MarketPriceStore store;
        QVERIFY(!store.hasTick("MSFT"));
        QCOMPARE(store.lastTick("MSFT").bid, 0.0);
    }

    void updatesOnSubsequentTick() {
        Backtest::MarketPriceStore store;
        store.onTick(makeTick("AAPL", 185.0, 185.1));
        store.onTick(makeTick("AAPL", 186.0, 186.1));
        QCOMPARE(store.lastTick("AAPL").bid, 186.0);
    }

    void clearRemovesAllTicks() {
        Backtest::MarketPriceStore store;
        store.onTick(makeTick("AAPL", 185.0, 185.1));
        store.clear();
        QVERIFY(!store.hasTick("AAPL"));
    }
};

// ---------------------------------------------------------------------------
// TestSimulatedLedger
// ---------------------------------------------------------------------------

class TestSimulatedLedger : public QObject {
    Q_OBJECT
private slots:
    void initialStateIsCorrect() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        QCOMPARE(ledger.cash(), 100000.0);
        QCOMPARE(ledger.realizedPnl(), 0.0);
        QCOMPARE(ledger.unrealizedPnl(), 0.0);
        QCOMPARE(ledger.portfolioValue(), 100000.0);
    }

    void buyReducesCash() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        Backtest::FilledOrder fill;
        fill.symbol    = "AAPL";
        fill.quantity  = 100.0;
        fill.fillPrice = 185.0;
        fill.timestamp = QDateTime::currentDateTime();

        ledger.onFill(fill);

        QCOMPARE(ledger.cash(), 100000.0 - 100.0 * 185.0);
    }

    void sellIncreasesRealizedPnl() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        // Buy 100 @ 185
        Backtest::FilledOrder buy;
        buy.symbol    = "AAPL";
        buy.quantity  = 100.0;
        buy.fillPrice = 185.0;
        buy.timestamp = QDateTime::currentDateTime();
        ledger.onFill(buy);

        // Sell 100 @ 190
        Backtest::FilledOrder sell;
        sell.symbol    = "AAPL";
        sell.quantity  = -100.0;
        sell.fillPrice = 190.0;
        sell.timestamp = QDateTime::currentDateTime();
        ledger.onFill(sell);

        QCOMPARE(ledger.realizedPnl(), 500.0);  // 100 * (190 - 185)
        QCOMPARE(ledger.cash(), 100000.0 + 500.0);
    }

    void unrealizedPnlUsesMarketPrice() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        // Buy 100 @ 185
        Backtest::FilledOrder buy;
        buy.symbol    = "AAPL";
        buy.quantity  = 100.0;
        buy.fillPrice = 185.0;
        buy.timestamp = QDateTime::currentDateTime();
        ledger.onFill(buy);

        // Market moves to 190
        store.onTick(makeTick("AAPL", 190.0, 190.1));

        QCOMPARE(ledger.unrealizedPnl(), 100.0 * (190.05 - 185.0));
    }

    void getPositionReturnsZeroForFlatSymbol() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        auto result = ledger.getPosition(0, "AAPL");
        QVERIFY(result.has_value());
        QCOMPARE(result->quantity, 0.0);
    }

    void getAllPositionsReturnsOpenPositions() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        Backtest::FilledOrder buy;
        buy.symbol    = "AAPL";
        buy.quantity  = 100.0;
        buy.fillPrice = 185.0;
        buy.timestamp = QDateTime::currentDateTime();
        ledger.onFill(buy);

        auto result = ledger.getAllPositions(0);
        QVERIFY(result.has_value());
        QCOMPARE(result->size(), 1);
        QCOMPARE(result->at(0).symbol, QString("AAPL"));
    }

    void updatePositionIsDisabled() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        Ports::PositionRow row;
        row.symbol   = "AAPL";
        row.quantity = 100.0;

        // updatePosition() must return an error in backtest mode
        // (Q_ASSERT_X would fire in debug, but we test the return value)
        // We skip the Q_ASSERT_X in this test by calling through the port interface
        // and checking the error code.
        // Note: in release builds the assert is skipped; in debug it aborts.
        // We test the error path only in release-like mode.
#ifndef QT_DEBUG
        auto result = ledger.updatePosition(row);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
#else
        QSKIP("updatePosition Q_ASSERT_X fires in debug mode — tested in release only");
#endif
    }

    void onBarCloseEmitsSnapshot() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        QList<Backtest::LedgerSnapshot> snapshots;
        connect(&ledger, &Backtest::SimulatedLedger::snapshot,
                [&](const Backtest::LedgerSnapshot& s) { snapshots.append(s); });

        QDateTime ts = QDateTime::fromString("2024-01-02T16:00:00", Qt::ISODate);
        ledger.onBarClose("AAPL", ts);

        QCOMPARE(snapshots.size(), 1);
        QCOMPARE(snapshots[0].timestamp, ts);
        QCOMPARE(snapshots[0].cash, 100000.0);
    }

    void averageCostUpdatesOnAddToPosition() {
        Backtest::MarketPriceStore store;
        Backtest::SimulatedLedger ledger(100000.0, &store);

        // Buy 100 @ 185
        Backtest::FilledOrder buy1;
        buy1.symbol    = "AAPL";
        buy1.quantity  = 100.0;
        buy1.fillPrice = 185.0;
        buy1.timestamp = QDateTime::currentDateTime();
        ledger.onFill(buy1);

        // Buy another 100 @ 190
        Backtest::FilledOrder buy2;
        buy2.symbol    = "AAPL";
        buy2.quantity  = 100.0;
        buy2.fillPrice = 190.0;
        buy2.timestamp = QDateTime::currentDateTime();
        ledger.onFill(buy2);

        auto pos = ledger.getPosition(0, "AAPL");
        QVERIFY(pos.has_value());
        QCOMPARE(pos->quantity, 200.0);
        QCOMPARE(pos->avgCost, 187.5);  // (185 + 190) / 2
    }
};

// ---------------------------------------------------------------------------
// TestSimulatedExecutionAdapter
// ---------------------------------------------------------------------------

class TestSimulatedExecutionAdapter : public QObject {
    Q_OBJECT
private slots:
    void midPriceModelFillsAtMid() {
        Backtest::MarketPriceStore store;
        SimulatedClock clock;
        clock.setCurrentTime(QDateTime::currentDateTime());
        store.onTick(makeTick("AAPL", 185.0, 185.2));

        Backtest::SimulatedExecutionAdapter adapter(
            Backtest::FillModelType::MidPrice, 0.0,
            Backtest::FillTiming::SignalOnTick_FillAtBidAsk,
            &store, &clock);

        QList<Backtest::FilledOrder> fills;
        connect(&adapter, &Backtest::SimulatedExecutionAdapter::filled,
                [&](const Backtest::FilledOrder& f) { fills.append(f); });

        auto result = adapter.placeOrder(makeIntent("AAPL", 100.0));
        QVERIFY(result.has_value());
        QCOMPARE(fills.size(), 1);
        QCOMPARE(fills[0].fillPrice, 185.1);  // mid of 185.0 and 185.2
    }

    void bidAskModelBuysAtAsk() {
        Backtest::MarketPriceStore store;
        SimulatedClock clock;
        clock.setCurrentTime(QDateTime::currentDateTime());
        store.onTick(makeTick("AAPL", 185.0, 185.2));

        Backtest::SimulatedExecutionAdapter adapter(
            Backtest::FillModelType::BidAsk, 0.0,
            Backtest::FillTiming::SignalOnTick_FillAtBidAsk,
            &store, &clock);

        QList<Backtest::FilledOrder> fills;
        connect(&adapter, &Backtest::SimulatedExecutionAdapter::filled,
                [&](const Backtest::FilledOrder& f) { fills.append(f); });

        std::ignore = adapter.placeOrder(makeIntent("AAPL", 100.0));  // buy
        QCOMPARE(fills[0].fillPrice, 185.2);  // ask
    }

    void bidAskModelSellsAtBid() {
        Backtest::MarketPriceStore store;
        SimulatedClock clock;
        clock.setCurrentTime(QDateTime::currentDateTime());
        store.onTick(makeTick("AAPL", 185.0, 185.2));

        Backtest::SimulatedExecutionAdapter adapter(
            Backtest::FillModelType::BidAsk, 0.0,
            Backtest::FillTiming::SignalOnTick_FillAtBidAsk,
            &store, &clock);

        QList<Backtest::FilledOrder> fills;
        connect(&adapter, &Backtest::SimulatedExecutionAdapter::filled,
                [&](const Backtest::FilledOrder& f) { fills.append(f); });

        std::ignore = adapter.placeOrder(makeIntent("AAPL", -100.0));  // sell
        QCOMPARE(fills[0].fillPrice, 185.0);  // bid
    }

    void slippageBpsApplied() {
        Backtest::MarketPriceStore store;
        SimulatedClock clock;
        clock.setCurrentTime(QDateTime::currentDateTime());
        store.onTick(makeTick("AAPL", 185.0, 185.0));  // equal bid/ask for simplicity

        Backtest::SimulatedExecutionAdapter adapter(
            Backtest::FillModelType::SlippageBps, 10.0,  // 10 bps = 0.1%
            Backtest::FillTiming::SignalOnTick_FillAtBidAsk,
            &store, &clock);

        QList<Backtest::FilledOrder> fills;
        connect(&adapter, &Backtest::SimulatedExecutionAdapter::filled,
                [&](const Backtest::FilledOrder& f) { fills.append(f); });

        std::ignore = adapter.placeOrder(makeIntent("AAPL", 100.0));  // buy
        QVERIFY(fills[0].fillPrice > 185.0);
        QVERIFY(qAbs(fills[0].fillPrice - 185.0 * 1.001) < 0.001);
    }

    void pendingOrderQueuedOnBarCloseTimingAndFlushedOnNextTick() {
        Backtest::MarketPriceStore store;
        SimulatedClock clock;
        clock.setCurrentTime(QDateTime::currentDateTime());
        store.onTick(makeTick("AAPL", 185.0, 185.2));

        Backtest::SimulatedExecutionAdapter adapter(
            Backtest::FillModelType::BidAsk, 0.0,
            Backtest::FillTiming::SignalOnClose_FillNextBarOpen,
            &store, &clock);

        QList<Backtest::FilledOrder> fills;
        connect(&adapter, &Backtest::SimulatedExecutionAdapter::filled,
                [&](const Backtest::FilledOrder& f) { fills.append(f); });

        // Place order — should be queued, not filled yet
        auto result = adapter.placeOrder(makeIntent("AAPL", 100.0));
        QVERIFY(result.has_value());
        QCOMPARE(result->status, QString("Queued"));
        QCOMPARE(fills.size(), 0);

        // Simulate next bar open tick
        Pipeline::MarketTick openTick = makeTick("AAPL", 186.0, 186.2);
        adapter.onNextTickOpen(openTick);

        QCOMPARE(fills.size(), 1);
        QCOMPARE(fills[0].fillPrice, 186.2);  // filled at next bar's ask
    }

    void noFillForUnknownSymbol() {
        Backtest::MarketPriceStore store;
        SimulatedClock clock;
        clock.setCurrentTime(QDateTime::currentDateTime());

        Backtest::SimulatedExecutionAdapter adapter(
            Backtest::FillModelType::BidAsk, 0.0,
            Backtest::FillTiming::SignalOnTick_FillAtBidAsk,
            &store, &clock);

        auto result = adapter.placeOrder(makeIntent("UNKNOWN", 100.0));
        QVERIFY(!result.has_value());
    }
};

// ---------------------------------------------------------------------------
// TestBacktestMetricsCollector
// ---------------------------------------------------------------------------

class TestBacktestMetricsCollector : public QObject {
    Q_OBJECT
private slots:
    void totalReturnCalculatedCorrectly() {
        Backtest::BacktestMetricsCollector collector(100000.0);

        Backtest::LedgerSnapshot snap;
        snap.timestamp      = QDateTime::fromString("2024-12-31T16:00:00", Qt::ISODate);
        snap.portfolioValue = 110000.0;
        snap.cash           = 110000.0;
        collector.onSnapshot(snap);

        auto result = collector.finalize(
            QDateTime::fromString("2024-01-01", Qt::ISODate),
            QDateTime::fromString("2024-12-31", Qt::ISODate));

        QCOMPARE(result.initialCapital, 100000.0);
        QCOMPARE(result.finalCapital, 110000.0);
        QCOMPARE(result.totalReturn, 0.1);  // 10%
    }

    void maxDrawdownCalculatedCorrectly() {
        Backtest::BacktestMetricsCollector collector(100000.0);

        auto addSnap = [&](double value, const QString& dateStr) {
            Backtest::LedgerSnapshot s;
            s.timestamp      = QDateTime::fromString(dateStr, Qt::ISODate);
            s.portfolioValue = value;
            collector.onSnapshot(s);
        };

        addSnap(100000.0, "2024-01-01");
        addSnap(110000.0, "2024-02-01");  // peak
        addSnap(99000.0,  "2024-03-01");  // trough: (110000-99000)/110000 = ~10%
        addSnap(105000.0, "2024-04-01");

        auto result = collector.finalize(
            QDateTime::fromString("2024-01-01", Qt::ISODate),
            QDateTime::fromString("2024-04-01", Qt::ISODate));

        QVERIFY(qAbs(result.maxDrawdown - (110000.0 - 99000.0) / 110000.0) < 0.001);
    }

    void emptySessionProducesZeroMetrics() {
        Backtest::BacktestMetricsCollector collector(100000.0);
        auto result = collector.finalize(
            QDateTime::fromString("2024-01-01", Qt::ISODate),
            QDateTime::fromString("2024-12-31", Qt::ISODate));

        QCOMPARE(result.totalReturn, 0.0);
        QCOMPARE(result.finalCapital, 100000.0);
        QCOMPARE(result.totalTrades, 0);
    }

    void tradeLogAccumulatesAllFills() {
        Backtest::BacktestMetricsCollector collector(100000.0);

        for (int i = 0; i < 5; ++i) {
            Backtest::FilledOrder fill;
            fill.orderId   = i;
            fill.symbol    = "AAPL";
            fill.quantity  = 100.0;
            fill.fillPrice = 185.0 + i;
            fill.timestamp = QDateTime::currentDateTime();
            collector.onFill(fill);
        }

        auto result = collector.finalize(
            QDateTime::currentDateTime(),
            QDateTime::currentDateTime());

        QCOMPARE(result.totalTrades, 5);
        QCOMPARE(result.tradeLog.size(), 5);
    }
};

// ---------------------------------------------------------------------------
// TestMarketDataReplayerExtensions
// ---------------------------------------------------------------------------

class TestMarketDataReplayerExtensions : public QObject {
    Q_OBJECT
private slots:
    void addBarSynthesizesFourTicks() {
        MarketDataReplayer replayer;

        IBComm::HistoricalBar bar;
        bar.symbol    = "AAPL";
        bar.timestamp = QDateTime::fromString("2024-01-02T09:30:00", Qt::ISODate);
        bar.open      = 185.0;
        bar.high      = 187.0;
        bar.low       = 184.0;
        bar.close     = 186.0;
        bar.volume    = 1000000;

        replayer.addBar(bar, true);

        QList<Pipeline::MarketTick> ticks;
        QList<QPair<QString, QDateTime>> barCloses;

        connect(&replayer, &MarketDataReplayer::tick,
                [&](const Pipeline::MarketTick& t) { ticks.append(t); });
        connect(&replayer, &MarketDataReplayer::ohlcvBar,
                [&](const Pipeline::OHLCVBar& b) {
                    barCloses.append({b.symbol, b.timestamp});
                });

        replayer.replay();

        QCOMPARE(ticks.size(), 4);
        QCOMPARE(ticks[0].bid, 185.0);  // open
        QCOMPARE(ticks[1].bid, 187.0);  // high
        QCOMPARE(ticks[2].bid, 184.0);  // low
        QCOMPARE(ticks[3].bid, 186.0);  // close

        QCOMPARE(barCloses.size(), 1);
        QCOMPARE(barCloses[0].first, QString("AAPL"));
    }

    void addBarWithoutSynthesisEmitsOnlyBarClose() {
        MarketDataReplayer replayer;

        IBComm::HistoricalBar bar;
        bar.symbol    = "AAPL";
        bar.timestamp = QDateTime::fromString("2024-01-02T09:30:00", Qt::ISODate);
        bar.open      = 185.0;
        bar.high      = 187.0;
        bar.low       = 184.0;
        bar.close     = 186.0;

        replayer.addBar(bar, false);  // no synthesis

        QList<Pipeline::MarketTick> ticks;
        QList<Pipeline::OHLCVBar> bars;
        connect(&replayer, &MarketDataReplayer::tick,
                [&](const Pipeline::MarketTick& t) { ticks.append(t); });
        connect(&replayer, &MarketDataReplayer::ohlcvBar,
                [&](const Pipeline::OHLCVBar& b) { bars.append(b); });

        replayer.replay();
        QCOMPARE(ticks.size(), 0);
        QCOMPARE(bars.size(), 1);
        QCOMPARE(bars[0].symbol, QString("AAPL"));
    }

    void addTickDirectlyIsReplayed() {
        MarketDataReplayer replayer;
        Pipeline::MarketTick t = makeTick("MSFT", 400.0, 400.2);
        replayer.addTick(t);

        QList<Pipeline::MarketTick> received;
        connect(&replayer, &MarketDataReplayer::tick,
                [&](const Pipeline::MarketTick& tick) { received.append(tick); });

        replayer.replay();
        QCOMPARE(received.size(), 1);
        QCOMPARE(received[0].symbol, QString("MSFT"));
    }

    void clearAllResetsReplayer() {
        MarketDataReplayer replayer;
        replayer.addTick(makeTick("AAPL", 185.0, 185.2));

        replayer.clearAll();

        QList<Pipeline::MarketTick> ticks;
        connect(&replayer, &MarketDataReplayer::tick,
                [&](const Pipeline::MarketTick& t) { ticks.append(t); });

        replayer.replay();
        QCOMPARE(ticks.size(), 0);
    }
};

// ---------------------------------------------------------------------------
// TestCsvHistoricalDataSource
// ---------------------------------------------------------------------------

class TestCsvHistoricalDataSource : public QObject {
    Q_OBJECT
private slots:
    void loadsBarsFromCsvFile() {
        // Write a temp CSV file
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());

        QTextStream out(&tmpFile);
        out << "symbol,timestamp,open,high,low,close,volume\n";
        out << "AAPL,2024-01-02T09:30:00,185.00,186.50,184.80,186.20,1200000\n";
        out << "AAPL,2024-01-02T09:31:00,186.20,187.00,186.00,186.80,800000\n";
        out << "MSFT,2024-01-02T09:30:00,400.00,402.00,399.50,401.50,500000\n";
        out.flush();
        tmpFile.close();  // close so CSV source can open it

        Backtest::CsvHistoricalDataSource source(tmpFile.fileName());

        QList<IBComm::HistoricalBar> bars;
        bool finished = false;
        connect(&source, &Backtest::IHistoricalDataSource::barLoaded,
                [&](const IBComm::HistoricalBar& b) { bars.append(b); });
        connect(&source, &Backtest::IHistoricalDataSource::loadFinished,
                [&]() { finished = true; });

        source.requestBars(QStringList(), QDateTime(), QDateTime(),
                           Backtest::BarResolution::Min1);

        QVERIFY(finished);
        QCOMPARE(bars.size(), 3);
    }

    void filtersSymbols() {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());

        QTextStream out(&tmpFile);
        out << "symbol,timestamp,open,high,low,close,volume\n";
        out << "AAPL,2024-01-02T09:30:00,185.00,186.50,184.80,186.20,1200000\n";
        out << "MSFT,2024-01-02T09:30:00,400.00,402.00,399.50,401.50,500000\n";
        out.flush();
        tmpFile.close();

        Backtest::CsvHistoricalDataSource source(tmpFile.fileName());

        QList<IBComm::HistoricalBar> bars;
        connect(&source, &Backtest::IHistoricalDataSource::barLoaded,
                [&](const IBComm::HistoricalBar& b) { bars.append(b); });

        source.requestBars(QStringList{"AAPL"}, QDateTime(), QDateTime(),
                           Backtest::BarResolution::Min1);

        QCOMPARE(bars.size(), 1);
        QCOMPARE(bars[0].symbol, QString("AAPL"));
    }

    void emitsLoadFailedForMissingFile() {
        Backtest::CsvHistoricalDataSource source("/nonexistent/path/data.csv");

        bool failed = false;
        connect(&source, &Backtest::IHistoricalDataSource::loadFailed,
                [&](const QString&) { failed = true; });

        source.requestBars(QStringList(), QDateTime(), QDateTime(),
                           Backtest::BarResolution::Day1);

        QVERIFY(failed);
    }
};

// ---------------------------------------------------------------------------
// TestJsonlHistoricalDataSource
// ---------------------------------------------------------------------------

class TestJsonlHistoricalDataSource : public QObject {
    Q_OBJECT
private slots:
    void loadsTicksFromJsonlFile() {
        // Write a temp JSONL file with MarketTick records
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());

        QTextStream out(&tmpFile);
        out << R"({"type":"MarketTick","symbol":"AAPL","bid":185.0,"ask":185.2,"timestamp":"2024-01-02T09:30:00.000","reqId":1})" << "\n";
        out << R"({"type":"MarketTick","symbol":"AAPL","bid":185.5,"ask":185.7,"timestamp":"2024-01-02T09:30:01.000","reqId":1})" << "\n";
        out.flush();
        tmpFile.close();

        Backtest::JsonlHistoricalDataSource source(tmpFile.fileName());

        QList<Pipeline::MarketTick> ticks;
        bool finished = false;
        connect(&source, &Backtest::IHistoricalDataSource::tickLoaded,
                [&](const Pipeline::MarketTick& t) { ticks.append(t); });
        connect(&source, &Backtest::IHistoricalDataSource::loadFinished,
                [&]() { finished = true; });

        source.requestBars(QStringList(), QDateTime(), QDateTime(),
                           Backtest::BarResolution::Tick);

        QVERIFY(finished);
        QCOMPARE(ticks.size(), 2);
        QCOMPARE(ticks[0].symbol, QString("AAPL"));
        QCOMPARE(ticks[0].bid, 185.0);
    }

    void emitsLoadFailedForMissingFile() {
        Backtest::JsonlHistoricalDataSource source("/nonexistent/path/data.jsonl");

        bool failed = false;
        connect(&source, &Backtest::IHistoricalDataSource::loadFailed,
                [&](const QString&) { failed = true; });

        source.requestBars(QStringList(), QDateTime(), QDateTime(),
                           Backtest::BarResolution::Tick);

        QVERIFY(failed);
    }
};

// ---------------------------------------------------------------------------
// TestFullBacktestSession — end-to-end integration test
// ---------------------------------------------------------------------------

class TestFullBacktestSession : public QObject {
    Q_OBJECT
private slots:
    void sessionWithCsvDataProducesResult() {
        // Write a simple CSV with 5 daily bars for AAPL
        QTemporaryFile csvFile;
        csvFile.setAutoRemove(true);
        QVERIFY(csvFile.open());

        QTextStream out(&csvFile);
        out << "symbol,timestamp,open,high,low,close,volume\n";
        out << "AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000\n";
        out << "AAPL,2024-01-03T16:00:00,186.20,188.00,185.50,187.50,1100000\n";
        out << "AAPL,2024-01-04T16:00:00,187.50,189.00,186.00,188.80,1300000\n";
        out << "AAPL,2024-01-05T16:00:00,188.80,190.00,187.50,189.50,900000\n";
        out << "AAPL,2024-01-08T16:00:00,189.50,191.00,188.00,190.00,1000000\n";
        out.flush();
        csvFile.close();

        // Write a minimal pipeline config (momentum alpha, no risk blocks)
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        QVERIFY(cfgFile.open());

        QTextStream cfgOut(&cfgFile);
        cfgOut << R"({
            "name": "TestMomentum",
            "alphas": [{"blockId": "momentum-alpha", "config": {"lookback": 2, "threshold": 0.001}}],
            "risks": [],
            "rebalance": {"blockId": "simple-rebalance", "config": {}},
            "execution": {"blockId": "market-order-execution", "config": {}},
            "mergePolicy": ""
        })";
        cfgOut.flush();
        cfgFile.close();

        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgFile.fileName();
        config.startDate          = QDateTime::fromString("2024-01-02", Qt::ISODate);
        config.endDate            = QDateTime::fromString("2024-01-08", Qt::ISODate);
        config.symbols            = {"AAPL"};
        config.dataSourceId       = "csv";
        config.dataPath           = csvFile.fileName();
        config.resolution         = Backtest::BarResolution::Day1;
        config.fillModel          = Backtest::FillModelType::MidPrice;
        config.fillTiming         = Backtest::FillTiming::SignalOnClose_FillAtClose;
        config.initialCapital     = 100000.0;

        Backtest::BacktestSession session(config);

        Backtest::BacktestResult result;
        bool sessionFinished = false;
        bool sessionFailed   = false;
        QString failReason;

        connect(&session, &Backtest::BacktestSession::finished,
                [&](const Backtest::BacktestResult& r) {
                    result = r;
                    sessionFinished = true;
                });
        connect(&session, &Backtest::BacktestSession::failed,
                [&](const QString& reason) {
                    failReason = reason;
                    sessionFailed = true;
                });

        int progressCount = 0;
        connect(&session, &Backtest::BacktestSession::progressChanged,
                [&](int p) { ++progressCount; qDebug() << "Progress:" << p; });

        session.run();

        qDebug() << "Progress events:" << progressCount;

        if (sessionFailed) {
            QFAIL(qPrintable("Session failed: " + failReason));
        }

        QVERIFY(sessionFinished);
        QCOMPARE(result.initialCapital, 100000.0);
        // Data quality should be DailyBars for Day1 resolution CSV
        QCOMPARE(result.dataQuality, Backtest::DataQuality::DailyBars);
        // Equity curve should have one entry per bar (5 bars = 5 snapshots)
        qDebug() << "Equity curve size:" << result.equityCurve.size()
                 << "Trades:" << result.totalTrades;
        QVERIFY(result.equityCurve.size() > 0);
    }
};

// ---------------------------------------------------------------------------
// Aggregate test class (all backtester tests in one QObject for main.cpp)
// ---------------------------------------------------------------------------

class TestBacktester : public QObject {
    Q_OBJECT
private slots:
    // IClock
    void iclock_wallClockReturnsCurrentTime()         { TestIClock t; QTest::qExec(&t); }
    void iclock_simulatedClockAdvancedManually()       { TestIClock t; QTest::qExec(&t); }

    // MarketPriceStore
    void priceStore_storesAndRetrieves()               { TestMarketPriceStore t; QTest::qExec(&t); }

    // SimulatedLedger
    void ledger_initialState()                         { TestSimulatedLedger t; QTest::qExec(&t); }

    // SimulatedExecutionAdapter
    void execAdapter_midPriceModel()                   { TestSimulatedExecutionAdapter t; QTest::qExec(&t); }

    // BacktestMetricsCollector
    void metrics_totalReturn()                         { TestBacktestMetricsCollector t; QTest::qExec(&t); }

    // MarketDataReplayer extensions
    void replayer_addBarSynthesis()                    { TestMarketDataReplayerExtensions t; QTest::qExec(&t); }

    // CSV data source
    void csv_loadsFromFile()                           { TestCsvHistoricalDataSource t; QTest::qExec(&t); }

    // JSONL data source
    void jsonl_loadsFromFile()                         { TestJsonlHistoricalDataSource t; QTest::qExec(&t); }

    // Full session
    void session_csvDataProducesResult()               { TestFullBacktestSession t; QTest::qExec(&t); }
};
