#ifndef BACKTEST_BACKTESTRESULT_H
#define BACKTEST_BACKTESTRESULT_H

#include <QVector>
#include <QDateTime>
#include "FilledOrder.h"
#include "LedgerSnapshot.h"
#include "DataQuality.h"

namespace Backtest {

struct BenchmarkResult {
    QString     symbol;
    double      totalReturn      = 0.0;   // buy-and-hold return over the period
    double      annualizedReturn = 0.0;
    double      maxDrawdown      = 0.0;
    double      sharpeRatio      = 0.0;
    double      startPrice       = 0.0;
    double      endPrice         = 0.0;
    QVector<LedgerSnapshot> equityCurve;  // normalised to same initial capital
};

struct BacktestResult {
    QVector<FilledOrder>    tradeLog;
    QVector<LedgerSnapshot> equityCurve;

    double  totalReturn      = 0.0;
    double  annualizedReturn = 0.0;
    double  sharpeRatio      = 0.0;
    double  maxDrawdown      = 0.0;
    double  winRate          = 0.0;
    int     totalTrades      = 0;

    QDateTime startDate;
    QDateTime endDate;
    double  initialCapital   = 0.0;
    double  finalCapital     = 0.0;

    // Reserved for future transaction cost modelling — zero in v1
    double  totalCommission  = 0.0;
    double  totalFees        = 0.0;

    DataQuality dataQuality  = DataQuality::SynthesizedOHLC;

    // Benchmark comparison (populated when BacktestConfig::benchmarkSymbol is set)
    BenchmarkResult benchmark;
    double  alphaVsBenchmark = 0.0;   // strategy annualizedReturn - benchmark annualizedReturn
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTRESULT_H
