#ifndef TST_LIVE_BACKTEST_H
#define TST_LIVE_BACKTEST_H

// ---------------------------------------------------------------------------
// Live end-to-end backtest tests — require internet access.
//
// These tests download REAL historical data from Yahoo Finance and run a full
// backtest. They run only when IBTRADING_LIVE_TESTS=1 (see tests/BACKTEST_TESTING.md).
//
// Strategy  : MA Crossover (fast=20 bars, slow=50 bars) on AMD + NVDA
// Benchmark : SPY (buy-and-hold)
// Period    : 10 years  (2015-01-01 → 2025-01-01)
// Capital   : $100,000
// Fill      : MidPrice, SignalOnClose→FillNextBarOpen, 5 bps slippage
//
// After each test a report is written to tests/outputs/ (SRCDIR/outputs).
// Both .html and .txt formats are produced.
// The path is printed to the test output so you can open it directly.
// ---------------------------------------------------------------------------

#include <QtTest>
#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>

#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestReportWriter.h"
#include "Backtest/YahooFinanceDataSource.h"

// ---------------------------------------------------------------------------
// Helper: build a pipeline config JSON file for MA crossover
// ---------------------------------------------------------------------------
static QString writeMACrossoverConfig(QTemporaryFile& f,
                                      int fastPeriod = 20,
                                      int slowPeriod = 50)
{
    if (!f.open()) return {};
    QTextStream out(&f);
    out << QString(R"({
    "name": "MACrossover_%1_%2",
    "alphas": [{
        "blockId": "ma-crossover-alpha",
        "config": { "fastPeriod": %1, "slowPeriod": %2 }
    }],
    "risks": [],
    "rebalance": { "blockId": "simple-rebalance", "config": { "defaultQuantity": 100 } },
    "execution": { "blockId": "market-order-execution", "config": {} },
    "mergePolicy": ""
})").arg(fastPeriod).arg(slowPeriod);
    out.flush();
    f.close();
    return f.fileName();
}

// ---------------------------------------------------------------------------
// Helper: report output directory
// ---------------------------------------------------------------------------
static QString reportDir()
{
    const QString dir =
        QDir(QString(SRCDIR)).absoluteFilePath(QStringLiteral("outputs"));
    QDir().mkpath(dir);
    return dir;
}

// ---------------------------------------------------------------------------
// TestLiveBacktest
// ---------------------------------------------------------------------------
class TestLiveBacktest : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Test 1: AMD + NVDA, MA crossover (20/50), SPY benchmark, 10 years
    // -----------------------------------------------------------------------
    void amdNvdaMACrossoverVsSpyTenYears()
    {
        // ---- Pipeline config ----
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        // Tick-based periods: with 4 OHLC ticks/bar, fast=80 ≈ 20 bars, slow=200 ≈ 50 bars.
        const QString cfgPath = writeMACrossoverConfig(cfgFile, 80, 200);
        QVERIFY2(!cfgPath.isEmpty(), "Failed to write pipeline config");

        // ---- Backtest config ----
        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgPath;
        config.startDate          = QDateTime(QDate(2015, 1, 1), QTime(0, 0), QTimeZone::utc());
        config.endDate            = QDateTime(QDate(2025, 1, 1), QTime(0, 0), QTimeZone::utc());
        config.symbols            = {"AMD", "NVDA"};
        config.dataSourceId       = "yahoo";
        config.dataPath           = {};
        config.resolution         = Backtest::BarResolution::Day1;
        config.fillModel          = Backtest::FillModelType::MidPrice;
        config.fillTiming         = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
        config.initialCapital     = 100'000.0;
        config.slippageBps        = 5.0;
        config.benchmarkSymbol    = "SPY";

        // ---- Run ----
        const Backtest::BacktestResult result = runSession(config);

        // ---- Write report ----
        const QString htmlPath = writeReport(config, result, "amd_nvda_ma_10y");
        qInfo() << "\n========================================";
        qInfo() << "  REPORT: " << qPrintable(htmlPath);
        qInfo() << "  TXT:    " << qPrintable(htmlPath.chopped(5) + ".txt");
        qInfo() << "========================================\n";

        // ---- Assertions ----
        QVERIFY2(!result.equityCurve.isEmpty(),
                 "Equity curve is empty — no data was loaded");
        QVERIFY2(result.equityCurve.size() > 200,
                 qPrintable(QString("Expected >200 equity points, got %1")
                            .arg(result.equityCurve.size())));
        QVERIFY2(result.initialCapital == 100'000.0, "Initial capital mismatch");
        QCOMPARE(result.dataQuality, Backtest::DataQuality::DailyBars);

        // Benchmark must be populated
        QVERIFY2(!result.benchmark.symbol.isEmpty(), "Benchmark not computed");
        QCOMPARE(result.benchmark.symbol, QString("SPY"));
        QVERIFY2(result.benchmark.startPrice > 0.0, "Benchmark start price is zero");
        QVERIFY2(result.benchmark.endPrice   > 0.0, "Benchmark end price is zero");
        QVERIFY2(result.benchmark.equityCurve.size() > 200,
                 "Benchmark equity curve too short");

        // Print summary to test output
        printSummary(config, result);
    }

    // -----------------------------------------------------------------------
    // Test 2: Same strategy, shorter period (2020-2024) — faster to run
    // -----------------------------------------------------------------------
    void amdNvdaMACrossoverVsSpy2020_2024()
    {
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        const QString cfgPath = writeMACrossoverConfig(cfgFile, 80, 200);
        QVERIFY2(!cfgPath.isEmpty(), "Failed to write pipeline config");

        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgPath;
        config.startDate          = QDateTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone::utc());
        config.endDate            = QDateTime(QDate(2024, 12, 31), QTime(0, 0), QTimeZone::utc());
        config.symbols            = {"AMD", "NVDA"};
        config.dataSourceId       = "yahoo";
        config.dataPath           = {};
        config.resolution         = Backtest::BarResolution::Day1;
        config.fillModel          = Backtest::FillModelType::MidPrice;
        config.fillTiming         = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
        config.initialCapital     = 100'000.0;
        config.slippageBps        = 5.0;
        config.benchmarkSymbol    = "SPY";

        const Backtest::BacktestResult result = runSession(config);

        const QString htmlPath = writeReport(config, result, "amd_nvda_ma_2020_2024");
        qInfo() << "\n========================================";
        qInfo() << "  REPORT: " << qPrintable(htmlPath);
        qInfo() << "  TXT:    " << qPrintable(htmlPath.chopped(5) + ".txt");
        qInfo() << "========================================\n";

        QVERIFY2(!result.equityCurve.isEmpty(), "Equity curve is empty");
        QVERIFY2(result.benchmark.startPrice > 0.0, "Benchmark start price is zero");

        printSummary(config, result);
    }

    // -----------------------------------------------------------------------
    // Test 3: Single asset — NVDA only, aggressive MA (10/30), no benchmark
    // -----------------------------------------------------------------------
    void nvdaOnlyMACrossoverFastPeriod()
    {
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        const QString cfgPath = writeMACrossoverConfig(cfgFile, 40, 120);
        QVERIFY2(!cfgPath.isEmpty(), "Failed to write pipeline config");

        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgPath;
        config.startDate          = QDateTime(QDate(2019, 1, 1), QTime(0, 0), QTimeZone::utc());
        config.endDate            = QDateTime(QDate(2024, 12, 31), QTime(0, 0), QTimeZone::utc());
        config.symbols            = {"NVDA"};
        config.dataSourceId       = "yahoo";
        config.dataPath           = {};
        config.resolution         = Backtest::BarResolution::Day1;
        config.fillModel          = Backtest::FillModelType::MidPrice;
        config.fillTiming         = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
        config.initialCapital     = 100'000.0;
        config.slippageBps        = 5.0;
        config.benchmarkSymbol    = "NVDA";  // compare strategy vs buy-and-hold NVDA

        const Backtest::BacktestResult result = runSession(config);

        const QString htmlPath = writeReport(config, result, "nvda_ma_fast_2019_2024");
        qInfo() << "\n========================================";
        qInfo() << "  REPORT: " << qPrintable(htmlPath);
        qInfo() << "  TXT:    " << qPrintable(htmlPath.chopped(5) + ".txt");
        qInfo() << "========================================\n";

        QVERIFY2(!result.equityCurve.isEmpty(), "Equity curve is empty");
        printSummary(config, result);
    }

private:
    // -----------------------------------------------------------------------
    // Run a BacktestSession synchronously, spinning the event loop.
    // Times out after 120 seconds (Yahoo Finance is usually < 5s per symbol).
    // -----------------------------------------------------------------------
    Backtest::BacktestResult runSession(const Backtest::BacktestConfig& config)
    {
        Backtest::BacktestSession session(config);

        Backtest::BacktestResult result;
        bool finished = false;
        bool failed   = false;
        QString failReason;

        connect(&session, &Backtest::BacktestSession::finished,
                [&](const Backtest::BacktestResult& r) {
                    result   = r;
                    finished = true;
                });
        connect(&session, &Backtest::BacktestSession::failed,
                [&](const QString& reason) {
                    failReason = reason;
                    failed     = true;
                    finished   = true;
                });
        int lastReportedProgress = -1;
        connect(&session, &Backtest::BacktestSession::progressChanged,
                [&](int p) {
                    const int bucket = (p / 10) * 10;
                    if (bucket != lastReportedProgress) {
                        lastReportedProgress = bucket;
                        qInfo() << "  Progress:" << bucket << "%";
                    }
                });

        // Timeout guard — if Yahoo is unreachable, fail cleanly
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(120'000);
        QEventLoop loop;
        connect(&timeout, &QTimer::timeout, &loop, [&]() {
            failReason = "Timeout: Yahoo Finance did not respond within 120s";
            failed     = true;
            finished   = true;
            loop.quit();
        });
        connect(&session, &Backtest::BacktestSession::finished, &loop, &QEventLoop::quit);
        connect(&session, &Backtest::BacktestSession::failed,   &loop, &QEventLoop::quit);

        timeout.start();
        session.run();  // synchronous for CSV/JSONL; for Yahoo this returns immediately
                        // and the event loop below drains the async network replies
        if (!finished) loop.exec();
        timeout.stop();

        if (failed) {
            // Can't use QFAIL here (non-void return) — propagate via empty result
            qWarning() << "BacktestSession failed:" << failReason;
            // Mark result as failed by leaving equityCurve empty;
            // the calling test slot will assert on it.
            result = Backtest::BacktestResult{};
        }

        return result;
    }

    // -----------------------------------------------------------------------
    // Write HTML + TXT report and return the HTML path
    // -----------------------------------------------------------------------
    static QString writeReport(const Backtest::BacktestConfig& config,
                               const Backtest::BacktestResult& result,
                               const QString& tag)
    {
        const QString dir = reportDir();
        // Use a tag-based name so reports don't overwrite each other
        Backtest::BacktestReportWriter writer(dir);

        // Temporarily adjust config copy to embed the tag in the filename
        Backtest::BacktestConfig cfgCopy = config;
        cfgCopy.symbols = QStringList{tag};  // drives the filename stem

        return writer.write(cfgCopy, result);
    }

    // -----------------------------------------------------------------------
    // Print a concise summary to the test output
    // -----------------------------------------------------------------------
    static void printSummary(const Backtest::BacktestConfig& config,
                             const Backtest::BacktestResult& result)
    {
        qInfo() << "--- Strategy Summary ---";
        qInfo() << "  Symbols         :" << config.symbols.join(", ");
        qInfo() << "  Period          :" << config.startDate.toString("yyyy-MM-dd")
                << "→" << config.endDate.toString("yyyy-MM-dd");
        qInfo() << "  Equity points   :" << result.equityCurve.size();
        qInfo() << "  Total trades    :" << result.totalTrades;
        qInfo() << "  Initial capital : $" << result.initialCapital;
        qInfo() << "  Final capital   : $" << result.finalCapital;
        qInfo() << "  Total return    :" << result.totalReturn * 100.0 << "%";
        qInfo() << "  Ann. return     :" << result.annualizedReturn * 100.0 << "%";
        qInfo() << "  Sharpe ratio    :" << result.sharpeRatio;
        qInfo() << "  Max drawdown    :" << result.maxDrawdown * 100.0 << "%";
        qInfo() << "  Win rate        :" << result.winRate * 100.0 << "%";

        if (!result.benchmark.symbol.isEmpty()) {
            qInfo() << "--- Benchmark (" << result.benchmark.symbol << ") ---";
            qInfo() << "  Start price     : $" << result.benchmark.startPrice;
            qInfo() << "  End price       : $" << result.benchmark.endPrice;
            qInfo() << "  Total return    :" << result.benchmark.totalReturn * 100.0 << "%";
            qInfo() << "  Ann. return     :" << result.benchmark.annualizedReturn * 100.0 << "%";
            qInfo() << "  Sharpe ratio    :" << result.benchmark.sharpeRatio;
            qInfo() << "  Max drawdown    :" << result.benchmark.maxDrawdown * 100.0 << "%";
            qInfo() << "  Alpha (ann.)    :" << result.alphaVsBenchmark * 100.0 << "%";
        }
    }
};

#endif // TST_LIVE_BACKTEST_H
