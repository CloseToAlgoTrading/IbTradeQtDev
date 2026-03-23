#include "BacktestMarketDataAccessor.h"

namespace Backtest {

BacktestMarketDataAccessor::BacktestMarketDataAccessor(MarketPriceStore* store)
    : m_store(store)
{}

std::optional<Pipeline::MarketTick> BacktestMarketDataAccessor::lastTick(const QString& symbol) const
{
    if (!m_store || !m_store->hasTick(symbol))
        return std::nullopt;
    return m_store->lastTick(symbol);
}

} // namespace Backtest
