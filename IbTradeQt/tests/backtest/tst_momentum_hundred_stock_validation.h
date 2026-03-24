#ifndef TST_MOMENTUM_HUNDRED_STOCK_VALIDATION_H
#define TST_MOMENTUM_HUNDRED_STOCK_VALIDATION_H

// Same shape as tst_momentum_three_stock_validation.h but with a 100-name universe.
//
// Price correctness (same as 3-stock test):
// 1) HistoricalDataManager prefetches Yahoo Day1 bars into SQLite (HistoricalBars) via getBarsMulti.
//    For many symbols, HistoricalDataManager::Config sets yahooFetchBatchSize / yahooFetchTimeoutMs
//    so fetchAndCache runs multiple Yahoo batches (see HistoricalDataManager.cpp) — one HTTP GET per
//    symbol, but batched to avoid a single requestBars wave timing out.
// 2) BacktestSession receives setPreloadedBars(strategyBars) so replay does not re-download.
// 3) setHistoricalDataManager(histMgr.get()) wires IHistoricalRead to the same cache so semantic
//    momentum + fills use the same rows the test compares in the trade log (fill ≈ cached close).
//
// Requires outbound HTTPS. Prefetch retries symbols with too few bars (Yahoo often rate-limits large
// parallel waves). If prefetch still fails after retries, the test QSKIPs.
//
// HTML is written to tests/outputs/momentum_100stock_validation.html (compact trades table for 100 names).

#include <QtTest>
#include <QObject>
#include <QNetworkAccessManager>
#include <QFile>
#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QDateTime>
#include <QTimeZone>
#include <QDir>
#include <QTemporaryFile>
#include <QUuid>
#include <cmath>
#include <memory>

#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestFillValidation.h"
#include "Backtest/HistoricalDataManager.h"
#include "Backtest/SemanticMomentumPipeline.h"
#include "DB/dbquery.h"
#include "backtest/momentum_e2e_universe.h"
#include "backtest/momentum_semantic_validation_html.h"

namespace {

constexpr int kUniverseSize        = 100;
constexpr int kYahooBatchSize      = 10;
constexpr int kYahooTimeoutMs      = 120'000;
constexpr int kMinPrefetchBars     = 25;
constexpr int kPrefetchMaxRetries  = 5;
/// Calendar length of the simulated backtest (extra year of history is fetched before this for momentum lookback).
constexpr int kHundredStockBacktestCalendarYears = 20;
constexpr int kHundredApproxTradingDaysInBacktest = 252 * kHundredStockBacktestCalendarYears;
constexpr int kRebalanceEveryNBars = 22;
constexpr double kInitialCapital     = 100'000.0;
constexpr double kMaxPositionFrac    = 0.25;

QStringList hundredStockUniverse()
{
    QStringList all = momentumE2eUniverseSymbols();
    return all.mid(0, kUniverseSize);
}

static void createHistoricalBarsTableHundred(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery     q(db);
    QVERIFY(q.exec(QLatin1String(CREATE_TABLE_HISTORICAL_BARS)));
}

static void writeHundredStockValidationHtml(
    const Backtest::BacktestResult& result,
    const QString& path,
    const QStringList& universe,
    const QMap<QString, QVector<IBComm::HistoricalBar>>& strategyBars,
    const QDateTime& windowStart,
    const QDateTime& windowEnd,
    int rebalanceEveryNBars,
    double initialCapitalCfg,
    double maxPositionSharesRisk)
{
    MomentumSemanticValidationHtml::Params p;
    p.documentTitle        = QStringLiteral("Momentum 100-stock validation");
    p.pageHeading          = QStringLiteral("Momentum — 100 large-cap names (top 2) — price validation");
    p.primaryInfoboxInnerHtml =
        QStringLiteral("<strong>Live Yahoo Finance data.</strong> Daily OHLCV comes from "
                       "<code>YahooFinanceDataSource</code> (v8 chart API), cached in <code>HistoricalBars</code> "
                       "via <code>HistoricalDataManager::getBarsMulti</code> (batched prefetch). "
                       "The test asserts fill ≈ cached close within "
                       "<code>max($0.05, 0.2%)</code> to allow tiny differences between prefetch and in-session cache reads. "
                       "Compare visually to Yahoo’s chart for the same symbol/date (adjusted).");
    p.introParagraph1 =
        QStringLiteral("<p>Fill price should match <b>hist close</b> of the <b>traded symbol</b> on that session (same "
                       "<code>HistoricalBars</code> row used for sizing and <code>IHistoricalRead::getBars</code>).</p>");
    p.introParagraph2 =
        QStringLiteral("<p><b>Hist O/C</b> per row in the compact trades table are for the <b>traded</b> symbol only "
                       "(full per-name grid omitted for 100 names). Charts: blue = close; green = buy; red = sell. "
                       "Negative cumulative holdings are <b>short</b> exposure. The semantic pipeline uses absolute "
                       "<code>TargetPosition</code> targets → risk → execution intents (sells before buys); "
                       "<code>simple-rebalance</code> does not open shorts from flat on alpha sell rows.</p>");
    p.introParagraph3 =
        QStringLiteral("<p style=\"font-size:13px;color:#333\"><b>Backtest stats and cumulative P&amp;L vs buy-and-hold benchmark</b> "
                       "are in the tables and chart below (before the trades table). Regenerate by rebuilding "
                       "<code>ibtrading_tests</code> "
                       "and running this test with network (do not set <code>IBTRADING_SKIP_NETWORK_TESTS=1</code>).</p>");
    p.topN                    = 2;
    p.tradingDaysBlurb        = kHundredApproxTradingDaysInBacktest;
    p.universe                = universe;
    p.fullOhlcGridPerTrade  = false;
    p.includeCorrelationColumn = false;

    MomentumSemanticValidationHtml::writeReport(result, path, strategyBars, windowStart, windowEnd, rebalanceEveryNBars,
                                                initialCapitalCfg, maxPositionSharesRisk, p);
}

} // namespace

class TestMomentumHundredStockValidation : public QObject {
    Q_OBJECT
private slots:

    void momentum_top2_hundred_stock_histPriceValidationHtml()
    {
        const QStringList universe = hundredStockUniverse();
        QCOMPARE(universe.size(), kUniverseSize);
        QVERIFY2(momentumE2eUniverseSymbols().size() >= kUniverseSize,
                 "momentum_e2e_universe must list at least 100 symbols");

        if (qEnvironmentVariableIntValue("IBTRADING_SKIP_NETWORK_TESTS") == 1)
            QSKIP("Skipped: IBTRADING_SKIP_NETWORK_TESTS=1 (Yahoo live fetch disabled)");

        const int evalEvery = kRebalanceEveryNBars;

        const QDate endCal = QDateTime::currentDateTimeUtc().date();
        QDate       startCal = endCal.addYears(-kHundredStockBacktestCalendarYears);
        while (startCal.dayOfWeek() >= 6)
            startCal = startCal.addDays(1);

        const QDate     dataStart = startCal.addYears(-1);
        const QDateTime startDate(QDateTime(dataStart, QTime(0, 0), QTimeZone::utc()));
        const QDateTime endDate(endCal, QTime(23, 59, 59), QTimeZone::utc());

        QNetworkAccessManager networkManager;

        const QString conn =
            QStringLiteral("mom100_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QTemporaryFile dbFile;
        dbFile.setAutoRemove(true);
        QVERIFY(dbFile.open());
        dbFile.close();

        std::unique_ptr<Backtest::HistoricalDataManager> histMgr;

        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
            db.setDatabaseName(dbFile.fileName());
            QVERIFY(db.open());
            createHistoricalBarsTableHundred(conn);

            histMgr = std::make_unique<Backtest::HistoricalDataManager>(conn, &networkManager);
            Backtest::HistoricalDataManager::Config hdmCfg;
            hdmCfg.yahooFetchBatchSize = kYahooBatchSize;
            hdmCfg.yahooFetchTimeoutMs = kYahooTimeoutMs;
            histMgr->setConfig(hdmCfg);

            QString   refreshed;
            QStringList fetchSyms = universe;
            fetchSyms.append(QStringLiteral("SPY"));
            qInfo() << "Prefetch" << fetchSyms.size() << "symbols (batch" << hdmCfg.yahooFetchBatchSize
                    << ", timeout" << hdmCfg.yahooFetchTimeoutMs << "ms)";
            Backtest::HistoricalDataManager::PrefetchRetryOptions pr;
            pr.minBarsPerSymbol = kMinPrefetchBars;
            pr.maxRetries       = kPrefetchMaxRetries;
            pr.firstBackoffMs   = 2000;
            pr.laterBackoffMs   = 4000;
            QMap<QString, QVector<IBComm::HistoricalBar>> barsBySym =
                histMgr->getBarsMultiWithRetry(fetchSyms, QStringLiteral("Day1"), QStringLiteral("yahoo"), startDate,
                                               endDate, pr, &refreshed, {});

            QMap<QString, QVector<IBComm::HistoricalBar>> strategyBars;
            for (const QString& s : universe)
                strategyBars[s] = barsBySym.value(s);
            const QVector<IBComm::HistoricalBar> spyPreload = barsBySym.value(QStringLiteral("SPY"));

            bool yahooOk = true;
            for (const QString& sym : fetchSyms) {
                const int n = (sym == QStringLiteral("SPY")) ? spyPreload.size() : strategyBars.value(sym).size();
                if (n < kMinPrefetchBars) {
                    yahooOk = false;
                    qWarning() << "Yahoo prefetch: insufficient bars for" << sym << "count=" << n;
                }
            }
            if (!yahooOk)
                QSKIP("Yahoo Finance prefetch failed or returned too few daily bars (offline CI, rate limit, or "
                      "blocked). Run this test with network access.");

            QVERIFY2(!strategyBars.isEmpty(), "prefetch");

            const double maxShares = std::floor(kInitialCapital * kMaxPositionFrac / 20.0);

            const QJsonObject pipeline =
                Backtest::buildSemanticMomentumPipeline(universe, kInitialCapital, maxShares, evalEvery,
                                                        /*momentumPeriod*/ 20,
                                                        /*momentumThreshold*/ 0.001,
                                                        /*topN*/ 2);

            Backtest::BacktestConfig config;
            config.startDate       = startDate;
            config.endDate         = endDate;
            config.symbols         = universe;
            config.dataSourceId    = QStringLiteral("yahoo");
            config.resolution      = Backtest::BarResolution::Day1;
            config.fillModel       = Backtest::FillModelType::MidPrice;
            config.fillTiming      = Backtest::FillTiming::SignalOnClose_FillAtClose;
            config.initialCapital  = kInitialCapital;
            config.benchmarkSymbol = QStringLiteral("SPY");

            Backtest::BacktestSession session(config);
            session.setPipelineConfig(pipeline);
            session.setYahooNetworkAccessManager(&networkManager);
            session.setPreloadedBars(strategyBars);
            session.setPreloadedBenchmarkBars(spyPreload);
            session.setHistoricalDataManager(histMgr.get());

            Backtest::BacktestResult result;
            QString                  err;
            bool                     ok = false;
            connect(&session, &Backtest::BacktestSession::finished,
                    [&](const Backtest::BacktestResult& r) {
                        result = r;
                        ok     = true;
                    });
            connect(&session, &Backtest::BacktestSession::failed, [&](const QString& e) { err = e; });

            session.run();

            QVERIFY2(ok, qPrintable(QStringLiteral("Session failed: ") + err));
            QVERIFY2(result.totalTrades > 0, "expected trades");

            QDir().mkpath(QDir(QString(SRCDIR)).absoluteFilePath(QStringLiteral("outputs")));
            const QString reportPath =
                QDir(QString(SRCDIR)).absoluteFilePath(QStringLiteral("outputs/momentum_100stock_validation.html"));
            writeHundredStockValidationHtml(result, reportPath, universe, strategyBars, startDate, endDate, evalEvery,
                                            kInitialCapital, maxShares);
            qInfo() << "Momentum 100-stock validation HTML:" << reportPath;
            const QString cwdCopy =
                QDir(QDir::currentPath()).absoluteFilePath(QStringLiteral("outputs/momentum_100stock_validation.html"));
            if (reportPath != cwdCopy) {
                if (QFile::exists(cwdCopy))
                    QFile::remove(cwdCopy);
                if (QFile::copy(reportPath, cwdCopy))
                    qInfo() << "Momentum 100-stock validation HTML (cwd copy):" << cwdCopy;
            }

            QVERIFY(QFile::exists(reportPath));

            const QVector<Backtest::FillHistMismatch> mism =
                Backtest::fillPriceVsHistoricalCloseMismatches(result, strategyBars);
            if (!mism.isEmpty()) {
                const auto& x = mism.first();
                QVERIFY2(false, qPrintable(QStringLiteral("Fill mismatch for ") + x.symbol + QStringLiteral(" diff=")
                                           + QString::number(x.absDiff, 'f', 8)));
            }

            if (db.isOpen())
                db.close();
        }
        QSqlDatabase::removeDatabase(conn);
    }
};

#endif // TST_MOMENTUM_HUNDRED_STOCK_VALIDATION_H
