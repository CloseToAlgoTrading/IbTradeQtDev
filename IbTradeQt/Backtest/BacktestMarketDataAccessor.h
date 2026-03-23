#ifndef BACKTEST_BACKTESTMARKETDATAACCESSOR_H
#define BACKTEST_BACKTESTMARKETDATAACCESSOR_H

#include "../Pipeline/IMarketDataAccessor.h"
#include "MarketPriceStore.h"

namespace Backtest {

/// `IMarketDataAccessor` backed by `MarketPriceStore` (backtest replay path).
class BacktestMarketDataAccessor : public Pipeline::IMarketDataAccessor {
public:
    explicit BacktestMarketDataAccessor(MarketPriceStore* store);

    std::optional<Pipeline::MarketTick> lastTick(const QString& symbol) const override;

private:
    MarketPriceStore* m_store = nullptr;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTMARKETDATAACCESSOR_H
