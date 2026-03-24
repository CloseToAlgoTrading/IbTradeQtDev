#ifndef BACKTEST_EQUITYCURVEPNL_H
#define BACKTEST_EQUITYCURVEPNL_H

#include "Backtest/LedgerSnapshot.h"

#include <QVector>

namespace Backtest {

/// Cumulative P&L series: each point is portfolioValue − initialCapital (same timestamps).
QVector<LedgerSnapshot> equityCurveToPnlSeries(const QVector<LedgerSnapshot>& curve,
                                                double initialCapital);

} // namespace Backtest

#endif
