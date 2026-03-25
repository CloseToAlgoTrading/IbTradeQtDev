#ifndef DATAMANAGEMENT_IHISTORICALBARSCOVERAGESYNCPROVIDER_H
#define DATAMANAGEMENT_IHISTORICALBARSCOVERAGESYNCPROVIDER_H

#include "DataManagement/DataManagementTypes.h"
#include <QString>
#include <optional>

namespace DataManagement {

class BacktestMarketDataRepository;

/// Worker-thread provider for coverage sync. Dependencies belong in the concrete constructor,
/// not on each `syncCoverage` call. Phase B: `IbHistoricalBarsCoverageSyncProvider`.
class IHistoricalBarsCoverageSyncProvider {
public:
    virtual ~IHistoricalBarsCoverageSyncProvider() = default;

    /// Provider family id (e.g. `"yahoo"`). May differ from `key.dataSourceId` in a future registry.
    virtual QString providerId() const = 0;

    virtual bool canSync(const HistoricalBarsDatasetKey& key) const = 0;

    /// Returns a result on success; on failure returns `std::nullopt` and sets \a errorMessage.
    virtual std::optional<SyncCoverageResult> syncCoverage(const HistoricalBarsDatasetKey& key,
                                                           const QDateTime& requestedFromUtc,
                                                           const QDateTime& requestedToUtc,
                                                           BacktestMarketDataRepository& repo,
                                                           QString* errorMessage) = 0;
};

} // namespace DataManagement

#endif
