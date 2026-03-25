#ifndef DATAMANAGEMENT_YAHOOHISTORICALBARSCOVERAGESYNCPROVIDER_H
#define DATAMANAGEMENT_YAHOOHISTORICALBARSCOVERAGESYNCPROVIDER_H

#include "DataManagement/IHistoricalBarsCoverageSyncProvider.h"
#include "DataManagement/YahooHistoricalBarsCoveragePlanner.h"

class QNetworkAccessManager;

namespace DataManagement {

class YahooHistoricalBarsCoverageSyncProvider : public IHistoricalBarsCoverageSyncProvider {
public:
    explicit YahooHistoricalBarsCoverageSyncProvider(QNetworkAccessManager* networkManager,
                                                     int yahooFetchTimeoutMs = 60000);

    QString providerId() const override;

    bool canSync(const HistoricalBarsDatasetKey& key) const override;

    std::optional<SyncCoverageResult> syncCoverage(const HistoricalBarsDatasetKey& key,
                                                   const QDateTime& requestedFromUtc,
                                                   const QDateTime& requestedToUtc,
                                                   BacktestMarketDataRepository& repo,
                                                   QString* errorMessage) override;

private:
    QNetworkAccessManager*             m_networkManager = nullptr;
    int                                m_yahooTimeoutMs = 60000;
    YahooHistoricalBarsCoveragePlanner m_planner;
};

} // namespace DataManagement

#endif
