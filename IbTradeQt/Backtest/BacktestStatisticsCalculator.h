#ifndef BACKTEST_BACKTESTSTATISTICSCALCULATOR_H
#define BACKTEST_BACKTESTSTATISTICSCALCULATOR_H

#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestStatistics.h"

namespace Backtest {

/// Pure statistics from execution outcome. Does not depend on BacktestReportContext.
class BacktestStatisticsCalculator {
public:
    BacktestStatistics compute(const BacktestResult& result) const;
};

} // namespace Backtest

#endif
