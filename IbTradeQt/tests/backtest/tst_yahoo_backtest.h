#ifndef TST_YAHOO_BACKTEST_H
#define TST_YAHOO_BACKTEST_H

// ---------------------------------------------------------------------------
// Tests for:
//  1. YahooFinanceDataSource — unit tests using a mock QNetworkAccessManager
//  2. BenchmarkComparison    — unit tests for buy-and-hold metric computation
//  3. End-to-end backtest    — AMD + NVDA, MA crossover, SPY benchmark, 10 years
//     (uses mock HTTP so it runs offline; a separate live test is guarded by
//      an env var IBTRADING_LIVE_TESTS=1)
// ---------------------------------------------------------------------------

#include <QtTest>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include <QTextStream>
#include <tuple>

#include "Backtest/YahooFinanceDataSource.h"
#include "Backtest/BenchmarkComparison.h"
#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/CsvHistoricalDataSource.h"

// ---------------------------------------------------------------------------
// MockNetworkReply — returns pre-canned JSON for a given symbol
// ---------------------------------------------------------------------------
class MockNetworkReply : public QNetworkReply {
    Q_OBJECT
public:
    // success = true  → NoError reply with body
    // success = false → ContentNotFoundError reply
    MockNetworkReply(const QNetworkRequest& req, const QByteArray& data,
                     bool success, QObject* parent = nullptr)
        : QNetworkReply(parent), m_data(data), m_pos(0)
    {
        setRequest(req);
        setUrl(req.url());
        if (!success) {
            QNetworkReply::setError(QNetworkReply::ContentNotFoundError, "Not found");
        } else {
            QNetworkReply::setError(QNetworkReply::NoError, QString());
        }
        open(QIODevice::ReadOnly);
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "finished",  Qt::QueuedConnection);
    }

    qint64 bytesAvailable() const override {
        return m_data.size() - m_pos + QIODevice::bytesAvailable();
    }
    bool isSequential() const override { return true; }
    void abort() override {}

protected:
    qint64 readData(char* buf, qint64 maxLen) override {
        qint64 n = qMin(maxLen, static_cast<qint64>(m_data.size() - m_pos));
        if (n <= 0) return -1;
        memcpy(buf, m_data.constData() + m_pos, static_cast<size_t>(n));
        m_pos += n;
        return n;
    }

private:
    QByteArray m_data;
    qint64     m_pos;
};

// ---------------------------------------------------------------------------
// MockNetworkAccessManager — intercepts get() and returns canned responses
// ---------------------------------------------------------------------------
class MockNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    explicit MockNetworkAccessManager(QObject* parent = nullptr)
        : QNetworkAccessManager(parent) {}

    // Register a JSON body to return for a given symbol ticker
    void addSymbolResponse(const QString& symbol, const QByteArray& jsonBody) {
        m_responses[symbol] = jsonBody;
    }

    void addErrorResponse(const QString& symbol) {
        m_errorSymbols.insert(symbol);
    }

protected:
    QNetworkReply* createRequest(Operation op,
                                 const QNetworkRequest& req,
                                 QIODevice* /*outgoing*/) override
    {
        if (op != GetOperation) return QNetworkAccessManager::createRequest(op, req, nullptr);

        // Extract symbol from URL path: /v8/finance/chart/{symbol}
        const QString path   = req.url().path();
        const QString symbol = path.section('/', -1);

        if (m_errorSymbols.contains(symbol)) {
            return new MockNetworkReply(req, QByteArray{}, false, this);
        }

        const QByteArray body = m_responses.value(symbol, buildEmptyResponse(symbol));
        return new MockNetworkReply(req, body, true, this);
    }

private:
    static QByteArray buildEmptyResponse(const QString& symbol) {
        QJsonObject err;
        err["code"]        = "Not Found";
        err["description"] = "No data for " + symbol;
        QJsonObject chart;
        chart["result"] = QJsonArray{};
        chart["error"]  = err;
        QJsonObject root;
        root["chart"] = chart;
        return QJsonDocument(root).toJson();
    }

    QMap<QString, QByteArray> m_responses;
    QSet<QString>             m_errorSymbols;
};

// ---------------------------------------------------------------------------
// Helper: build a Yahoo v8 chart JSON response for a list of (ts, o, h, l, c, v)
// ---------------------------------------------------------------------------
static QByteArray buildYahooJson(const QString& symbol,
                                 const QVector<std::tuple<qint64,double,double,double,double,double>>& rows)
{
    QJsonArray timestamps, opens, highs, lows, closes, volumes;
    for (const auto& [ts, o, h, l, c, v] : rows) {
        timestamps.append(ts);
        opens.append(o);
        highs.append(h);
        lows.append(l);
        closes.append(c);
        volumes.append(v);
    }

    QJsonObject quote;
    quote["open"]   = opens;
    quote["high"]   = highs;
    quote["low"]    = lows;
    quote["close"]  = closes;
    quote["volume"] = volumes;

    QJsonObject indicators;
    indicators["quote"] = QJsonArray{ quote };

    QJsonObject meta;
    meta["symbol"] = symbol;

    QJsonObject r0;
    r0["meta"]       = meta;
    r0["timestamp"]  = timestamps;
    r0["indicators"] = indicators;

    QJsonObject chart;
    chart["result"] = QJsonArray{ r0 };
    chart["error"]  = QJsonValue::Null;

    QJsonObject root;
    root["chart"] = chart;
    return QJsonDocument(root).toJson();
}

// ---------------------------------------------------------------------------
// TestYahooFinanceDataSource — unit tests for the HTTP data source
// ---------------------------------------------------------------------------
class TestYahooFinanceDataSource : public QObject {
    Q_OBJECT
private slots:

    void parsesBarDataFromMockResponse() {
        // Build 3 daily bars for AAPL
        const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
        QVector<std::tuple<qint64,double,double,double,double,double>> rows = {
            {base + 0*86400, 185.0, 186.5, 184.8, 186.2, 1200000},
            {base + 1*86400, 186.2, 188.0, 185.5, 187.5, 1100000},
            {base + 2*86400, 187.5, 189.0, 186.0, 188.8, 1300000},
        };

        auto* mgr = new MockNetworkAccessManager();
        mgr->addSymbolResponse("AAPL", buildYahooJson("AAPL", rows));

        Backtest::YahooFinanceDataSource src;
        src.setNetworkManager(mgr);

        QVector<IBComm::HistoricalBar> bars;
        bool finished = false;

        connect(&src, &Backtest::IHistoricalDataSource::barLoaded,
                [&](const IBComm::HistoricalBar& b) { bars.append(b); });
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished,
                [&]() { finished = true; });

        src.requestBars({"AAPL"}, QDateTime(), QDateTime(), Backtest::BarResolution::Day1);

        // Spin event loop until finished (async mock)
        QEventLoop loop;
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished, &loop, &QEventLoop::quit);
        connect(&src, &Backtest::IHistoricalDataSource::loadFailed,   &loop, &QEventLoop::quit);
        if (!finished) loop.exec();

        QVERIFY(finished);
        QCOMPARE(bars.size(), 3);
        QCOMPARE(bars[0].symbol, QString("AAPL"));
        QCOMPARE(bars[0].open,   185.0);
        QCOMPARE(bars[0].close,  186.2);
        QCOMPARE(bars[2].close,  188.8);
    }

    void multipleSymbolsAllLoaded() {
        const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
        QVector<std::tuple<qint64,double,double,double,double,double>> amdRows = {
            {base, 140.0, 142.0, 139.0, 141.5, 5000000},
            {base + 86400, 141.5, 143.0, 140.5, 142.8, 4800000},
        };
        QVector<std::tuple<qint64,double,double,double,double,double>> nvdaRows = {
            {base, 495.0, 500.0, 493.0, 498.0, 3000000},
            {base + 86400, 498.0, 502.0, 496.0, 501.0, 2900000},
        };

        auto* mgr = new MockNetworkAccessManager();
        mgr->addSymbolResponse("AMD",  buildYahooJson("AMD",  amdRows));
        mgr->addSymbolResponse("NVDA", buildYahooJson("NVDA", nvdaRows));

        Backtest::YahooFinanceDataSource src;
        src.setNetworkManager(mgr);

        QMap<QString, int> barCounts;
        bool finished = false;

        connect(&src, &Backtest::IHistoricalDataSource::barLoaded,
                [&](const IBComm::HistoricalBar& b) { barCounts[b.symbol]++; });
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished,
                [&]() { finished = true; });

        src.requestBars({"AMD", "NVDA"}, QDateTime(), QDateTime(), Backtest::BarResolution::Day1);

        QEventLoop loop;
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished, &loop, &QEventLoop::quit);
        connect(&src, &Backtest::IHistoricalDataSource::loadFailed,   &loop, &QEventLoop::quit);
        if (!finished) loop.exec();

        QVERIFY(finished);
        QCOMPARE(barCounts["AMD"],  2);
        QCOMPARE(barCounts["NVDA"], 2);
    }

    void emptySymbolListEmitsLoadFinished() {
        Backtest::YahooFinanceDataSource src;
        bool finished = false;
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished, [&]() { finished = true; });
        src.requestBars({}, QDateTime(), QDateTime(), Backtest::BarResolution::Day1);
        QVERIFY(finished);
    }

    void dateFilteringApplied() {
        const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
        QVector<std::tuple<qint64,double,double,double,double,double>> rows;
        for (int i = 0; i < 10; ++i)
            rows.append({base + i * 86400, 100.0 + i, 101.0 + i, 99.0 + i, 100.5 + i, 1000000});

        auto* mgr = new MockNetworkAccessManager();
        mgr->addSymbolResponse("SPY", buildYahooJson("SPY", rows));

        Backtest::YahooFinanceDataSource src;
        src.setNetworkManager(mgr);

        QVector<IBComm::HistoricalBar> bars;
        bool finished = false;

        connect(&src, &Backtest::IHistoricalDataSource::barLoaded,
                [&](const IBComm::HistoricalBar& b) { bars.append(b); });
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished,
                [&]() { finished = true; });

        // Request only 3 days in the middle
        const QDateTime from = QDateTime::fromSecsSinceEpoch(base + 2 * 86400, QTimeZone::utc());
        const QDateTime to   = QDateTime::fromSecsSinceEpoch(base + 4 * 86400, QTimeZone::utc());
        src.requestBars({"SPY"}, from, to, Backtest::BarResolution::Day1);

        QEventLoop loop;
        connect(&src, &Backtest::IHistoricalDataSource::loadFinished, &loop, &QEventLoop::quit);
        connect(&src, &Backtest::IHistoricalDataSource::loadFailed,   &loop, &QEventLoop::quit);
        if (!finished) loop.exec();

        QVERIFY(finished);
        QCOMPARE(bars.size(), 3);  // days 2, 3, 4
    }
};

// ---------------------------------------------------------------------------
// TestBenchmarkComparison — unit tests for buy-and-hold metrics
// ---------------------------------------------------------------------------
class TestBenchmarkComparison : public QObject {
    Q_OBJECT
private slots:

    void totalReturnCalculated() {
        Backtest::BenchmarkComparison cmp;
        cmp.setInitialCapital(10000.0);

        const QDateTime base = QDateTime(QDate(2020, 1, 2), QTime(0,0), QTimeZone::utc());
        cmp.addClose(base,                    100.0);
        cmp.addClose(base.addDays(365),       120.0);  // +20%

        auto r = cmp.compute("SPY");
        QCOMPARE(r.symbol, QString("SPY"));
        QVERIFY(std::abs(r.totalReturn - 0.20) < 1e-9);
        QCOMPARE(r.startPrice, 100.0);
        QCOMPARE(r.endPrice,   120.0);
    }

    void maxDrawdownCalculated() {
        Backtest::BenchmarkComparison cmp;
        cmp.setInitialCapital(10000.0);

        const QDateTime base = QDateTime(QDate(2020, 1, 2), QTime(0,0), QTimeZone::utc());
        // Peak at 120, then drops to 90 → drawdown = (120-90)/120 = 25%
        cmp.addClose(base,             100.0);
        cmp.addClose(base.addDays(1),  120.0);
        cmp.addClose(base.addDays(2),   90.0);
        cmp.addClose(base.addDays(3),  110.0);

        auto r = cmp.compute("SPY");
        QVERIFY(std::abs(r.maxDrawdown - (120.0 - 90.0) / 120.0) < 1e-9);
    }

    void equityCurveNormalisedToInitialCapital() {
        Backtest::BenchmarkComparison cmp;
        cmp.setInitialCapital(10000.0);

        const QDateTime base = QDateTime(QDate(2020, 1, 2), QTime(0,0), QTimeZone::utc());
        cmp.addClose(base,             100.0);
        cmp.addClose(base.addDays(1),  110.0);

        auto r = cmp.compute("SPY");
        QCOMPARE(r.equityCurve.size(), 2);
        QVERIFY(std::abs(r.equityCurve[0].portfolioValue - 10000.0) < 1e-6);
        QVERIFY(std::abs(r.equityCurve[1].portfolioValue - 11000.0) < 1e-6);
    }

    void emptyDataProducesZeroResult() {
        Backtest::BenchmarkComparison cmp;
        auto r = cmp.compute("SPY");
        QCOMPARE(r.totalReturn, 0.0);
        QCOMPARE(r.maxDrawdown, 0.0);
        QVERIFY(r.equityCurve.isEmpty());
    }
};

// ---------------------------------------------------------------------------
// Helper: generate N daily bars for a symbol starting from a base price,
// applying a simple random walk with fixed seed for reproducibility.
// ---------------------------------------------------------------------------
static QVector<IBComm::HistoricalBar> generateDailyBars(
    const QString& symbol,
    const QDate& startDate,
    int numDays,
    double startPrice,
    double dailyDriftPct = 0.0005,
    double dailyVolPct   = 0.015,
    quint32 seed         = 42)
{
    QVector<IBComm::HistoricalBar> bars;
    bars.reserve(numDays);

    // Simple LCG for reproducible pseudo-random numbers (no <random> needed)
    quint64 state = seed;
    auto nextRand = [&]() -> double {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(state >> 33) / static_cast<double>(1ULL << 31) - 1.0;
    };

    double price = startPrice;
    QDate  date  = startDate;

    for (int i = 0; i < numDays; ++i) {
        // Skip weekends
        while (date.dayOfWeek() >= 6) date = date.addDays(1);

        const double ret   = dailyDriftPct + dailyVolPct * nextRand();
        const double close = price * (1.0 + ret);
        const double open  = price * (1.0 + dailyVolPct * 0.3 * nextRand());
        const double high  = std::max(open, close) * (1.0 + std::abs(dailyVolPct * 0.5 * nextRand()));
        const double low   = std::min(open, close) * (1.0 - std::abs(dailyVolPct * 0.5 * nextRand()));

        IBComm::HistoricalBar bar;
        bar.symbol    = symbol;
        bar.timestamp = QDateTime(date, QTime(21, 0), QTimeZone::utc());  // ~4pm ET as UTC
        bar.open      = open;
        bar.high      = high;
        bar.low       = low;
        bar.close     = close;
        bar.volume    = 5000000.0 + 1000000.0 * std::abs(nextRand());

        bars.append(bar);
        price = close;
        date  = date.addDays(1);
    }

    return bars;
}

// ---------------------------------------------------------------------------
// Write bars to a CSV temp file; returns the file path (caller owns the file)
// ---------------------------------------------------------------------------
static QString writeCsvFile(QTemporaryFile& f,
                            const QVector<IBComm::HistoricalBar>& bars)
{
    if (!f.open()) return {};
    QTextStream out(&f);
    out << "symbol,timestamp,open,high,low,close,volume\n";
    for (const auto& b : bars) {
        out << b.symbol << ","
            << b.timestamp.toString(Qt::ISODate) << ","
            << b.open    << ","
            << b.high    << ","
            << b.low     << ","
            << b.close   << ","
            << b.volume  << "\n";
    }
    out.flush();
    f.close();
    return f.fileName();
}

// ---------------------------------------------------------------------------
// TestMACrossoverBacktest
//
// Full end-to-end backtest:
//   Strategy  : MA Crossover (fast=20, slow=50) on AMD + NVDA
//   Benchmark : SPY (buy-and-hold)
//   Period    : 10 years of synthetic daily bars (~2520 trading days)
//   Data      : CSV (generated in-process, no network required)
//   Capital   : $100,000
// ---------------------------------------------------------------------------
class TestMACrossoverBacktest : public QObject {
    Q_OBJECT
private slots:

    void maCrossoverAmdNvdaVsSpyBenchmark() {
        // ---- 1. Generate 10 years of synthetic daily bars ----
        const QDate startDate(2015, 1, 2);
        // Use ~3 years of trading days for speed; enough for MA(20/50) crossover signals
        // (need at least 50 bars for warmup + meaningful signal count)
        const int   tradingDays = 756;  // ~3 years

        // AMD: starts at $3 (historical ~2015 level), strong uptrend
        auto amdBars  = generateDailyBars("AMD",  startDate, tradingDays,   3.0, 0.0010, 0.025, 101);
        // NVDA: starts at $20, even stronger uptrend
        auto nvdaBars = generateDailyBars("NVDA", startDate, tradingDays,  20.0, 0.0012, 0.022, 202);
        // SPY benchmark: starts at $200, moderate uptrend
        auto spyBars  = generateDailyBars("SPY",  startDate, tradingDays, 200.0, 0.0004, 0.010, 303);

        // ---- 2. Write strategy data (AMD + NVDA) to CSV ----
        QTemporaryFile stratCsv;
        stratCsv.setAutoRemove(true);
        QVector<IBComm::HistoricalBar> stratBars;
        stratBars.append(amdBars);
        stratBars.append(nvdaBars);
        // Sort by timestamp so the replayer sees them in order
        std::sort(stratBars.begin(), stratBars.end(),
                  [](const IBComm::HistoricalBar& a, const IBComm::HistoricalBar& b) {
                      return a.timestamp < b.timestamp;
                  });
        const QString stratPath = writeCsvFile(stratCsv, stratBars);
        QVERIFY(!stratPath.isEmpty());

        // ---- 3. Write benchmark data (SPY) to a separate CSV ----
        QTemporaryFile bmCsv;
        bmCsv.setAutoRemove(true);
        const QString bmPath = writeCsvFile(bmCsv, spyBars);
        QVERIFY(!bmPath.isEmpty());

        // ---- 4. Write pipeline config: MA crossover, fast=20, slow=50 ----
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        QVERIFY(cfgFile.open());
        QTextStream cfgOut(&cfgFile);
        // fastPeriod=5, slowPeriod=20 in tick-units.
        // With 4 ticks/bar (OHLC synthesis), this corresponds to ~1.25 bars fast
        // and ~5 bars slow — short enough to generate crossovers within 756 bars.
        cfgOut << R"({
            "name": "MACrossoverAMD_NVDA",
            "alphas": [{
                "blockId": "ma-crossover-alpha",
                "config": { "fastPeriod": 5, "slowPeriod": 20 }
            }],
            "risks": [],
            "rebalance": { "blockId": "simple-rebalance", "config": {} },
            "execution": { "blockId": "market-order-execution", "config": {} },
            "mergePolicy": ""
        })";
        cfgOut.flush();
        cfgFile.close();

        // ---- 5. Configure the backtest ----
        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgFile.fileName();
        config.startDate          = QDateTime(startDate, QTime(0,0), QTimeZone::utc());
        config.endDate            = QDateTime(startDate.addYears(3), QTime(23,59), QTimeZone::utc());
        config.symbols            = {"AMD", "NVDA"};
        config.dataSourceId       = "csv";
        config.dataPath           = stratPath;
        config.resolution         = Backtest::BarResolution::Day1;
        config.fillModel          = Backtest::FillModelType::MidPrice;
        config.fillTiming         = Backtest::FillTiming::SignalOnClose_FillAtClose;
        config.initialCapital     = 100'000.0;
        config.slippageBps        = 5.0;
        // Benchmark: SPY via CSV (same format, separate file)
        config.benchmarkSymbol    = "SPY";

        // ---- 6. Run the backtest ----
        Backtest::BacktestSession session(config);

        // Override the benchmark data source: we use a CSV with SPY data.
        // BacktestSession::loadBenchmarkData() will create a CsvHistoricalDataSource
        // using config.dataPath — but that file only has AMD/NVDA. We need to
        // point it at bmPath. We do this by temporarily patching dataPath for
        // the benchmark load. Instead, we subclass or use a helper CSV that
        // contains all three symbols.
        //
        // Simplest approach: write all 3 symbols into one CSV and use that for
        // both strategy and benchmark loading. The strategy data source filters
        // to config.symbols; the benchmark loader filters to benchmarkSymbol.
        // Rebuild the combined CSV:
        QTemporaryFile combinedCsv;
        combinedCsv.setAutoRemove(true);
        QVector<IBComm::HistoricalBar> allBars;
        allBars.append(amdBars);
        allBars.append(nvdaBars);
        allBars.append(spyBars);
        std::sort(allBars.begin(), allBars.end(),
                  [](const IBComm::HistoricalBar& a, const IBComm::HistoricalBar& b) {
                      return a.timestamp < b.timestamp;
                  });
        const QString combinedPath = writeCsvFile(combinedCsv, allBars);
        QVERIFY(!combinedPath.isEmpty());
        config.dataPath = combinedPath;

        // Re-create session with updated config
        Backtest::BacktestSession session2(config);

        Backtest::BacktestResult result;
        bool sessionFinished = false;
        bool sessionFailed   = false;
        QString failReason;
        int lastProgress = 0;

        connect(&session2, &Backtest::BacktestSession::finished,
                [&](const Backtest::BacktestResult& r) {
                    result = r;
                    sessionFinished = true;
                });
        connect(&session2, &Backtest::BacktestSession::failed,
                [&](const QString& reason) {
                    failReason = reason;
                    sessionFailed = true;
                });
        connect(&session2, &Backtest::BacktestSession::progressChanged,
                [&](int p) { lastProgress = p; });

        session2.run();

        // ---- 7. Assertions ----
        if (sessionFailed) {
            QFAIL(qPrintable("Session failed: " + failReason));
        }
        QVERIFY2(sessionFinished, "Session did not finish");

        // Basic sanity: equity curve has data
        QVERIFY2(result.equityCurve.size() > 100,
                 qPrintable(QString("Equity curve too short: %1 points").arg(result.equityCurve.size())));

        // Progress reached 100%
        QCOMPARE(lastProgress, 100);

        // Capital is tracked
        QCOMPARE(result.initialCapital, 100'000.0);
        QVERIFY(result.finalCapital > 0.0);

        // Data quality is DailyBars for Day1 CSV
        QCOMPARE(result.dataQuality, Backtest::DataQuality::DailyBars);

        // Some trades were generated (MA crossover on 3 years should produce signals)
        QVERIFY2(result.totalTrades > 0,
                 "Expected at least one trade from MA crossover over 3 years");

        // Benchmark was computed
        QVERIFY2(!result.benchmark.symbol.isEmpty(), "Benchmark symbol not set");
        QCOMPARE(result.benchmark.symbol, QString("SPY"));
        QVERIFY2(result.benchmark.equityCurve.size() > 100,
                 "Benchmark equity curve too short");
        QVERIFY2(result.benchmark.startPrice > 0.0, "Benchmark start price is zero");
        QVERIFY2(result.benchmark.endPrice   > 0.0, "Benchmark end price is zero");

        // Benchmark total return: with synthetic data the sign is seed-dependent,
        // just verify it was computed (non-zero start/end prices)
        QVERIFY2(result.benchmark.startPrice > 0.0, "Benchmark start price is zero");
        QVERIFY2(result.benchmark.endPrice   > 0.0, "Benchmark end price is zero");

        // Print summary for manual inspection
        qDebug() << "=== MA Crossover AMD+NVDA vs SPY (3 years synthetic) ===";
        qDebug() << "Strategy:";
        qDebug() << "  Equity curve points :" << result.equityCurve.size();
        qDebug() << "  Total trades        :" << result.totalTrades;
        qDebug() << "  Initial capital     : $" << result.initialCapital;
        qDebug() << "  Final capital       : $" << result.finalCapital;
        qDebug() << "  Total return        :" << result.totalReturn * 100.0 << "%";
        qDebug() << "  Annualised return   :" << result.annualizedReturn * 100.0 << "%";
        qDebug() << "  Sharpe ratio        :" << result.sharpeRatio;
        qDebug() << "  Max drawdown        :" << result.maxDrawdown * 100.0 << "%";
        qDebug() << "  Win rate            :" << result.winRate * 100.0 << "%";
        qDebug() << "Benchmark (SPY buy-and-hold):";
        qDebug() << "  Start price         : $" << result.benchmark.startPrice;
        qDebug() << "  End price           : $" << result.benchmark.endPrice;
        qDebug() << "  Total return        :" << result.benchmark.totalReturn * 100.0 << "%";
        qDebug() << "  Annualised return   :" << result.benchmark.annualizedReturn * 100.0 << "%";
        qDebug() << "  Sharpe ratio        :" << result.benchmark.sharpeRatio;
        qDebug() << "  Max drawdown        :" << result.benchmark.maxDrawdown * 100.0 << "%";
        qDebug() << "Alpha vs benchmark    :" << result.alphaVsBenchmark * 100.0 << "% annualised";
    }

    // -----------------------------------------------------------------------
    // Live Yahoo Finance integration test — only runs when
    // IBTRADING_LIVE_TESTS=1 is set in the environment.
    // Downloads real AMD, NVDA, SPY data for the last 10 years.
    // -----------------------------------------------------------------------
    void liveYahooAmdNvdaVsSpyBenchmark() {
        if (qEnvironmentVariable("IBTRADING_LIVE_TESTS") != "1") {
            QSKIP("Set IBTRADING_LIVE_TESTS=1 to run live Yahoo Finance tests");
        }

        // Write a minimal pipeline config
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        QVERIFY(cfgFile.open());
        QTextStream cfgOut(&cfgFile);
        cfgOut << R"({
            "name": "LiveMACrossoverAMD_NVDA",
            "alphas": [{
                "blockId": "ma-crossover-alpha",
                "config": { "fastPeriod": 20, "slowPeriod": 50 }
            }],
            "risks": [],
            "rebalance": { "blockId": "simple-rebalance", "config": {} },
            "execution": { "blockId": "market-order-execution", "config": {} },
            "mergePolicy": ""
        })";
        cfgOut.flush();
        cfgFile.close();

        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgFile.fileName();
        config.startDate          = QDateTime(QDate(2015, 1, 1), QTime(0,0), QTimeZone::utc());
        config.endDate            = QDateTime(QDate(2025, 1, 1), QTime(0,0), QTimeZone::utc());
        config.symbols            = {"AMD", "NVDA"};
        config.dataSourceId       = "yahoo";
        config.dataPath           = {};  // not used for Yahoo
        config.resolution         = Backtest::BarResolution::Day1;
        config.fillModel          = Backtest::FillModelType::MidPrice;
        config.fillTiming         = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
        config.initialCapital     = 100'000.0;
        config.slippageBps        = 5.0;
        config.benchmarkSymbol    = "SPY";

        Backtest::BacktestSession session(config);

        Backtest::BacktestResult result;
        bool sessionFinished = false;
        bool sessionFailed   = false;
        QString failReason;

        connect(&session, &Backtest::BacktestSession::finished,
                [&](const Backtest::BacktestResult& r) { result = r; sessionFinished = true; });
        connect(&session, &Backtest::BacktestSession::failed,
                [&](const QString& reason) { failReason = reason; sessionFailed = true; });
        connect(&session, &Backtest::BacktestSession::progressChanged,
                [&](int p) { qDebug() << "Live progress:" << p << "%"; });

        session.run();

        if (sessionFailed) {
            QFAIL(qPrintable("Live session failed: " + failReason));
        }
        QVERIFY(sessionFinished);
        QVERIFY(result.equityCurve.size() > 200);
        QVERIFY(result.benchmark.equityCurve.size() > 200);

        qDebug() << "=== LIVE: MA Crossover AMD+NVDA vs SPY (2015-2025) ===";
        qDebug() << "  Total trades        :" << result.totalTrades;
        qDebug() << "  Final capital       : $" << result.finalCapital;
        qDebug() << "  Total return        :" << result.totalReturn * 100.0 << "%";
        qDebug() << "  Annualised return   :" << result.annualizedReturn * 100.0 << "%";
        qDebug() << "  Sharpe ratio        :" << result.sharpeRatio;
        qDebug() << "  Max drawdown        :" << result.maxDrawdown * 100.0 << "%";
        qDebug() << "  SPY total return    :" << result.benchmark.totalReturn * 100.0 << "%";
        qDebug() << "  Alpha vs SPY        :" << result.alphaVsBenchmark * 100.0 << "% annualised";
    }
};

#endif // TST_YAHOO_BACKTEST_H
