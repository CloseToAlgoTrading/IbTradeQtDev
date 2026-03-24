#ifndef BACKTEST_BACKTESTSUMMARYFORMATTER_H
#define BACKTEST_BACKTESTSUMMARYFORMATTER_H

#include "Backtest/DataQuality.h"
#include "Backtest/BacktestResult.h"
#include "Pipeline/StrategyRuntimePolicy.h"

#include <QString>
#include <QVector>

namespace Backtest {

struct BacktestSummaryRow {
    QString metric;
    QString strategyValue;
    /// Benchmark column; empty when `hasBenchmark` is false (two-column table).
    QString benchmarkValue;
};

/// Single source of truth for summary statistics rows (Qt table + HTML report).
class BacktestSummaryFormatter {
public:
    static QString dataQualityLabel(DataQuality q);

    /// True when benchmark comparison data exists (symbol, equity curve, or persisted metrics).
    static bool hasBenchmarkComparisonData(const BacktestResult& result);

    /// When benchmark data exists, fills benchmark column; otherwise two columns only.
    static QVector<BacktestSummaryRow> buildRows(const BacktestResult& result,
                                                 const Pipeline::StrategyRuntimePolicy& policy);

    static QString benchmarkColumnHeader(const BacktestResult& result);

    /// Policy for semantic validation HTML: EveryBarClose + EveryNBars(rebalanceEveryNBars).
    static Pipeline::StrategyRuntimePolicy policyForSemanticReport(int rebalanceEveryNBars);

private:
    static QString evaluationCell(const Pipeline::StrategyRuntimePolicy& policy);
    static QString rebalanceCell(const Pipeline::StrategyRuntimePolicy& policy);
    static QString naCell();
};

} // namespace Backtest

#endif
