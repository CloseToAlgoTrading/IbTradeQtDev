#ifndef DATAMANAGEMENT_IBHISTORICALBARSCOVERAGESYNCPROVIDER_H
#define DATAMANAGEMENT_IBHISTORICALBARSCOVERAGESYNCPROVIDER_H

#include "DataManagement/IHistoricalBarsCoverageSyncProvider.h"

class CBrokerDataProvider;

namespace DataManagement {

class IBHistoricalBarsCoverageSyncProvider : public IHistoricalBarsCoverageSyncProvider {
public:
    explicit IBHistoricalBarsCoverageSyncProvider(CBrokerDataProvider* broker,
                                                  int fetchTimeoutMs = 60000);

    QString providerId() const override;
    bool canSync(const HistoricalBarsDatasetKey& key) const override;

    std::optional<SyncCoverageResult> syncCoverage(const HistoricalBarsDatasetKey& key,
                                                   const QDateTime& requestedFromUtc,
                                                   const QDateTime& requestedToUtc,
                                                   BacktestMarketDataRepository& repo,
                                                   QString* errorMessage) override;

private:
    CBrokerDataProvider* m_broker = nullptr;
    int m_fetchTimeoutMs = 60000;
};

} // namespace DataManagement

#endif
