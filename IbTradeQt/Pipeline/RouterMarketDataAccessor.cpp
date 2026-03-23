#include "RouterMarketDataAccessor.h"
#include "../IBComm/MarketDataRouter.h"

namespace Pipeline {

RouterMarketDataAccessor::RouterMarketDataAccessor(IBComm::MarketDataRouter* router)
    : m_router(router)
{
}

std::optional<MarketTick> RouterMarketDataAccessor::lastTick(const QString& symbol) const
{
    if (!m_router || !m_router->hasLastTick(symbol))
        return std::nullopt;
    return m_router->lastPrice(symbol);
}

} // namespace Pipeline
