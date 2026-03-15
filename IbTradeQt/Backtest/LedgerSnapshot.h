#ifndef BACKTEST_LEDGERSNAPSHOT_H
#define BACKTEST_LEDGERSNAPSHOT_H

#include <QDateTime>

namespace Backtest {

// Point-in-time snapshot of ledger state, emitted after each bar close.
// BacktestMetricsCollector subscribes to these to build the equity curve.
struct LedgerSnapshot {
    QDateTime   timestamp;
    double      portfolioValue = 0.0;  // cash + unrealizedPnl
    double      cash           = 0.0;
    double      unrealizedPnl  = 0.0;
    double      realizedPnl    = 0.0;
};

} // namespace Backtest

#endif // BACKTEST_LEDGERSNAPSHOT_H
