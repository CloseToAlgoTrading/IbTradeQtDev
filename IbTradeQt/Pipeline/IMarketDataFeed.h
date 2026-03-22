#ifndef PIPELINE_IMARKETDATAFEED_H
#define PIPELINE_IMARKETDATAFEED_H

#include "Contracts.h"

namespace Pipeline {

/// Duck-typed feed contract (same signal names across adapters):
///   void tick(const Pipeline::MarketTick&)
///   void ohlcvBar(const Pipeline::OHLCVBar&)
/// Optional for live/tests: void tickByTickTrade(const Pipeline::TickByTickTrade&)
///
/// Implementations: IBComm::MarketDataRouter, MarketDataReplayer, MockMarketDataRouter.
/// StrategyPipelineRunner::connectMarketDataFeed() wires these into ingestTick / ingestOhlcvBar.

} // namespace Pipeline

#endif
