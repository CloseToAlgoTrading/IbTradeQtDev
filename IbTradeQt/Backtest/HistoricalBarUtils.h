#ifndef BACKTEST_HISTORICALBARUTILS_H
#define BACKTEST_HISTORICALBARUTILS_H

#include <QDateTime>
#include <QVector>

namespace IBComm {
struct HistoricalBar;
}

namespace Backtest {

/// Match a trade timestamp to the bar for that session day (UTC calendar day), else nearest prior bar.
const IBComm::HistoricalBar* barForSessionDay(const QVector<IBComm::HistoricalBar>& series,
                                               const QDateTime&                    tradeUtc);

} // namespace Backtest

#endif
