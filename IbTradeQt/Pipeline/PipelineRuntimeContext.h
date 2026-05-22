#ifndef PIPELINE_PIPELINERUNTIMECONTEXT_H
#define PIPELINE_PIPELINERUNTIMECONTEXT_H

#include <QMap>
#include <QString>

#include "IHistoricalRead.h"
#include "IMarketDataAccessor.h"
#include "HistoricalReadPolicy.h"

class IClock;

namespace Ports {
class IOrderExecutionPort;
class IPositionRepositoryPort;
}

namespace Pipeline {

class IDataSubscriptionPort;

/// Injected into blocks — same abstract ports for backtest and live (hexagonal).
struct PipelineRuntimeContext {
    IMarketDataAccessor*   marketData   = nullptr;
    IHistoricalRead*       historical   = nullptr;
    IDataSubscriptionPort* subscription  = nullptr;
    Ports::IOrderExecutionPort* execution = nullptr;
    Ports::IPositionRepositoryPort* positions = nullptr;
    /// Snapshot of symbol → quantity for this strategy (from `positions` / ledger). Refreshed by
    /// StrategyPipelineRunner before each rebalance / semantic pipeline step so blocks can read
    /// `holdings` via `setRuntimeContext(&m_runtimeContext)` without a separate map parameter.
    QMap<QString, double> holdings;
    /// Strategy-level budget for equal-weight / notional sizing; 0 = unset (blocks use defaults).
    double strategyAllocatedCapital = 0.0;
    /// Backtest / tests: SimulatedClock so blocks resolve prices at replay time, not wall clock.
    /// Live: typically nullptr (use wall time in blocks that need it).
    IClock* clock = nullptr;
    /// Filled by StrategyPipelineRunner::setRuntimeContext from graph.config strategyId.
    int strategyId = 0;
    /// Default policy for historical bar requests. Backtest pre-resolves from controller;
    /// live runner parses from graph config as fallback.
    HistoricalReadPolicy historicalReadPolicyDefault = HistoricalReadPolicy::PreferCache;
    /// True if historicalReadPolicyDefault was explicitly resolved (not just struct default).
    /// Prevents runner fallback from overriding an intentional PreferCache selection.
    bool historicalReadPolicyResolved = false;
    /// Backtest-level historical data selection. When set, blocks that read historical bars use
    /// this instead of their legacy block-local source fields so the run config stays authoritative.
    QString historicalDataSourceId;
    QString historicalResolution;
};

/// Prefer \a ctx->holdings when \a ctx is non-null (runner-filled). Otherwise use \a passed (e.g. tests).
inline const QMap<QString, double>& holdingsForBlocks(
    const PipelineRuntimeContext* ctx,
    const QMap<QString, double>& passed)
{
    if (ctx)
        return ctx->holdings;
    return passed;
}

} // namespace Pipeline

#endif // PIPELINE_PIPELINERUNTIMECONTEXT_H
