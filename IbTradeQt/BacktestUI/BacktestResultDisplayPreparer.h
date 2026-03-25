#ifndef BACKTESTUI_BACKTESTRESULTDISPLAYPREPARER_H
#define BACKTESTUI_BACKTESTRESULTDISPLAYPREPARER_H

#include "Backtest/BacktestDataTypes.h"
#include "Backtest/BacktestSummaryFormatter.h"
#include "Backtest/LedgerSnapshot.h"
#include "DB/dbdatatypes.h"

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

namespace BacktestUI {

/// Heavy lifting for backtest result panels (runs off the GUI thread).
struct PreparedBacktestDisplay {
    QVector<Backtest::LedgerSnapshot> strategyPnl;
    QVector<Backtest::LedgerSnapshot> benchmarkPnl;
    QString                           benchmarkSymbol;
    double                            initialCapital = 0;
    QVector<Backtest::BacktestSummaryRow> summaryRows;
    bool                              threeColSummary   = false;
    QString                           benchmarkColumnHeader;
    QMap<QString, QList<DbHistoricalBar>> histBars;
    QList<DbBacktestTrade>            dbFills;
    QVector<Backtest::FilledOrder>    tradeLog;
};

PreparedBacktestDisplay prepareBacktestDisplay(const Backtest::BacktestLoadedRun& run,
                                               const QString& resolvedBenchmarkSymbol,
                                               const QJsonObject& pipelineForSummary);

} // namespace BacktestUI

#endif
