#include "backtest/tst_backtest_summary_formatter.h"

#include "Backtest/BacktestSummaryFormatter.h"
#include "Backtest/EquityCurvePnl.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/DataQuality.h"
#include "Backtest/LedgerSnapshot.h"
#include "Pipeline/StrategyRuntimePolicy.h"

#include <QtTest>

using namespace Backtest;

void TestBacktestSummaryFormatter::dataQualityLabel_dailyBars()
{
    QCOMPARE(BacktestSummaryFormatter::dataQualityLabel(DataQuality::DailyBars),
             QStringLiteral("DailyBars"));
}

void TestBacktestSummaryFormatter::buildRows_noBenchmark_twoColumns()
{
    BacktestResult r;
    r.startDate      = QDateTime(QDate(2020, 1, 1), QTime(0, 0), Qt::UTC);
    r.endDate        = QDateTime(QDate(2020, 12, 31), QTime(0, 0), Qt::UTC);
    r.initialCapital = 100000.0;
    r.finalCapital   = 105000.0;
    r.totalReturn    = 0.05;
    r.annualizedReturn = 0.05;
    r.sharpeRatio    = 1.2;
    r.maxDrawdown    = 0.1;
    r.winRate        = 0.5;
    r.totalTrades    = 10;
    r.dataQuality    = DataQuality::DailyBars;
    r.equityCurve.resize(2);

    Pipeline::StrategyRuntimePolicy policy;
    policy.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose;
    policy.rebalanceMode  = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNBars;
    policy.rebalanceIntervalN = 5;

    const auto rows = BacktestSummaryFormatter::buildRows(r, policy);
    QVERIFY(rows.size() >= 5);
    QCOMPARE(rows.first().metric, QStringLiteral("Start"));
    QVERIFY(rows.first().benchmarkValue.isEmpty());
}

void TestBacktestSummaryFormatter::buildRows_withBenchmark_threeColumns()
{
    BacktestResult r;
    r.startDate      = QDateTime(QDate(2020, 1, 1), QTime(0, 0), Qt::UTC);
    r.endDate        = QDateTime(QDate(2020, 12, 31), QTime(0, 0), Qt::UTC);
    r.initialCapital = 100000.0;
    r.finalCapital   = 110000.0;
    r.totalReturn    = 0.1;
    r.annualizedReturn = 0.1;
    r.sharpeRatio    = 1.0;
    r.maxDrawdown    = 0.1;
    r.winRate        = 0.4;
    r.totalTrades    = 5;
    r.alphaVsBenchmark = 0.01;
    r.dataQuality    = DataQuality::DailyBars;
    r.equityCurve.resize(3);
    r.benchmark.symbol = QStringLiteral("SPY");
    r.benchmark.totalReturn = 0.08;
    r.benchmark.annualizedReturn = 0.08;
    r.benchmark.sharpeRatio = 0.9;
    r.benchmark.maxDrawdown = 0.2;
    r.benchmark.startPrice = 100.0;
    r.benchmark.endPrice = 108.0;
    r.benchmark.equityCurve.resize(2);

    Pipeline::StrategyRuntimePolicy policy =
        BacktestSummaryFormatter::policyForSemanticReport(22);

    const auto rows = BacktestSummaryFormatter::buildRows(r, policy);
    QCOMPARE(rows.size(), 17);
    QCOMPARE(BacktestSummaryFormatter::benchmarkColumnHeader(r),
             QStringLiteral("Benchmark (buy-and-hold SPY)"));
    QCOMPARE(rows.at(3).metric, QStringLiteral("Rebalance / trade"));
    QVERIFY(rows.at(3).strategyValue.contains(QStringLiteral("22")));
}

void TestBacktestSummaryFormatter::policyForSemanticReport_matchesEvaluationRebalance()
{
    const auto p = BacktestSummaryFormatter::policyForSemanticReport(7);
    QCOMPARE(static_cast<int>(p.evaluationMode),
             static_cast<int>(Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose));
    QCOMPARE(static_cast<int>(p.rebalanceMode),
             static_cast<int>(Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNBars));
    QCOMPARE(p.rebalanceIntervalN, 7);
}

void TestBacktestSummaryFormatter::equityCurveToPnlSeries_basic()
{
    QVector<LedgerSnapshot> c;
    LedgerSnapshot a;
    a.timestamp      = QDateTime(QDate(2020, 1, 1), QTime(0, 0), Qt::UTC);
    a.portfolioValue = 100000.0;
    LedgerSnapshot b;
    b.timestamp      = QDateTime(QDate(2020, 1, 2), QTime(0, 0), Qt::UTC);
    b.portfolioValue = 101000.0;
    c << a << b;

    const auto pnl = equityCurveToPnlSeries(c, 100000.0);
    QCOMPARE(pnl.size(), 2);
    QVERIFY(qFuzzyCompare(pnl[0].portfolioValue, 0.0));
    QVERIFY(qFuzzyCompare(pnl[1].portfolioValue, 1000.0));
}

void TestBacktestSummaryFormatter::equityCurveToPnlSeries_empty()
{
    const auto pnl = equityCurveToPnlSeries({}, 100000.0);
    QCOMPARE(pnl.size(), 0);
}
