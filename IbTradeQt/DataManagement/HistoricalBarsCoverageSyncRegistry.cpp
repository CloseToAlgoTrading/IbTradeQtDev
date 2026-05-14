#include "DataManagement/HistoricalBarsCoverageSyncRegistry.h"
#include "DataManagement/DataManagementTypes.h"
#include "DataManagement/IHistoricalBarsCoverageSyncProvider.h"
#include "DataManagement/IBHistoricalBarsCoverageSyncProvider.h"
#include "DataManagement/YahooHistoricalBarsCoverageSyncProvider.h"

#include <QNetworkAccessManager>

namespace DataManagement {

std::unique_ptr<IHistoricalBarsCoverageSyncProvider>
HistoricalBarsCoverageSyncRegistry::makeProviderForKey(const HistoricalBarsDatasetKey& key,
                                                       QNetworkAccessManager* networkManager,
                                                       int yahooFetchTimeoutMs,
                                                       CBrokerDataProvider* brokerDataProvider)
{
    if (key.dataSourceId == QLatin1String("yahoo"))
        return std::make_unique<YahooHistoricalBarsCoverageSyncProvider>(networkManager,
                                                                         yahooFetchTimeoutMs);
    if (key.dataSourceId == QLatin1String("ib"))
        return std::make_unique<IBHistoricalBarsCoverageSyncProvider>(brokerDataProvider,
                                                                      yahooFetchTimeoutMs);
    return nullptr;
}

} // namespace DataManagement
