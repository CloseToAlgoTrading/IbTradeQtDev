#include "backtest/tst_backtest_statistics.h"

#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestStatisticsCalculator.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/LedgerSnapshot.h"

#include <QJsonValue>
#include <QtMath>
#include <QtTest>

using namespace Backtest;

static BacktestResult flatResult()
{
    BacktestResult r;
    r.initialCapital = 100000.0;
    r.finalCapital   = 100000.0;
    r.startDate      = QDateTime(QDate(2020, 1, 1), QTime(0, 0), Qt::UTC);
    r.endDate        = QDateTime(QDate(2020, 12, 31), QTime(0, 0), Qt::UTC);
    r.totalReturn    = 0.0;
    r.annualizedReturn = 0.0;
    LedgerSnapshot s1;
    s1.timestamp      = r.startDate;
    s1.portfolioValue = 100000.0;
    LedgerSnapshot s2;
    s2.timestamp      = r.endDate;
    s2.portfolioValue = 100000.0;
    r.equityCurve     = {s1, s2};
    r.totalTrades     = 0;
    return r;
}

static BacktestResult zeroTradesWithCurve()
{
    BacktestResult r = flatResult();
    r.totalTrades    = 0;
    r.finalCapital   = 95000.0;
    r.totalReturn    = -0.05;
    r.equityCurve[1].portfolioValue = 95000.0;
    return r;
}

void TestBacktestStatistics::sortino_zeroWhenFlat()
{
    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(flatResult());
    QVERIFY(qFuzzyIsNull(st.sortinoRatio) || qFuzzyIsNull(st.sharpeRatio));
}

void TestBacktestStatistics::profitFactor_zeroTrades()
{
    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(zeroTradesWithCurve());
    QCOMPARE(st.totalTrades, 0);
    QVERIFY(st.profitFactor >= 0.0);
}

void TestBacktestStatistics::json_hasSchemaVersion()
{
    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(flatResult());
    const QJsonObject            o  = st.toJson(1);
    QCOMPARE(o.value(QStringLiteral("schemaVersion")).toInt(), 1);
    QVERIFY(o.contains(QStringLiteral("monthlyReturns")));
    QVERIFY(o.contains(QStringLiteral("yearlyReturns")));
}

void TestBacktestStatistics::profitFactor_allLosingRoundTrip()
{
    BacktestResult r;
    r.initialCapital = 100000.0;
    r.finalCapital   = 90000.0;
    r.startDate      = QDateTime(QDate(2020, 1, 1), QTime(0, 0), Qt::UTC);
    r.endDate        = QDateTime(QDate(2020, 1, 10), QTime(0, 0), Qt::UTC);
    r.totalReturn    = -0.1;
    r.totalTrades    = 2;
    LedgerSnapshot s1;
    s1.timestamp      = r.startDate;
    s1.portfolioValue = 100000.0;
    s1.cash           = 100000.0;
    LedgerSnapshot s2;
    s2.timestamp      = r.endDate;
    s2.portfolioValue = 90000.0;
    s2.cash           = 90000.0;
    r.equityCurve     = {s1, s2};
    FilledOrder buy;
    buy.symbol     = QStringLiteral("AAA");
    buy.quantity   = 100.0;
    buy.fillPrice  = 100.0;
    buy.timestamp  = r.startDate;
    FilledOrder sell;
    sell.symbol    = QStringLiteral("AAA");
    sell.quantity  = -100.0;
    sell.fillPrice = 90.0;
    sell.timestamp = r.endDate;
    r.tradeLog = {buy, sell};

    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(r);
    QVERIFY(st.profitFactor < 1e-9 || qFuzzyIsNull(st.profitFactor));
}

void TestBacktestStatistics::exposure_cashVsInvested()
{
    BacktestResult r = flatResult();
    r.equityCurve[0].cash           = 50000.0;
    r.equityCurve[0].portfolioValue = 100000.0;
    r.equityCurve[1].cash           = 50000.0;
    r.equityCurve[1].portfolioValue = 100000.0;
    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(r);
    QVERIFY(qAbs(st.averageExposurePct - 0.5) < 1e-9);
}

void TestBacktestStatistics::monthlyReturns_twoMonths()
{
    BacktestResult r = flatResult();
    r.equityCurve.clear();
    LedgerSnapshot a;
    a.timestamp      = QDateTime(QDate(2020, 1, 2), QTime(0, 0), Qt::UTC);
    a.portfolioValue = 100000.0;
    LedgerSnapshot b;
    b.timestamp      = QDateTime(QDate(2020, 1, 30), QTime(0, 0), Qt::UTC);
    b.portfolioValue = 100000.0;
    LedgerSnapshot c;
    c.timestamp      = QDateTime(QDate(2020, 2, 3), QTime(0, 0), Qt::UTC);
    c.portfolioValue = 100000.0;
    LedgerSnapshot d;
    d.timestamp      = QDateTime(QDate(2020, 2, 28), QTime(0, 0), Qt::UTC);
    d.portfolioValue = 110000.0;
    r.equityCurve = {a, b, c, d};

    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(r);
    QCOMPARE(st.monthlyReturns.size(), 2);
    QCOMPARE(st.yearlyReturns.size(), 1);
    bool febFound = false;
    for (const QJsonValue& v : st.monthlyReturns) {
        const QJsonObject o = v.toObject();
        if (o.value(QStringLiteral("period")).toString() == QLatin1String("2020-02")) {
            febFound = true;
            QVERIFY(qAbs(o.value(QStringLiteral("return")).toDouble() - 0.1) < 1e-9);
        }
    }
    QVERIFY(febFound);
}

void TestBacktestStatistics::turnover_positiveWithTrades()
{
    BacktestResult r = flatResult();
    FilledOrder f;
    f.symbol    = QStringLiteral("X");
    f.quantity  = 10.0;
    f.fillPrice = 100.0;
    f.timestamp = r.startDate;
    r.tradeLog.append(f);
    r.totalTrades = 1;

    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(r);
    QVERIFY(st.turnoverAnnualized > 0.0);
}
