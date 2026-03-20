#ifndef TST_BACKTEST_ENGINE_COVERAGE_H
#define TST_BACKTEST_ENGINE_COVERAGE_H

// High-value backtest engine coverage: fill/timing matrix (CSV), date edges,
// slippage, universe + pipeline failure modes, Yahoo mock failure shapes,
// metrics invariants, and BacktestController + SQLite + worker thread smoke.

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <QTextStream>
#include <QEventLoop>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtSql/QSqlDatabase>
#include <cmath>

#include "DB/dbquery.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestController.h"
#include "Backtest/BacktestMetricsCollector.h"
#include "Backtest/LedgerSnapshot.h"
#include "Backtest/BenchmarkComparison.h"

#include "backtest/tst_yahoo_backtest.h"

namespace {

static bool nearlyEq(double a, double b, double eps = 1e-9)
{
    return (std::isfinite(a) && std::isfinite(b) && std::abs(a - b) <= eps);
}

static bool initBacktestSqliteSchema(const QString& dbPath)
{
    const QString conn = QStringLiteral("bt_cov_init_") +
                         QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
    db.setDatabaseName(dbPath);
    if (!db.open())
        return false;

    auto exec = [&db](const char* sql) {
        QSqlQuery q(db);
        if (!q.exec(QLatin1String(sql)))
            qWarning() << "initBacktestSqliteSchema:" << q.lastError().text();
    };
    auto execSilent = [&db](const char* sql) {
        QSqlQuery q(db);
        q.exec(QLatin1String(sql));
    };

    exec(CREATE_TABLE_BACKTEST_RUNS);
    exec(CREATE_TABLE_BACKTEST_METRICS);
    exec(CREATE_TABLE_BACKTEST_TRADES);
    exec(CREATE_TABLE_BACKTEST_EQUITY_CURVE);
    exec(CREATE_TABLE_HISTORICAL_BARS);
    exec(CREATE_TABLE_BACKTEST_RUN_PROFILES);
    execSilent(ALTER_BACKTEST_RUNS_ADD_STRATEGY_DEF_ID);
    execSilent(ALTER_BACKTEST_RUNS_ADD_SCOPE_TYPE);
    execSilent(ALTER_BACKTEST_RUNS_ADD_SCOPE_REF_ID);
    execSilent(ALTER_BACKTEST_RUNS_ADD_STRATEGY_VERSION);
    execSilent(ALTER_BACKTEST_RUNS_ADD_CATALOG_STRATEGY_ID);
    execSilent(ALTER_BACKTEST_RUNS_ADD_CATALOG_VERSION_ID);

    db.close();
    QSqlDatabase::removeDatabase(conn);
    return true;
}

static QString makeMomentumPipelineFile()
{
    static QString path;
    static bool ready = false;
    if (!ready) {
        ready = true;
        QTemporaryFile f;
        f.setAutoRemove(false);
        if (f.open()) {
            QTextStream out(&f);
            out << R"({
            "name": "CovMomentum",
            "selection": [],
            "alphas": [{"blockId": "momentum-alpha", "config": {"lookback": 2, "threshold": 0.001}}],
            "risks": [],
            "rebalance": {"blockId": "simple-rebalance", "config": {}},
            "execution": {"blockId": "market-order-execution", "config": {}},
            "mergePolicy": ""
        })";
            out.flush();
            f.close();
            path = f.fileName();
        }
    }
    return path;
}

static QString writeTempCsv(const QVector<QString>& linesAfterHeader)
{
    QTemporaryFile f;
    f.setAutoRemove(false);
    if (!f.open())
        return QString();
    QTextStream out(&f);
    out << "symbol,timestamp,open,high,low,close,volume\n";
    for (const QString& line : linesAfterHeader)
        out << line << "\n";
    out.flush();
    f.close();
    return f.fileName();
}

// Returns true and writes to *outResult on success. Uses QTest::qFail on failure (cannot use
// QVERIFY in a function that returns BacktestResult — macro expands to bare `return`).
static bool runSessionSync(Backtest::BacktestConfig cfg,
                           const QJsonObject& inlinePipeline,
                           Backtest::BacktestResult* outResult)
{
    Backtest::BacktestSession session(cfg);
    if (!inlinePipeline.isEmpty())
        session.setPipelineConfig(inlinePipeline);

    Backtest::BacktestResult result;
    bool done   = false;
    bool failed = false;
    QString reason;
    QObject::connect(&session, &Backtest::BacktestSession::finished,
                     [&](const Backtest::BacktestResult& r) {
                         result = r;
                         done   = true;
                     });
    QObject::connect(&session, &Backtest::BacktestSession::failed,
                     [&](const QString& e) {
                         reason = e;
                         failed = true;
                         done   = true;
                     });
    session.run();
    if (failed) {
        QTest::qFail(qPrintable(QStringLiteral("session failed: ") + reason), __FILE__, __LINE__);
        return false;
    }
    if (!done) {
        QTest::qFail("BacktestSession did not emit finished or failed", __FILE__, __LINE__);
        return false;
    }
    *outResult = result;
    return true;
}

static const char* kPipelineMomentum = R"({
    "name": "CovMomentum",
    "selection": [],
    "alphas": [{"blockId": "momentum-alpha", "config": {"lookback": 2, "threshold": 0.001}}],
    "risks": [],
    "rebalance": {"blockId": "simple-rebalance", "config": {}},
    "execution": {"blockId": "market-order-execution", "config": {}},
    "mergePolicy": ""
})";

} // namespace

class TestBacktestEngineCoverage : public QObject {
    Q_OBJECT

private slots:

    void metrics_finalize_emptyCurve_isInitialOnly()
    {
        Backtest::BacktestMetricsCollector m(100000.0);
        const QDateTime start = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        const QDateTime end   = QDateTime::fromString(QStringLiteral("2024-06-01"), Qt::ISODate);
        Backtest::BacktestResult r = m.finalize(start, end);
        QCOMPARE(r.initialCapital, 100000.0);
        QCOMPARE(r.finalCapital, 100000.0);
        QCOMPARE(r.totalReturn, 0.0);
        QVERIFY(r.equityCurve.isEmpty());
    }

    void metrics_finalize_singleSnapshot_totalReturnFormula()
    {
        Backtest::BacktestMetricsCollector m(100000.0);
        Backtest::LedgerSnapshot s;
        s.portfolioValue = 105000.0;
        s.timestamp      = QDateTime::fromString(QStringLiteral("2024-01-05"), Qt::ISODate);
        m.onSnapshot(s);
        const QDateTime start = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        const QDateTime end   = QDateTime::fromString(QStringLiteral("2024-01-10"), Qt::ISODate);
        Backtest::BacktestResult r = m.finalize(start, end);
        QCOMPARE(r.finalCapital, 105000.0);
        QVERIFY(qAbs(r.totalReturn - 0.05) < 1e-9);
    }

    void csv_allFillModelsAndTimings_finish()
    {
        const QString csvPath = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000"),
            QStringLiteral("AAPL,2024-01-03T16:00:00,186.20,188.00,185.50,187.50,1100000"),
            QStringLiteral("AAPL,2024-01-04T16:00:00,187.50,189.00,186.00,188.80,1300000"),
        });
        const QString pipePath = makeMomentumPipelineFile();
        QVERIFY(!csvPath.isEmpty());
        QVERIFY(!pipePath.isEmpty());

        struct Row {
            Backtest::FillModelType fm;
            Backtest::FillTiming    ft;
        };
        const Row rows[] = {
            { Backtest::FillModelType::Instant, Backtest::FillTiming::SignalOnClose_FillNextBarOpen },
            { Backtest::FillModelType::Instant, Backtest::FillTiming::SignalOnTick_FillAtBidAsk },
            { Backtest::FillModelType::Instant, Backtest::FillTiming::SignalOnClose_FillAtClose },
            { Backtest::FillModelType::MidPrice, Backtest::FillTiming::SignalOnClose_FillNextBarOpen },
            { Backtest::FillModelType::MidPrice, Backtest::FillTiming::SignalOnTick_FillAtBidAsk },
            { Backtest::FillModelType::MidPrice, Backtest::FillTiming::SignalOnClose_FillAtClose },
            { Backtest::FillModelType::BidAsk, Backtest::FillTiming::SignalOnClose_FillNextBarOpen },
            { Backtest::FillModelType::BidAsk, Backtest::FillTiming::SignalOnTick_FillAtBidAsk },
            { Backtest::FillModelType::BidAsk, Backtest::FillTiming::SignalOnClose_FillAtClose },
            { Backtest::FillModelType::SlippageBps, Backtest::FillTiming::SignalOnClose_FillNextBarOpen },
            { Backtest::FillModelType::SlippageBps, Backtest::FillTiming::SignalOnTick_FillAtBidAsk },
            { Backtest::FillModelType::SlippageBps, Backtest::FillTiming::SignalOnClose_FillAtClose },
        };

        for (const Row& row : rows) {
            Backtest::BacktestConfig cfg;
            cfg.strategyConfigPath = pipePath;
            cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
            cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-04"), Qt::ISODate);
            cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
            cfg.dataSourceId       = QStringLiteral("csv");
            cfg.dataPath           = csvPath;
            cfg.resolution         = Backtest::BarResolution::Day1;
            cfg.fillModel          = row.fm;
            cfg.fillTiming         = row.ft;
            cfg.slippageBps        = 2.0;
            cfg.initialCapital     = 100000.0;

            Backtest::BacktestResult r;
            QVERIFY(runSessionSync(cfg, QJsonObject(), &r));
            QVERIFY(!r.equityCurve.isEmpty());
            QVERIFY(std::isfinite(r.finalCapital));
            QVERIFY(r.finalCapital > 0.0);
            QCOMPARE(r.initialCapital, 100000.0);
        }
    }

    void csv_slippageBps_zeroAndHigh_finishes()
    {
        const QString csvPath = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,100.00,101.00,99.00,100.50,1000000"),
            QStringLiteral("AAPL,2024-01-03T16:00:00,100.50,102.00,100.00,101.00,900000"),
        });
        const QString pipePath = makeMomentumPipelineFile();
        QVERIFY(!csvPath.isEmpty());
        QVERIFY(!pipePath.isEmpty());

        for (double slip : { 0.0, 500.0 }) {
            Backtest::BacktestConfig cfg;
            cfg.strategyConfigPath = pipePath;
            cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
            cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-03"), Qt::ISODate);
            cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
            cfg.dataSourceId       = QStringLiteral("csv");
            cfg.dataPath           = csvPath;
            cfg.resolution         = Backtest::BarResolution::Day1;
            cfg.fillModel          = Backtest::FillModelType::SlippageBps;
            cfg.fillTiming         = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
            cfg.slippageBps        = slip;
            cfg.initialCapital     = 100000.0;

            Backtest::BacktestResult r;
            QVERIFY(runSessionSync(cfg, QJsonObject(), &r));
            QVERIFY(std::isfinite(r.finalCapital));
        }
    }

    void csv_invertedDateRange_noBars_metricsStayInitial()
    {
        const QString csvPath = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000"),
        });
        const QString pipePath = makeMomentumPipelineFile();
        QVERIFY(!csvPath.isEmpty());
        QVERIFY(!pipePath.isEmpty());

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = pipePath;
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-02-01"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-01"), Qt::ISODate);
        cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
        cfg.dataSourceId       = QStringLiteral("csv");
        cfg.dataPath           = csvPath;
        cfg.resolution         = Backtest::BarResolution::Day1;
        cfg.initialCapital     = 100000.0;

        Backtest::BacktestResult r;
        QVERIFY(runSessionSync(cfg, QJsonObject(), &r));
        QCOMPARE(r.finalCapital, r.initialCapital);
        QCOMPARE(r.totalReturn, 0.0);
        QVERIFY(r.equityCurve.isEmpty());
    }

    void emptyUniverse_failsWithKnownMessage()
    {
        const QString csvPath = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000"),
        });
        QVERIFY(!csvPath.isEmpty());

        QJsonObject pipeline = QJsonDocument::fromJson(QByteArray(kPipelineMomentum)).object();
        pipeline.insert(QStringLiteral("selection"), QJsonArray{});

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = QString();
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-04"), Qt::ISODate);
        cfg.symbols            = QStringList{};
        cfg.dataSourceId       = QStringLiteral("csv");
        cfg.dataPath           = csvPath;
        cfg.resolution         = Backtest::BarResolution::Day1;
        cfg.initialCapital     = 100000.0;

        Backtest::BacktestSession session(cfg);
        session.setPipelineConfig(pipeline);

        bool failed = false;
        QString err;
        QObject::connect(&session, &Backtest::BacktestSession::failed,
                         [&](const QString& e) {
                             failed = true;
                             err    = e;
                         });
        session.run();
        QVERIFY(failed);
        QVERIFY(err.contains(QStringLiteral("tradeable universe")));
    }

    void emptyPipelineConfig_failsGracefully()
    {
        const QString csvPath = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000"),
        });
        QVERIFY(!csvPath.isEmpty());

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = QString();
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-04"), Qt::ISODate);
        cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
        cfg.dataSourceId       = QStringLiteral("csv");
        cfg.dataPath           = csvPath;
        cfg.resolution         = Backtest::BarResolution::Day1;
        cfg.initialCapital     = 100000.0;

        Backtest::BacktestSession session(cfg);
        session.setPipelineConfig(QJsonObject{});

        bool failed = false;
        QString err;
        QObject::connect(&session, &Backtest::BacktestSession::failed,
                         [&](const QString& e) {
                             failed = true;
                             err    = e;
                         });
        session.run();
        QVERIFY(failed);
        QVERIFY(err.contains(QStringLiteral("invalid")) || err.contains(QStringLiteral("empty")));
    }

    void jsonl_minimalTickSession_finishes()
    {
        QTemporaryFile jf;
        jf.setAutoRemove(true);
        QVERIFY(jf.open());
        QTextStream jo(&jf);
        jo << R"({"type":"MarketTick","symbol":"AAPL","bid":185.0,"ask":185.2,"timestamp":"2024-01-02T09:30:00.000","reqId":1})"
           << "\n";
        jo.flush();
        jf.close();

        QJsonObject pipeline = QJsonDocument::fromJson(QByteArray(kPipelineMomentum)).object();

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = QString();
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-03"), Qt::ISODate);
        cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
        cfg.dataSourceId       = QStringLiteral("jsonl");
        cfg.dataPath           = jf.fileName();
        cfg.resolution         = Backtest::BarResolution::Tick;
        cfg.initialCapital     = 100000.0;

        Backtest::BacktestResult r;
        QVERIFY(runSessionSync(cfg, pipeline, &r));
        QCOMPARE(r.dataQuality, Backtest::DataQuality::RealTicks);
        QVERIFY(std::isfinite(r.finalCapital));
    }

    void yahoo_mockMalformedJson_completesWithNoBars_initialCapital()
    {
        MockNetworkAccessManager nam;
        nam.addSymbolResponse(QStringLiteral("AAPL"), QByteArray("{not even json"));

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = makeMomentumPipelineFile();
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-05"), Qt::ISODate);
        cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
        cfg.dataSourceId       = QStringLiteral("yahoo");
        cfg.dataPath           = QString();
        cfg.resolution         = Backtest::BarResolution::Day1;
        cfg.initialCapital     = 100000.0;
        QVERIFY(!cfg.strategyConfigPath.isEmpty());

        Backtest::BacktestSession session(cfg);
        session.setYahooNetworkAccessManager(&nam);

        Backtest::BacktestResult r;
        bool ok = false;
        QObject::connect(&session, &Backtest::BacktestSession::finished,
                         [&](const Backtest::BacktestResult& out) {
                             r  = out;
                             ok = true;
                         });
        session.run();
        QVERIFY(ok);
        QCOMPARE(r.finalCapital, r.initialCapital);
        QVERIFY(r.equityCurve.isEmpty());
    }

    void csv_benchmark_matchesBenchmarkComparison_reference()
    {
        // Same SPY closes as reference; wide end date so CSV date filter keeps all rows.
        const QString csvPath = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000"),
            QStringLiteral("SPY,2024-01-02T16:00:00,400.00,402.00,399.00,400.00,1000000"),
            QStringLiteral("AAPL,2024-01-03T16:00:00,186.20,188.00,185.50,187.50,1100000"),
            QStringLiteral("SPY,2024-01-03T16:00:00,400.00,405.00,398.00,404.00,900000"),
            QStringLiteral("AAPL,2024-01-04T16:00:00,187.50,189.00,186.00,188.80,1300000"),
            QStringLiteral("SPY,2024-01-04T16:00:00,404.00,408.00,403.00,406.00,800000"),
        });
        QVERIFY(!csvPath.isEmpty());
        const QString pipePath = makeMomentumPipelineFile();
        QVERIFY(!pipePath.isEmpty());

        const QDateTime t1 = QDateTime::fromString(QStringLiteral("2024-01-02T16:00:00"), Qt::ISODate);
        const QDateTime t2 = QDateTime::fromString(QStringLiteral("2024-01-03T16:00:00"), Qt::ISODate);
        const QDateTime t3 = QDateTime::fromString(QStringLiteral("2024-01-04T16:00:00"), Qt::ISODate);

        Backtest::BenchmarkComparison refCmp;
        refCmp.setInitialCapital(100000.0);
        refCmp.addClose(t1, 400.0);
        refCmp.addClose(t2, 404.0);
        refCmp.addClose(t3, 406.0);
        const Backtest::BenchmarkResult ref = refCmp.compute(QStringLiteral("SPY"));

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = pipePath;
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-10"), Qt::ISODate);
        cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
        cfg.dataSourceId       = QStringLiteral("csv");
        cfg.dataPath           = csvPath;
        cfg.resolution         = Backtest::BarResolution::Day1;
        cfg.initialCapital     = 100000.0;
        cfg.benchmarkSymbol    = QStringLiteral("SPY");

        Backtest::BacktestResult r;
        QVERIFY(runSessionSync(cfg, QJsonObject(), &r));

        QVERIFY(nearlyEq(r.benchmark.totalReturn, ref.totalReturn, 1e-9));
        QVERIFY(nearlyEq(r.benchmark.annualizedReturn, ref.annualizedReturn, 1e-9));
        QVERIFY(nearlyEq(r.benchmark.maxDrawdown, ref.maxDrawdown, 1e-9));
        QVERIFY(nearlyEq(r.benchmark.sharpeRatio, ref.sharpeRatio, 1e-9));
        QVERIFY(nearlyEq(r.benchmark.startPrice, ref.startPrice, 1e-9));
        QVERIFY(nearlyEq(r.benchmark.endPrice, ref.endPrice, 1e-9));
        QVERIFY(nearlyEq(r.alphaVsBenchmark,
                         r.annualizedReturn - r.benchmark.annualizedReturn, 1e-9));
    }

    void yahoo_mockEmptyChartResult_finishesWithoutNetworkFailure()
    {
        MockNetworkAccessManager nam;
        // Default fallback in mock is empty chart — already tests empty path; register explicit:
        nam.addSymbolResponse(QStringLiteral("AAPL"), QByteArray());

        Backtest::BacktestConfig cfg;
        cfg.strategyConfigPath = makeMomentumPipelineFile();
        cfg.startDate          = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        cfg.endDate            = QDateTime::fromString(QStringLiteral("2024-01-05"), Qt::ISODate);
        cfg.symbols            = QStringList{ QStringLiteral("AAPL") };
        cfg.dataSourceId       = QStringLiteral("yahoo");
        cfg.resolution         = Backtest::BarResolution::Day1;
        cfg.initialCapital     = 100000.0;
        QVERIFY(!cfg.strategyConfigPath.isEmpty());

        Backtest::BacktestSession session(cfg);
        session.setYahooNetworkAccessManager(&nam);

        bool finished = false;
        QString unexpectedErr;
        QObject::connect(&session, &Backtest::BacktestSession::finished,
                         [&](const Backtest::BacktestResult&) { finished = true; });
        QObject::connect(&session, &Backtest::BacktestSession::failed,
                         [&](const QString& e) { unexpectedErr = e; });
        session.run();
        QVERIFY2(unexpectedErr.isEmpty(), qPrintable(unexpectedErr));
        QVERIFY(finished);
    }

    void backtestController_csv_workerThread_persistsFinishedRow()
    {
        QTemporaryFile dbFile;
        dbFile.setAutoRemove(true);
        QVERIFY(dbFile.open());
        const QString path = dbFile.fileName();
        dbFile.close();

        QVERIFY(initBacktestSqliteSchema(path));

        Backtest::BacktestController ctrl(path, nullptr);

        Backtest::BacktestRunConfig rc;
        rc.strategyId          = QStringLiteral("s1");
        rc.strategyDisplayName = QStringLiteral("cov");
        rc.pipelineConfigJson  = QLatin1String(kPipelineMomentum);
        rc.symbols             = QStringList{ QStringLiteral("AAPL") };
        rc.startDate           = QDateTime::fromString(QStringLiteral("2024-01-02"), Qt::ISODate);
        rc.endDate             = QDateTime::fromString(QStringLiteral("2024-01-05"), Qt::ISODate);
        rc.initialCapital      = 100000.0;
        rc.dataSourceId        = QStringLiteral("csv");
        rc.dataPath            = writeTempCsv({
            QStringLiteral("AAPL,2024-01-02T16:00:00,185.00,186.50,184.80,186.20,1200000"),
            QStringLiteral("AAPL,2024-01-03T16:00:00,186.20,188.00,185.50,187.50,1100000"),
        });
        QVERIFY(!rc.dataPath.isEmpty());
        rc.resolution          = QStringLiteral("Day1");
        rc.fillModel           = QStringLiteral("MidPrice");
        rc.fillTiming          = QStringLiteral("SignalOnClose_FillNextBarOpen");
        rc.slippageBps         = 1.0;

        QEventLoop loop;
        bool gotFinished = false;
        Backtest::BacktestLoadedRun lastLoaded;
        QObject::connect(&ctrl, &Backtest::BacktestController::finished,
                         [&](const Backtest::BacktestLoadedRun& loaded) {
                             lastLoaded   = loaded;
                             gotFinished  = true;
                             loop.quit();
                         });
        QString controllerFail;
        QObject::connect(&ctrl, &Backtest::BacktestController::failed,
                         [&](const QString& e) {
                             controllerFail = e;
                             loop.quit();
                         });

        ctrl.start(rc);
        QTimer::singleShot(120000, &loop, &QEventLoop::quit);
        loop.exec();

        QVERIFY2(controllerFail.isEmpty(), qPrintable(controllerFail));
        QVERIFY(gotFinished);

        QSqlDatabase qdb = QSqlDatabase::database(ctrl.dbConnectionName());
        QVERIFY(qdb.isOpen());
        QSqlQuery qStatus(qdb);
        QVERIFY(qStatus.exec(QStringLiteral("SELECT status FROM BacktestRuns WHERE runId='%1'")
                                 .arg(ctrl.currentRunId())));
        QVERIFY(qStatus.next());
        QCOMPARE(qStatus.value(0).toString(), QStringLiteral("Finished"));

        // Persisted metrics row must match the finished signal payload (same path UI uses).
        auto qm = query_fetchBacktestMetrics(ctrl.currentRunId(), ctrl.dbConnectionName());
        QVERIFY(qm.exec());
        QVERIFY(qm.next());
        const Backtest::BacktestResult& br = lastLoaded.result;
        QCOMPARE(qm.value(QStringLiteral("totalReturn")).toDouble(), br.totalReturn);
        QCOMPARE(qm.value(QStringLiteral("annualizedReturn")).toDouble(), br.annualizedReturn);
        QCOMPARE(qm.value(QStringLiteral("sharpeRatio")).toDouble(), br.sharpeRatio);
        QCOMPARE(qm.value(QStringLiteral("maxDrawdown")).toDouble(), br.maxDrawdown);
        QCOMPARE(qm.value(QStringLiteral("winRate")).toDouble(), br.winRate);
        QCOMPARE(qm.value(QStringLiteral("totalTrades")).toInt(), br.totalTrades);
        QCOMPARE(qm.value(QStringLiteral("initialCapital")).toDouble(), br.initialCapital);
        QCOMPARE(qm.value(QStringLiteral("finalCapital")).toDouble(), br.finalCapital);
        QCOMPARE(qm.value(QStringLiteral("benchmarkReturn")).toDouble(), br.benchmark.totalReturn);
        QCOMPARE(qm.value(QStringLiteral("benchmarkSharpe")).toDouble(), br.benchmark.sharpeRatio);
        QCOMPARE(qm.value(QStringLiteral("alpha")).toDouble(), br.alphaVsBenchmark);
    }
};

#endif // TST_BACKTEST_ENGINE_COVERAGE_H
