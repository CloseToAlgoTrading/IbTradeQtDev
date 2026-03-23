#ifndef PIPELINE_PIPELINERUNTIMECONTEXT_H
#define PIPELINE_PIPELINERUNTIMECONTEXT_H

#include "IHistoricalRead.h"
#include "IMarketDataAccessor.h"

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
};

} // namespace Pipeline

#endif // PIPELINE_PIPELINERUNTIMECONTEXT_H
