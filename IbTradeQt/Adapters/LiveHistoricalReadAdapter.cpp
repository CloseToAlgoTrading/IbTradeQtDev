#include "LiveHistoricalReadAdapter.h"

#include "Adapters/IBHistoricalDataFetcher.h"
#include "IBComm/cbrokerdataprovider.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcLiveHistRead, "pipeline.liveHistoricalRead")

namespace Pipeline {

CBrokerDataProvider* LiveHistoricalReadAdapter::s_broker = nullptr;

LiveHistoricalReadAdapter::LiveHistoricalReadAdapter(QObject* parent)
    : QObject(parent)
{}

void LiveHistoricalReadAdapter::setBrokerDataProvider(CBrokerDataProvider* provider)
{
    s_broker = provider;
}

CBrokerDataProvider* LiveHistoricalReadAdapter::brokerDataProvider()
{
    return s_broker;
}

QVector<HistoricalBarSnapshot> LiveHistoricalReadAdapter::getBars(
    const QString& symbol,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    HistoricalReadPolicy policy)
{
    Q_UNUSED(dataSourceId);
    Q_UNUSED(policy);

    QVector<HistoricalBarSnapshot> out;
    Adapters::IBHistoricalDataFetcher fetcher(s_broker);
    Adapters::IBHistoricalFetchRequest req;
    req.symbols = QStringList{symbol};
    req.resolution = resolution;
    req.fromUtc = from.toUTC();
    req.toUtc = to.toUTC();
    auto result = fetcher.fetch(req);
    if (!result.errorMessage.isEmpty()) {
        qCWarning(lcLiveHistRead) << "LiveHistoricalReadAdapter:" << result.errorMessage;
        return out;
    }

    out.reserve(result.bars.size());
    for (const auto& bar : result.bars) {
        HistoricalBarSnapshot s;
        s.timestamp = bar.timestamp;
        s.open = bar.open;
        s.high = bar.high;
        s.low = bar.low;
        s.close = bar.close;
        s.volume = bar.volume;
        out.append(s);
    }

    return out;
}

} // namespace Pipeline
