#include "Backtest/BacktestReportModelBuilder.h"

#include <QCoreApplication>
#include <QJsonObject>

namespace Backtest {

BacktestReportModel BacktestReportModelBuilder::build(const BacktestResult& r,
                                                        const BacktestStatistics& stats,
                                                        const BacktestReportContext& ctx) const
{
    BacktestReportModel m;
    m.schemaVersion = 1;

    QJsonObject summary;
    summary[QStringLiteral("totalReturn")]      = r.totalReturn;
    summary[QStringLiteral("annualizedReturn")] = r.annualizedReturn;
    summary[QStringLiteral("sharpeRatio")]      = r.sharpeRatio;
    summary[QStringLiteral("maxDrawdown")]      = r.maxDrawdown;
    summary[QStringLiteral("finalCapital")]     = r.finalCapital;
    summary[QStringLiteral("initialCapital")]   = r.initialCapital;
    summary[QStringLiteral("sortinoRatio")]    = stats.sortinoRatio;
    summary[QStringLiteral("calmarRatio")]     = stats.calmarRatio;
    summary[QStringLiteral("profitFactor")]    = stats.profitFactor;
    summary[QStringLiteral("averageExposurePct")] = stats.averageExposurePct;
    summary[QStringLiteral("turnoverAnnualized")] = stats.turnoverAnnualized;

    QJsonObject enrich;
    enrich[QStringLiteral("hasStrategyBars")] = !ctx.strategyBarsBySymbol.isEmpty();
    enrich[QStringLiteral("windowStartUtc")]  = ctx.windowStartUtc.toString(Qt::ISODate);
    enrich[QStringLiteral("windowEndUtc")]    = ctx.windowEndUtc.toString(Qt::ISODate);

    m.sections[QStringLiteral("summary")]     = summary;
    m.sections[QStringLiteral("enrichment")] = enrich;
    m.sections[QStringLiteral("statistics")] = stats.toJson(1);
    return m;
}

} // namespace Backtest
