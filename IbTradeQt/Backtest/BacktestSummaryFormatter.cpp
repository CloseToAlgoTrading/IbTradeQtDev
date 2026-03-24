#include "Backtest/BacktestSummaryFormatter.h"

#include "Backtest/BacktestResult.h"

#include <algorithm>
#include <cmath>

namespace Backtest {

using Pipeline::StrategyRuntimePolicy;

QString BacktestSummaryFormatter::dataQualityLabel(DataQuality q)
{
    switch (q) {
    case DataQuality::RealTicks:       return QStringLiteral("RealTicks");
    case DataQuality::SynthesizedOHLC: return QStringLiteral("SynthesizedOHLC");
    case DataQuality::DailyBars:       return QStringLiteral("DailyBars");
    }
    return QStringLiteral("?");
}

QString BacktestSummaryFormatter::naCell()
{
    return QStringLiteral("\u2014");
}

QString BacktestSummaryFormatter::evaluationCell(const StrategyRuntimePolicy& policy)
{
    using EM = StrategyRuntimePolicy::EvaluationMode;
    switch (policy.evaluationMode) {
    case EM::EveryTick:
        return QStringLiteral("every tick (EveryTick)");
    case EM::EveryBarClose:
        return QStringLiteral("every bar close (EveryBarClose)");
    case EM::EveryNBars:
        return QStringLiteral("every %1 bars (EveryNBars)")
            .arg(policy.evaluationIntervalN);
    case EM::EveryNMinutes:
        return QStringLiteral("every %1 minutes (EveryNMinutes)")
            .arg(policy.evaluationIntervalN);
    case EM::EveryNDays:
        return QStringLiteral("every %1 days (EveryNDays)")
            .arg(policy.evaluationIntervalN);
    }
    return QStringLiteral("every bar close (EveryBarClose)");
}

QString BacktestSummaryFormatter::rebalanceCell(const StrategyRuntimePolicy& policy)
{
    using RM = StrategyRuntimePolicy::RebalanceMode;
    switch (policy.rebalanceMode) {
    case RM::Immediate:
        return QStringLiteral("immediate (Immediate)");
    case RM::EveryNBars:
        return QStringLiteral("every %1 bar closes (~%1 trading days per day in this feed)")
            .arg(policy.rebalanceIntervalN);
    case RM::EveryNMinutes:
        return QStringLiteral("every %1 minutes (EveryNMinutes)")
            .arg(policy.rebalanceIntervalN);
    case RM::EveryNDays:
        return QStringLiteral("every %1 days (EveryNDays)")
            .arg(policy.rebalanceIntervalN);
    }
    return QStringLiteral("immediate (Immediate)");
}

bool BacktestSummaryFormatter::hasBenchmarkComparisonData(const BacktestResult& result)
{
    if (!result.benchmark.symbol.isEmpty())
        return true;
    if (!result.benchmark.equityCurve.isEmpty())
        return true;
    if (std::fabs(result.benchmark.totalReturn) > 1e-15)
        return true;
    if (std::fabs(result.benchmark.sharpeRatio) > 1e-15)
        return true;
    if (std::fabs(result.benchmark.annualizedReturn) > 1e-15)
        return true;
    if (result.benchmark.startPrice > 0.0 || result.benchmark.endPrice > 0.0)
        return true;
    if (result.benchmark.maxDrawdown > 0.0)
        return true;
    return false;
}

QString BacktestSummaryFormatter::benchmarkColumnHeader(const BacktestResult& result)
{
    if (!hasBenchmarkComparisonData(result))
        return {};
    if (result.benchmark.symbol.isEmpty())
        return QStringLiteral("Benchmark");
    return QStringLiteral("Benchmark (buy-and-hold %1)").arg(result.benchmark.symbol);
}

StrategyRuntimePolicy BacktestSummaryFormatter::policyForSemanticReport(int rebalanceEveryNBars)
{
    StrategyRuntimePolicy p;
    p.evaluationMode = StrategyRuntimePolicy::EvaluationMode::EveryBarClose;
    p.rebalanceMode = StrategyRuntimePolicy::RebalanceMode::EveryNBars;
    p.rebalanceIntervalN = std::max(1, rebalanceEveryNBars);
    return p;
}

QVector<BacktestSummaryRow> BacktestSummaryFormatter::buildRows(const BacktestResult& result,
                                                                const StrategyRuntimePolicy& policy)
{
    const QString na = naCell();
    const QString isoStart = result.startDate.toUTC().toString(Qt::ISODate);
    const QString isoEnd   = result.endDate.toUTC().toString(Qt::ISODate);
    const QString evalStr  = evaluationCell(policy);
    const QString rebStr   = rebalanceCell(policy);
    const QString dq       = dataQualityLabel(result.dataQuality);

    QVector<BacktestSummaryRow> rows;

    auto add2 = [&](const QString& metric, const QString& strategyVal) {
        BacktestSummaryRow r;
        r.metric = metric;
        r.strategyValue = strategyVal;
        r.benchmarkValue.clear();
        rows.append(r);
    };

    auto add3 = [&](const QString& metric, const QString& strategyVal, const QString& benchVal) {
        BacktestSummaryRow r;
        r.metric = metric;
        r.strategyValue = strategyVal;
        r.benchmarkValue = benchVal;
        rows.append(r);
    };

    if (hasBenchmarkComparisonData(result)) {
        QString benchFinalCap;
        if (!result.benchmark.equityCurve.isEmpty())
            benchFinalCap = QString::number(result.benchmark.equityCurve.last().portfolioValue, 'f', 2);
        else
            benchFinalCap = QString::number(result.initialCapital * (1.0 + result.benchmark.totalReturn), 'f', 2);

        add3(QStringLiteral("Start"), isoStart, isoStart);
        add3(QStringLiteral("End"), isoEnd, isoEnd);
        // Benchmark curve uses the same bar timestamps as the strategy.
        add3(QStringLiteral("Evaluation"), evalStr, evalStr);
        add3(QStringLiteral("Rebalance / trade"), rebStr,
             QStringLiteral("buy once at start (buy-and-hold)"));
        add3(QStringLiteral("Data quality"), dq, dq);
        add3(QStringLiteral("Initial capital"), QString::number(result.initialCapital, 'f', 2),
             QString::number(result.initialCapital, 'f', 2));
        add3(QStringLiteral("Final capital"), QString::number(result.finalCapital, 'f', 2), benchFinalCap);
        add3(QStringLiteral("Start price (underlying)"), na,
             QString::number(result.benchmark.startPrice, 'f', 4));
        add3(QStringLiteral("End price (underlying)"), na,
             QString::number(result.benchmark.endPrice, 'f', 4));
        add3(QStringLiteral("Total return"),
             QString::number(result.totalReturn * 100.0, 'f', 2) + QStringLiteral(" %"),
             QString::number(result.benchmark.totalReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        add3(QStringLiteral("Annualized return"),
             QString::number(result.annualizedReturn * 100.0, 'f', 2) + QStringLiteral(" %"),
             QString::number(result.benchmark.annualizedReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        add3(QStringLiteral("Sharpe ratio"), QString::number(result.sharpeRatio, 'f', 4),
             QString::number(result.benchmark.sharpeRatio, 'f', 4));
        add3(QStringLiteral("Max drawdown"),
             QString::number(result.maxDrawdown * 100.0, 'f', 2)
                 + QStringLiteral(" % (from end-of-bar MTM equity; not OHLC intraday low)"),
             QString::number(result.benchmark.maxDrawdown * 100.0, 'f', 2) + QStringLiteral(" %"));
        add3(QStringLiteral("Win rate"),
             QString::number(result.winRate * 100.0, 'f', 2) + QStringLiteral(" %"), na);
        add3(QStringLiteral("Total trades (fills)"), QString::number(result.totalTrades), na);
        add3(QStringLiteral("Equity snapshots"), QString::number(result.equityCurve.size()),
             QString::number(result.benchmark.equityCurve.size()));
        add3(QStringLiteral("Alpha vs benchmark (ann.)"),
             QString::number(result.alphaVsBenchmark * 100.0, 'f', 2) + QStringLiteral(" %"), na);
    } else {
        add2(QStringLiteral("Start"), isoStart);
        add2(QStringLiteral("End"), isoEnd);
        add2(QStringLiteral("Evaluation"), evalStr);
        add2(QStringLiteral("Rebalance / trade"), rebStr);
        add2(QStringLiteral("Data quality"), dq);
        add2(QStringLiteral("Initial capital"), QString::number(result.initialCapital, 'f', 2));
        add2(QStringLiteral("Final capital"), QString::number(result.finalCapital, 'f', 2));
        add2(QStringLiteral("Total return"),
             QString::number(result.totalReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        add2(QStringLiteral("Annualized return"),
             QString::number(result.annualizedReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        add2(QStringLiteral("Sharpe ratio"), QString::number(result.sharpeRatio, 'f', 4));
        add2(QStringLiteral("Max drawdown"),
             QString::number(result.maxDrawdown * 100.0, 'f', 2)
                 + QStringLiteral(" % (from end-of-bar MTM equity; not OHLC intraday low)"));
        add2(QStringLiteral("Win rate"),
             QString::number(result.winRate * 100.0, 'f', 2) + QStringLiteral(" %"));
        add2(QStringLiteral("Total trades (fills)"), QString::number(result.totalTrades));
        add2(QStringLiteral("Equity snapshots"), QString::number(result.equityCurve.size()));
    }

    return rows;
}

} // namespace Backtest
