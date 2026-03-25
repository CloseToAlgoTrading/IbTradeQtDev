#ifndef DATAMANAGEMENT_HISTORICALBARSCOVERAGESYNCREGISTRY_H
#define DATAMANAGEMENT_HISTORICALBARSCOVERAGESYNCREGISTRY_H

#include "DataManagement/DataManagementTypes.h"
#include <memory>

class QNetworkAccessManager;

namespace DataManagement {

class IHistoricalBarsCoverageSyncProvider;

/// Routes `key.dataSourceId` to a provider implementation. Phase A: `"yahoo"` only.
/// Longer term, provenance id may map to a provider family without equating to `providerId()`.
class HistoricalBarsCoverageSyncRegistry {
public:
    /// Returns nullptr when no provider is registered for this provenance id (Phase A: non-yahoo).
    static std::unique_ptr<IHistoricalBarsCoverageSyncProvider> makeProviderForKey(
        const HistoricalBarsDatasetKey& key, QNetworkAccessManager* networkManager,
        int yahooFetchTimeoutMs = 60000);
};

} // namespace DataManagement

#endif
