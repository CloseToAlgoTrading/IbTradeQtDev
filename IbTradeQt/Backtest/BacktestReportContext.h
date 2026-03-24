#ifndef BACKTEST_BACKTESTREPORTCONTEXT_H
#define BACKTEST_BACKTESTREPORTCONTEXT_H

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVector>

#include "IBComm/HistoricalDataRouter.h"

namespace Backtest {

/// Optional enrichment for HTML/report builders — not used for core KPI math in
/// BacktestStatisticsCalculator.
struct BacktestReportContext {
    QMap<QString, QVector<IBComm::HistoricalBar>> strategyBarsBySymbol;
    QDateTime windowStartUtc;
    QDateTime windowEndUtc;
};

} // namespace Backtest

#endif
