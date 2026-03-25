#include "BacktestUI/BacktestResultDisplayPreparer.h"
#include "Backtest/EquityCurvePnl.h"
#include "Pipeline/StrategyRuntimePolicy.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace BacktestUI {

namespace {

QVector<Backtest::LedgerSnapshot> downsampleLedger(const QVector<Backtest::LedgerSnapshot>& c,
                                                  int maxPoints)
{
    if (c.size() <= maxPoints)
        return c;
    const int step = qMax(1, static_cast<int>(c.size()) / maxPoints);
    QVector<Backtest::LedgerSnapshot> out;
    out.reserve(qMin(c.size() / step + 2, maxPoints + 2));
    for (int i = 0; i < c.size(); i += step)
        out.append(c[i]);
    if (!c.isEmpty()
        && (out.isEmpty() || out.last().timestamp != c.last().timestamp))
        out.append(c.last());
    return out;
}

} // namespace

PreparedBacktestDisplay prepareBacktestDisplay(const Backtest::BacktestLoadedRun& run,
                                               const QString& resolvedBenchmarkSymbol,
                                               const QJsonObject& pipelineForSummary)
{
    constexpr int kMaxEquityPoints = 4000;
    constexpr int kMaxCandleBars     = 6000;

    PreparedBacktestDisplay p;
    Backtest::BacktestResult result = run.result;
    if (result.benchmark.symbol.isEmpty() && !resolvedBenchmarkSymbol.isEmpty())
        result.benchmark.symbol = resolvedBenchmarkSymbol;

    p.initialCapital   = result.initialCapital;
    p.benchmarkSymbol  = result.benchmark.symbol;

    const Pipeline::StrategyRuntimePolicy policy =
        Pipeline::StrategyRuntimePolicy::fromJson(
            pipelineForSummary.value(QStringLiteral("runtimePolicy")).toObject());
    p.summaryRows           = Backtest::BacktestSummaryFormatter::buildRows(result, policy);
    p.threeColSummary       = Backtest::BacktestSummaryFormatter::hasBenchmarkComparisonData(result);
    p.benchmarkColumnHeader = Backtest::BacktestSummaryFormatter::benchmarkColumnHeader(result);

    const QVector<Backtest::LedgerSnapshot> stratPnl =
        Backtest::equityCurveToPnlSeries(result.equityCurve, result.initialCapital);
    const QVector<Backtest::LedgerSnapshot> benchPnl =
        Backtest::equityCurveToPnlSeries(result.benchmark.equityCurve, result.initialCapital);
    p.strategyPnl  = downsampleLedger(stratPnl, kMaxEquityPoints);
    p.benchmarkPnl = downsampleLedger(benchPnl, kMaxEquityPoints);

    for (auto it = run.histBars.begin(); it != run.histBars.end(); ++it) {
        QList<DbHistoricalBar> list = it.value();
        if (list.size() <= kMaxCandleBars) {
            p.histBars[it.key()] = std::move(list);
            continue;
        }
        const int step = qMax(1, list.size() / kMaxCandleBars);
        QList<DbHistoricalBar> capped;
        capped.reserve(kMaxCandleBars + 2);
        for (int i = 0; i < list.size(); i += step)
            capped.append(list.at(i));
        if (!list.isEmpty()
            && (capped.isEmpty()
                || capped.last().timestamp != list.last().timestamp))
            capped.append(list.last());
        p.histBars[it.key()] = std::move(capped);
    }

    p.dbFills.reserve(result.tradeLog.size());
    for (const auto& fill : result.tradeLog) {
        DbBacktestTrade t;
        t.runId     = run.record.runId;
        t.symbol    = fill.symbol;
        t.side      = fill.quantity > 0 ? QStringLiteral("BUY") : QStringLiteral("SELL");
        t.quantity  = std::abs(fill.quantity);
        t.fillPrice = fill.fillPrice;
        t.timestamp = fill.timestamp.toUTC().toString(Qt::ISODate);
        p.dbFills.append(t);
    }
    p.tradeLog = result.tradeLog;
    return p;
}

} // namespace BacktestUI
