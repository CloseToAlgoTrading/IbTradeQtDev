#ifndef PIPELINE_ROUTERMARKETDATAACCESSOR_H
#define PIPELINE_ROUTERMARKETDATAACCESSOR_H

#include "IMarketDataAccessor.h"

namespace IBComm {
class MarketDataRouter;
}

namespace Pipeline {

/// Live: last quote from IBComm::MarketDataRouter cache (same tick stream as the runner).
class RouterMarketDataAccessor : public IMarketDataAccessor {
public:
    explicit RouterMarketDataAccessor(IBComm::MarketDataRouter* router);

    std::optional<MarketTick> lastTick(const QString& symbol) const override;

private:
    IBComm::MarketDataRouter* m_router = nullptr;
};

} // namespace Pipeline

#endif
