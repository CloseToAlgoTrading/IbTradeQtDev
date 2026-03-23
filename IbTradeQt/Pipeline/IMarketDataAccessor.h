#ifndef PIPELINE_IMARKETDATAACCESSOR_H
#define PIPELINE_IMARKETDATAACCESSOR_H

#include <optional>
#include <QString>
#include "Contracts.h"

namespace Pipeline {

/// Read-only access to last streamed quotes (Option C: fed by router/replayer → runner).
class IMarketDataAccessor {
public:
    virtual ~IMarketDataAccessor() = default;

    virtual std::optional<MarketTick> lastTick(const QString& symbol) const = 0;
};

} // namespace Pipeline

#endif // PIPELINE_IMARKETDATAACCESSOR_H
