#ifndef TST_SEMANTIC_LIVE_HISTORICAL_H
#define TST_SEMANTIC_LIVE_HISTORICAL_H

// ---------------------------------------------------------------------------
// Optional live test: real Yahoo Finance historical data + pipeline with
// semanticPipeline + semanticModelRebalance (full ModelDataList path).
//
// Runs only when IBTRADING_SEMANTIC_LIVE_TESTS=1 (see tests/BACKTEST_TESTING.md).
// Requires network. Uses a short window and one symbol to keep runtime low.
// ---------------------------------------------------------------------------

#include <QtTest>
#include <QDate>
#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTimeZone>

#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestReportWriter.h"

namespace SemanticLiveTestDetail {

static QString reportDir()
{
    const QString base = qEnvironmentVariable("TMPDIR", "/tmp");
    const QString dir = base + "/ibtrading_backtest_reports";
    QDir().mkpath(dir);
    return dir;
}

/// Same shape as tst_live_backtest MA config, plus top-level semantic flags
/// (passed through PipelineFactory as graph.config).
static QString writeSemanticMACrossoverConfig(QTemporaryFile& f,
                                            int fastPeriod = 80,
                                            int slowPeriod = 200)
{
    if (!f.open())
        return {};
    QTextStream out(&f);
    out << QString(R"({
    "name": "MACrossover_Semantic_%1_%2",
    "semanticPipeline": true,
    "semanticModelRebalance": true,
    "combineTickAndSemanticSignals": true,
    "alphas": [{
        "blockId": "ma-crossover-alpha",
        "config": { "fastPeriod": %1, "slowPeriod": %2 }
    }],
    "risks": [],
    "rebalance": { "blockId": "simple-rebalance", "config": { "defaultQuantity": 100 } },
    "execution": { "blockId": "market-order-execution", "config": {} },
    "mergePolicy": ""
})")
               .arg(fastPeriod)
               .arg(slowPeriod);
    out.flush();
    f.close();
    return f.fileName();
}

static Backtest::BacktestResult runSession(const Backtest::BacktestConfig& config)
{
    Backtest::BacktestSession session(config);

    Backtest::BacktestResult result;
    bool finished = false;
    bool failed = false;
    QString failReason;

    QObject::connect(&session, &Backtest::BacktestSession::finished,
                       [&](const Backtest::BacktestResult& r) {
                           result = r;
                           finished = true;
                       });
    QObject::connect(&session, &Backtest::BacktestSession::failed,
                       [&](const QString& reason) {
                           failReason = reason;
                           failed = true;
                           finished = true;
                       });

    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(120'000);
    QEventLoop loop;
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        failReason = QStringLiteral("Timeout: Yahoo Finance did not respond within 120s");
        failed = true;
        finished = true;
        loop.quit();
    });
    QObject::connect(&session, &Backtest::BacktestSession::finished, &loop, &QEventLoop::quit);
    QObject::connect(&session, &Backtest::BacktestSession::failed, &loop, &QEventLoop::quit);

    timeout.start();
    session.run();
    if (!finished)
        loop.exec();
    timeout.stop();

    if (failed) {
        qWarning() << "BacktestSession failed:" << failReason;
        result = Backtest::BacktestResult{};
    }

    return result;
}

static QString writeReport(const Backtest::BacktestConfig& config,
                           const Backtest::BacktestResult& result,
                           const QString& tag)
{
    const QString dir = reportDir();
    Backtest::BacktestReportWriter writer(dir);
    Backtest::BacktestConfig cfgCopy = config;
    cfgCopy.symbols = QStringList{tag};
    return writer.write(cfgCopy, result);
}

static void printSummary(const Backtest::BacktestConfig& config,
                         const Backtest::BacktestResult& result)
{
    qInfo() << "--- Semantic pipeline live (Yahoo) ---";
    qInfo() << "  Symbols         :" << config.symbols.join(", ");
    qInfo() << "  Period          :" << config.startDate.toString("yyyy-MM-dd") << "→"
            << config.endDate.toString("yyyy-MM-dd");
    qInfo() << "  Equity points   :" << result.equityCurve.size();
    qInfo() << "  Total trades    :" << result.totalTrades;
    qInfo() << "  Total return    :" << result.totalReturn * 100.0 << "%";
}

} // namespace SemanticLiveTestDetail

class TestSemanticPipelineLiveHistorical : public QObject {
    Q_OBJECT

private slots:
    /// Yahoo daily bars, MA crossover, semanticPipeline + semanticModelRebalance.
    void nvda_yahoo_semantic_model_pipeline_smoke()
    {
        QTemporaryFile cfgFile;
        cfgFile.setAutoRemove(true);
        const QString cfgPath =
            SemanticLiveTestDetail::writeSemanticMACrossoverConfig(cfgFile, 80, 200);
        QVERIFY2(!cfgPath.isEmpty(), "Failed to write pipeline config");

        Backtest::BacktestConfig config;
        config.strategyConfigPath = cfgPath;
        config.startDate = QDateTime(QDate(2024, 1, 1), QTime(0, 0), QTimeZone::utc());
        config.endDate = QDateTime(QDate(2024, 6, 30), QTime(0, 0), QTimeZone::utc());
        config.symbols = {"NVDA"};
        config.dataSourceId = "yahoo";
        config.dataPath = {};
        config.resolution = Backtest::BarResolution::Day1;
        config.fillModel = Backtest::FillModelType::MidPrice;
        config.fillTiming = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
        config.initialCapital = 100'000.0;
        config.slippageBps = 5.0;
        config.benchmarkSymbol = "NVDA";

        const Backtest::BacktestResult result =
            SemanticLiveTestDetail::runSession(config);

        const QString htmlPath =
            SemanticLiveTestDetail::writeReport(config, result, "nvda_semantic_ma_2024h1");
        qInfo() << "\n========================================";
        qInfo() << "  SEMANTIC LIVE REPORT: " << qPrintable(htmlPath);
        qInfo() << "  TXT: " << qPrintable(htmlPath.chopped(5) + ".txt");
        qInfo() << "========================================\n";

        QVERIFY2(!result.equityCurve.isEmpty(),
                 "Equity curve is empty — session failed or no Yahoo data");
        QVERIFY2(result.equityCurve.size() > 20,
                 qPrintable(QStringLiteral("Expected enough daily points, got %1")
                                .arg(result.equityCurve.size())));
        QCOMPARE(result.dataQuality, Backtest::DataQuality::DailyBars);

        SemanticLiveTestDetail::printSummary(config, result);
    }
};

#endif // TST_SEMANTIC_LIVE_HISTORICAL_H
