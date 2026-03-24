#ifndef BACKTEST_BACKTESTSTATISTICS_H
#define BACKTEST_BACKTESTSTATISTICS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace Backtest {

/// Extended numeric statistics (DTO). Formula authority: BacktestStatisticsCalculator + docs/BACKTEST_METRICS_SPEC.md
struct BacktestStatistics {
    double totalReturn       = 0.0;
    double annualizedReturn  = 0.0;
    double sharpeRatio       = 0.0;
    double sortinoRatio      = 0.0;
    double maxDrawdown       = 0.0;
    double calmarRatio       = 0.0;
    double winRate           = 0.0;
    int    totalTrades       = 0;
    double profitFactor      = 0.0;
    double expectancy        = 0.0;
    double averageTradePnl   = 0.0;

    /// Mean over snapshots of (portfolioValue − cash) / portfolioValue when PV > 0 (long market value / equity).
    double averageExposurePct = 0.0;
    /// Annualized one-way turnover: (Σ|fill notional| / (2·mean equity)) · (252 / N), N = equity return intervals.
    double turnoverAnnualized = 0.0;

    /// Per calendar month: `{ "period": "yyyy-MM", "return": r }` simple return first→last snapshot in month.
    QJsonArray monthlyReturns;
    /// Per calendar year: `{ "period": "yyyy", "return": r }`.
    QJsonArray yearlyReturns;

    int metricDefinitionsVersion = 2; // v2: exposure, turnover, monthly/yearly period returns

    QJsonObject toJson(int schemaVersion = 1) const;
};

} // namespace Backtest

#endif
