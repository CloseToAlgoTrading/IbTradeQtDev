#ifndef TST_MOMENTUM_THREE_STOCK_VALIDATION_H
#define TST_MOMENTUM_THREE_STOCK_VALIDATION_H

// Focused diagnostic: universe MSFT, NVDA, GOOG — momentum top 2, equal-weight rebalance.
//
// Uses real Yahoo Finance daily bars (v8 chart API) via HistoricalDataManager + QNetworkAccessManager.
// Requires outbound HTTPS. If prefetch fails (offline CI), the test is skipped (QSKIP).
//
// Writes HTML: summary + SPY benchmark stats, cumulative P&L vs buy-and-hold, trades vs cached O/C, per-symbol charts.

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

#include "Backtest/BacktestFillValidation.h"
#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/HistoricalDataManager.h"
#include "Backtest/SemanticMomentumPipeline.h"
#include "DB/dbquery.h"
#include "backtest/momentum_semantic_validation_html.h"

namespace {

/// Calendar length of the simulated backtest (extra year of history is fetched before this for momentum lookback).
constexpr int kBacktestCalendarYears        = 20;
constexpr int kApproxTradingDaysInBacktest = 252 * kBacktestCalendarYears;

QStringList threeStockUniverse()
{
    return QStringList{QStringLiteral("MSFT"), QStringLiteral("NVDA"), QStringLiteral("GOOG")};
}

static void createHistoricalBarsTableThree(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery     q(db);
    QVERIFY(q.exec(QLatin1String(CREATE_TABLE_HISTORICAL_BARS)));
}

static void writeThreeStockValidationHtml(
    const Backtest::BacktestResult& result,
    const QString& path,
    const QMap<QString, QVector<IBComm::HistoricalBar>>& strategyBars,
    const QDateTime& windowStart,
    const QDateTime& windowEnd,
    int rebalanceEveryNBars,
    double initialCapitalCfg,
    double maxPositionSharesRisk)
{
    MomentumSemanticValidationHtml::Params p;
    p.documentTitle        = QStringLiteral("Momentum 3-stock validation");
    p.pageHeading          = QStringLiteral("Momentum — MSFT, NVDA, GOOG (top 2) — price validation");
    p.primaryInfoboxInnerHtml =
        QStringLiteral("<strong>Live Yahoo Finance data.</strong> Daily OHLCV comes from "
                       "<code>YahooFinanceDataSource</code> (v8 chart API), cached in <code>HistoricalBars</code> "
                       "(adjusted series as Yahoo returns). The test asserts fill ≈ cached close within "
                       "<code>max($0.05, 0.2%)</code> to allow tiny differences between prefetch and in-session cache reads. "
                       "Compare visually to Yahoo’s chart for the same symbol/date (adjusted).");
    p.introParagraph1 =
        QStringLiteral("<p>Fill price should match <b>hist close</b> of the <b>traded symbol</b> on that session (same "
                       "<code>HistoricalBars</code> row used for sizing and <code>IHistoricalRead::getBars</code>).</p>");
    p.introParagraph2 =
        QStringLiteral("<p><b>Hist O/C</b> per row are those cached daily bars on the trade’s <b>session date</b> "
                       "(21:00 UTC). Charts: blue = close; green = buy (positive fill qty); red = sell (negative qty). "
                       "Negative cumulative holdings are <b>short</b> exposure. The semantic pipeline uses absolute "
                       "<code>TargetPosition</code> targets → risk → execution intents (sells before buys); "
                       "<code>simple-rebalance</code> does not open shorts from flat on alpha sell rows.</p>");
    p.introParagraph3 =
        QStringLiteral("<p style=\"font-size:13px;color:#333\"><b>Backtest stats and cumulative P&amp;L vs buy-and-hold benchmark</b> "
                       "are in the tables and chart below (before the trades table). Regenerate by rebuilding "
                       "<code>ibtrading_tests</code> "
                       "and running this test with network (do not set <code>IBTRADING_SKIP_NETWORK_TESTS=1</code>).</p>");
    p.topN                 = 2;
    p.tradingDaysBlurb      = kApproxTradingDaysInBacktest;
    p.universe              = threeStockUniverse();
    p.fullOhlcGridPerTrade  = true;
    p.includeCorrelationColumn = false;

    MomentumSemanticValidationHtml::writeReport(result, path, strategyBars, windowStart, windowEnd, rebalanceEveryNBars,
                                                initialCapitalCfg, maxPositionSharesRisk, p);
}

} // namespace

class TestMomentumThreeStockValidation : public QObject {
    Q_OBJECT
private slots:

    void momentum_top2_three_stock_histPriceValidationHtml()
    {
        const QStringList universe = threeStockUniverse();
        QCOMPARE(universe.size(), 3);

        if (qEnvironmentVariableIntValue("IBTRADING_SKIP_NETWORK_TESTS") == 1)
            QSKIP("Skipped: IBTRADING_SKIP_NETWORK_TESTS=1 (Yahoo live fetch disabled)");

        // ~kBacktestCalendarYears of sessions; fetch one year earlier for momentum lookback (lookbackYears=1).
        const int evalEvery = 22;

        const QDate endCal = QDateTime::currentDateTimeUtc().date();
        QDate       startCal = endCal.addYears(-kBacktestCalendarYears);
        while (startCal.dayOfWeek() >= 6)
            startCal = startCal.addDays(1);

        const QDate     dataStart = startCal.addYears(-1);
        const QDateTime startDate(QDateTime(dataStart, QTime(0, 0), QTimeZone::utc()));
        const QDateTime endDate(endCal, QTime(23, 59, 59), QTimeZone::utc());

        QNetworkAccessManager networkManager;

        const QString conn =
            QStringLiteral("mom3_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QTemporaryFile dbFile;
        dbFile.setAutoRemove(true);
        QVERIFY(dbFile.open());
        dbFile.close();

        std::unique_ptr<Backtest::HistoricalDataManager> histMgr;

        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
            db.setDatabaseName(dbFile.fileName());
            QVERIFY(db.open());
            createHistoricalBarsTableThree(conn);

            histMgr = std::make_unique<Backtest::HistoricalDataManager>(conn, &networkManager);
            QString   refreshed;
            QStringList fetchSyms = universe;
            fetchSyms.append(QStringLiteral("SPY"));
            QMap<QString, QVector<IBComm::HistoricalBar>> allBars =
                histMgr->getBarsMulti(fetchSyms, QStringLiteral("Day1"), QStringLiteral("yahoo"), startDate, endDate,
                                      &refreshed, {});
            QMap<QString, QVector<IBComm::HistoricalBar>> strategyBars;
            for (const QString& s : universe)
                strategyBars[s] = allBars.value(s);
            const QVector<IBComm::HistoricalBar> spyPreload = allBars.value(QStringLiteral("SPY"));

            bool yahooOk = true;
            for (const QString& sym : fetchSyms) {
                const int n = (sym == QStringLiteral("SPY")) ? spyPreload.size() : strategyBars.value(sym).size();
                if (n < 25) {
                    yahooOk = false;
                    qWarning() << "Yahoo prefetch: insufficient bars for" << sym << "count=" << n;
                }
            }
            if (!yahooOk)
                QSKIP("Yahoo Finance prefetch failed or returned too few daily bars (offline CI, rate limit, or "
                      "blocked). Run this test with network access.");

            QVERIFY2(!strategyBars.isEmpty(), "prefetch");

            const double initialCapital = 100'000.0;
            const double maxShares =
                std::floor(initialCapital * 0.25 / 20.0);

            const QJsonObject pipeline =
                Backtest::buildSemanticMomentumPipeline(universe, initialCapital, maxShares, evalEvery,
                                                        /*momentumPeriod*/ 20,
                                                        /*momentumThreshold*/ 0.001,
                                                        /*topN*/ 2);

            Backtest::BacktestConfig config;
            config.startDate      = startDate;
            config.endDate        = endDate;
            config.symbols        = universe;
            config.dataSourceId   = QStringLiteral("yahoo");
            config.resolution     = Backtest::BarResolution::Day1;
            config.fillModel      = Backtest::FillModelType::MidPrice;
            config.fillTiming     = Backtest::FillTiming::SignalOnClose_FillAtClose;
            config.initialCapital   = initialCapital;
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
                QDir(QString(SRCDIR)).absoluteFilePath(QStringLiteral("outputs/momentum_3stock_validation.html"));
            writeThreeStockValidationHtml(result, reportPath, strategyBars, startDate, endDate, evalEvery,
                                        initialCapital, maxShares);
            qInfo() << "Momentum 3-stock validation HTML:" << reportPath;
            const QString cwdCopy =
                QDir(QDir::currentPath()).absoluteFilePath(QStringLiteral("outputs/momentum_3stock_validation.html"));
            // When cwd == SRCDIR, cwdCopy == reportPath: do not remove after write (would delete the report).
            if (reportPath != cwdCopy) {
                if (QFile::exists(cwdCopy))
                    QFile::remove(cwdCopy);
                if (QFile::copy(reportPath, cwdCopy))
                    qInfo() << "Momentum 3-stock validation HTML (cwd copy):" << cwdCopy;
            }

            QVERIFY(QFile::exists(reportPath));

            // Validate: each fill matches cached hist close for traded symbol on that session
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

#endif // TST_MOMENTUM_THREE_STOCK_VALIDATION_H
