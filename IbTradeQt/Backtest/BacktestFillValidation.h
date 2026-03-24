#ifndef BACKTEST_BACKTESTFILLVALIDATION_H
#define BACKTEST_BACKTESTFILLVALIDATION_H

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVector>

#include "Backtest/BacktestResult.h"
#include "IBComm/HistoricalDataRouter.h"

namespace Backtest {

struct FillHistMismatch {
    QString   symbol;
    QDateTime tradeUtc;
    double    fillPrice   = 0.0;
    double    histClose   = 0.0;
    double    absDiff     = 0.0;
    double    tolerance   = 0.0;
};

/// Compare each fill to cached historical close on the session day (same rule as momentum E2E tests).
QVector<FillHistMismatch> fillPriceVsHistoricalCloseMismatches(
    const BacktestResult&                               result,
    const QMap<QString, QVector<IBComm::HistoricalBar>>& barsBySymbol);

} // namespace Backtest

#endif
