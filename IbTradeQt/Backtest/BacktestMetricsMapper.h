#ifndef BACKTEST_BACKTESTMETRICSMAPPER_H
#define BACKTEST_BACKTESTMETRICSMAPPER_H

#include "DB/dbdatatypes.h"

namespace Backtest {

struct BacktestResult;
struct BacktestStatistics;

/// Single mapping from execution result + computed statistics to persisted DB row.
DbBacktestMetrics toDbBacktestMetrics(const QString& runId,
                                     const BacktestResult& result,
                                     const BacktestStatistics& stats);

} // namespace Backtest

#endif
