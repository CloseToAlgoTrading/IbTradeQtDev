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

    QVector<HistoricalBarSnapshot> getBars(
        const QString& symbol,
        const QString& resolution,
        const QString& dataSourceId,
        const QDateTime& from,
        const QDateTime& to) override;

private:
    static CBrokerDataProvider* s_broker;
};

} // namespace Pipeline

#endif // ADAPTERS_LIVEHISTORICALREADADAPTER_H
