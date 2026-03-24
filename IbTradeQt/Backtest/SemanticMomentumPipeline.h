#ifndef BACKTEST_SEMANTICMOMENTUMPIPELINE_H
#define BACKTEST_SEMANTICMOMENTUMPIPELINE_H

#include <QJsonObject>
#include <QStringList>

namespace Backtest {

/// Single source for semantic momentum E2E pipeline JSON (top-N momentum, simple rebalance, max risk, market execution).
QJsonObject buildSemanticMomentumPipeline(const QStringList& universe,
                                          double              initialCapital,
                                          double              maxPositionShares,
                                          int                 rebalanceEveryNBars,
                                          int                 momentumPeriod    = 20,
                                          double              momentumThreshold = 0.001,
                                          int                 topN              = 2);

} // namespace Backtest

#endif
