#include "Backtest/BacktestStatistics.h"

#include <QJsonObject>

namespace Backtest {

QJsonObject BacktestStatistics::toJson(int schemaVersion) const
{
    QJsonObject o;
    o[QStringLiteral("schemaVersion")]            = schemaVersion;
    o[QStringLiteral("metricDefinitionsVersion")] = metricDefinitionsVersion;
    o[QStringLiteral("totalReturn")]              = totalReturn;
    o[QStringLiteral("annualizedReturn")]         = annualizedReturn;
    o[QStringLiteral("sharpeRatio")]               = sharpeRatio;
    o[QStringLiteral("sortinoRatio")]            = sortinoRatio;
    o[QStringLiteral("maxDrawdown")]             = maxDrawdown;
    o[QStringLiteral("calmarRatio")]             = calmarRatio;
    o[QStringLiteral("winRate")]                 = winRate;
    o[QStringLiteral("totalTrades")]             = totalTrades;
    o[QStringLiteral("profitFactor")]            = profitFactor;
    o[QStringLiteral("expectancy")]              = expectancy;
    o[QStringLiteral("averageTradePnl")]         = averageTradePnl;
    o[QStringLiteral("averageExposurePct")]      = averageExposurePct;
    o[QStringLiteral("turnoverAnnualized")]      = turnoverAnnualized;
    o[QStringLiteral("monthlyReturns")]          = monthlyReturns;
    o[QStringLiteral("yearlyReturns")]           = yearlyReturns;
    return o;
}

} // namespace Backtest
