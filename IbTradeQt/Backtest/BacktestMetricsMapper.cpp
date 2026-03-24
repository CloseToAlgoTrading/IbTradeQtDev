#include "Backtest/BacktestMetricsMapper.h"

#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestStatistics.h"

#include <QJsonDocument>

namespace Backtest {

DbBacktestMetrics toDbBacktestMetrics(const QString& runId,
                                     const BacktestResult& result,
                                     const BacktestStatistics& stats)
{
    DbBacktestMetrics m;
    m.runId            = runId;
    m.totalReturn      = result.totalReturn;
    m.annualizedReturn = result.annualizedReturn;
    m.sharpeRatio      = result.sharpeRatio;
    m.maxDrawdown      = result.maxDrawdown;
    m.winRate          = result.winRate;
    m.totalTrades      = result.totalTrades;
    m.initialCapital   = result.initialCapital;
    m.finalCapital     = result.finalCapital;
    m.benchmarkReturn  = result.benchmark.totalReturn;
    m.benchmarkSharpe  = result.benchmark.sharpeRatio;
    m.alpha            = result.alphaVsBenchmark;
    m.sortinoRatio           = stats.sortinoRatio;
    m.calmarRatio            = stats.calmarRatio;
    m.profitFactor           = stats.profitFactor;
    m.averageExposurePct     = stats.averageExposurePct;
    m.turnoverAnnualized     = stats.turnoverAnnualized;
    m.metricDefinitionsVersion = stats.metricDefinitionsVersion;
    m.statisticsJson =
        QString::fromUtf8(QJsonDocument(stats.toJson(1)).toJson(QJsonDocument::Compact));
    return m;
}

} // namespace Backtest
