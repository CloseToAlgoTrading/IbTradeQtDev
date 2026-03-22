#ifndef BACKTEST_BACKTESTRUNPERSISTENCE_H
#define BACKTEST_BACKTESTRUNPERSISTENCE_H

#include <QString>

namespace Backtest {

struct BacktestRunConfig;

namespace Persistence {

/// Non-empty value for BacktestRuns.strategyId (NOT NULL). Prefer live node id when
/// present; otherwise catalog / definition / scope ids; finally runId-based fallback.
QString resolvedStrategyIdForPersistence(const BacktestRunConfig& config,
                                         const QString& runIdForFallback);

} // namespace Persistence
} // namespace Backtest

#endif // BACKTEST_BACKTESTRUNPERSISTENCE_H
