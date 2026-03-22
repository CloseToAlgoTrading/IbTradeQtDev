#include "StrategyPipelineRunner.h"
#include "../IBComm/MarketDataRouter.h"

namespace Pipeline {

void StrategyPipelineRunner::connectToMarketData(IBComm::MarketDataRouter* router)
{
    connectMarketDataFeedQueued(router);
    connectTickByTickFeedQueued(router);
}

} // namespace Pipeline
