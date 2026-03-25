#ifndef ADAPTERS_LIVEHISTORICALREADADAPTER_H
#define ADAPTERS_LIVEHISTORICALREADADAPTER_H

#include "../Pipeline/IHistoricalRead.h"
#include <QObject>

class CBrokerDataProvider;

namespace Pipeline {

/// Live `IHistoricalRead` via IB historical request + `HistoricalDataRouter::barsReceived`
/// (same interface as `BacktestHistoricalReadAdapter`; blocks stay mode-agnostic).
class LiveHistoricalReadAdapter : public QObject, public IHistoricalRead {
    Q_OBJECT

public:
    explicit LiveHistoricalReadAdapter(QObject* parent = nullptr);

    static void setBrokerDataProvider(CBrokerDataProvider* provider);
    static CBrokerDataProvider* brokerDataProvider();

    /// Live implementation: all policies currently collapse to direct broker historical request.
    /// - PreferCache: No persistent cache exists in live mode; fetches from broker.
    /// - RefreshFromSource: Same as PreferCache (broker-only fetch).
    /// - SourceOnly: Same as PreferCache (broker-only fetch).
    /// If a future live-side cache is added, RefreshFromSource would trigger cache update.
    QVector<HistoricalBarSnapshot> getBars(
        const QString& symbol,
        const QString& resolution,
        const QString& dataSourceId,
        const QDateTime& from,
        const QDateTime& to,
        HistoricalReadPolicy policy = HistoricalReadPolicy::PreferCache) override;

private:
    static CBrokerDataProvider* s_broker;
};

} // namespace Pipeline

#endif // ADAPTERS_LIVEHISTORICALREADADAPTER_H
