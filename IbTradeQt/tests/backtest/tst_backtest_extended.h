#ifndef TST_BACKTEST_EXTENDED_H
#define TST_BACKTEST_EXTENDED_H

// Extended automated backtest suite: multiple LEGO / universe / fill combinations (CSV data).
// Run with IBTRADING_EXTENDED_BACKTEST=1 (see tests/BACKTEST_TESTING.md).

#include <QtTest>
#include <QObject>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTimeZone>
#include <algorithm>
#include <cmath>

#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestSession.h"

namespace {

static QVector<IBComm::HistoricalBar> extGenerateDailyBars(const QString& symbol,
                                                           const QDate&   startDate,
                                                           int            numDays,
                                                           double         startPrice,
                                                           quint32        seed)
{
    QVector<IBComm::HistoricalBar> bars;
    bars.reserve(numDays);
    quint64 state = seed;
    auto    nextRand = [&]() -> double {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(state >> 33) / static_cast<double>(1ULL << 31) - 1.0;
    };
    double price = startPrice;
    QDate  date  = startDate;
    for (int i = 0; i < numDays; ++i) {
        while (date.dayOfWeek() >= 6)
            date = date.addDays(1);
        const double ret   = 0.0008 + 0.014 * nextRand();
        const double close = price * (1.0 + ret);
        const double open  = price * (1.0 + 0.004 * nextRand());
        const double high  = std::max(open, close) * (1.0 + 0.003 * std::abs(nextRand()));
        const double low   = std::min(open, close) * (1.0 - 0.003 * std::abs(nextRand()));

        IBComm::HistoricalBar bar;
        bar.symbol    = symbol;
        bar.timestamp = QDateTime(date, QTime(21, 0), QTimeZone::utc());
        bar.open      = open;
        bar.high      = high;
        bar.low       = low;
        bar.close     = close;
        bar.volume    = 2'000'000.0 + 500'000.0 * std::abs(nextRand());
        bars.append(bar);
        price = close;
        date  = date.addDays(1);
    }
    return bars;
}

static QString extWriteCsv(QTemporaryFile& f, const QVector<IBComm::HistoricalBar>& bars)
{
    if (!f.open())
        return {};
    QTextStream out(&f);
    out << "symbol,timestamp,open,high,low,close,volume\n";
    for (const auto& b : bars) {
        out << b.symbol << "," << b.timestamp.toString(Qt::ISODate) << "," << b.open << ","
            << b.high << "," << b.low << "," << b.close << "," << b.volume << "\n";
    }
    out.flush();
    f.close();
    return f.fileName();
}

static QJsonObject extLoadDefaultPipeline(const QString& filename)
{
    const QString path = QString(SRCDIR) + "/../Strategies/DefaultPipelines/" + filename;
    QFile           file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

} // namespace

class TestBacktestExtendedLego : public QObject {
    Q_OBJECT

private slots:

    void maCrossover_defaultPipeline_staticSelection_spyQqq()
    {
        QJsonObject pipeline = extLoadDefaultPipeline(QStringLiteral("ma_crossover_pipeline.json"));
        QVERIFY2(!pipeline.isEmpty(), "Failed to load ma_crossover_pipeline.json");

        const QDate startDate(2018, 1, 2);
        const int   nDays = 400;
        auto          spy = extGenerateDailyBars(QStringLiteral("SPY"), startDate, nDays, 250.0, 401U);
        auto          qqq = extGenerateDailyBars(QStringLiteral("QQQ"), startDate, nDays, 180.0, 402U);

        QVector<IBComm::HistoricalBar> all;
        all.append(spy);
        all.append(qqq);
        std::sort(all.begin(), all.end(), [](const IBComm::HistoricalBar& a,
                                             const IBComm::HistoricalBar& b) {
            return a.timestamp < b.timestamp;
        });

        QTemporaryFile combined;
        combined.setAutoRemove(true);
        const QString csvPath = extWriteCsv(combined, all);
        QVERIFY(!csvPath.isEmpty());

        Backtest::BacktestConfig config;
        config.strategyConfigPath.clear();
        config.startDate = QDateTime(startDate, QTime(0, 0), QTimeZone::utc());
        config.endDate =
            QDateTime(startDate.addDays(nDays + 45), QTime(23, 59), QTimeZone::utc());
        config.symbols      = {QStringLiteral("SPY"), QStringLiteral("QQQ")};
        config.dataSourceId = "csv";
        config.dataPath     = csvPath;
        config.resolution   = Backtest::BarResolution::Day1;
        config.fillModel    = Backtest::FillModelType::MidPrice;
        config.fillTiming   = Backtest::FillTiming::SignalOnClose_FillNextBarOpen;
        config.initialCapital = 100'000.0;
        config.slippageBps    = 5.0;
        config.benchmarkSymbol = QStringLiteral("SPY");

        Backtest::BacktestSession session(config);
        session.setPipelineConfig(pipeline);

        Backtest::BacktestResult result;
        bool                       ok = false;
        connect(&session, &Backtest::BacktestSession::finished,
                [&](const Backtest::BacktestResult& r) {
                    result = r;
                    ok     = true;
                });
        QString failReason;
        connect(&session, &Backtest::BacktestSession::failed,
                [&](const QString& e) { failReason = e; });

        session.run();

        QVERIFY2(ok, qPrintable(QStringLiteral("Session failed: ") + failReason));
        QVERIFY2(result.equityCurve.size() > 100, "Equity curve too short");
        QCOMPARE(result.benchmark.symbol, QStringLiteral("SPY"));
        QVERIFY2(result.benchmark.equityCurve.size() > 50, "Benchmark curve too short");
        QVERIFY(result.benchmark.startPrice > 0.0);
    }

    void momentum_externalUniverse_midPrice_fillAtClose()
    {
        const QDate startDate(2019, 3, 1);
        const int   nDays = 300;
        auto          aapl =
            extGenerateDailyBars(QStringLiteral("AAPL"), startDate, nDays, 150.0, 501U);

        QTemporaryFile csv;
        csv.setAutoRemove(true);
        const QString csvPath = extWriteCsv(csv, aapl);
        QVERIFY(!csvPath.isEmpty());

        QJsonObject pipeline;
        QJsonObject alpha;
        alpha["blockId"] = QStringLiteral("momentum-alpha");
        alpha["config"] =
            QJsonObject{{QStringLiteral("lookback"), 5}, {QStringLiteral("threshold"), 0.0005}};
        pipeline["alphas"]      = QJsonArray{alpha};
        pipeline["risks"]       = QJsonArray();
        QJsonObject risk;
        risk["blockId"] = QStringLiteral("max-position-risk");
        risk["config"]  = QJsonObject{{QStringLiteral("maxPositionSize"), 5000.0},
                                     {QStringLiteral("maxTotalExposure"), 50000.0}};
        pipeline["risks"] = QJsonArray{risk};
        QJsonObject reb;
        reb["blockId"]  = QStringLiteral("simple-rebalance");
        reb["config"]   = QJsonObject{{QStringLiteral("defaultQuantity"), 10.0}};
        pipeline["rebalance"] = reb;
        QJsonObject exec;
        exec["blockId"]  = QStringLiteral("market-order-execution");
        exec["config"]   = QJsonObject();
        pipeline["execution"]   = exec;
        pipeline["mergePolicy"] = QString();

        Backtest::BacktestConfig config;
        config.startDate = QDateTime(startDate, QTime(0, 0), QTimeZone::utc());
        config.endDate =
            QDateTime(startDate.addDays(nDays + 20), QTime(23, 59), QTimeZone::utc());
        config.symbols        = {QStringLiteral("AAPL")};
        config.dataSourceId   = "csv";
        config.dataPath       = csvPath;
        config.resolution     = Backtest::BarResolution::Day1;
        config.fillModel      = Backtest::FillModelType::MidPrice;
        config.fillTiming     = Backtest::FillTiming::SignalOnClose_FillAtClose;
        config.initialCapital = 50'000.0;
        config.slippageBps     = 3.0;
        config.benchmarkSymbol.clear();

        Backtest::BacktestSession session(config);
        session.setPipelineConfig(pipeline);

        bool                       ok = false;
        Backtest::BacktestResult result;
        QString                    failReason;
        connect(&session, &Backtest::BacktestSession::finished,
                [&](const Backtest::BacktestResult& r) {
                    result = r;
                    ok     = true;
                });
        connect(&session, &Backtest::BacktestSession::failed,
                [&](const QString& e) { failReason = e; });

        session.run();

        QVERIFY2(ok, qPrintable(QStringLiteral("Session failed: ") + failReason));
        QVERIFY(!result.equityCurve.isEmpty());
        QCOMPARE(result.initialCapital, 50'000.0);
    }
};

#endif // TST_BACKTEST_EXTENDED_H
