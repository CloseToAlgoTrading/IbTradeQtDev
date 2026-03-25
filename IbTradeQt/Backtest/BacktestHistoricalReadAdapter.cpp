#include "BacktestHistoricalReadAdapter.h"

#include "HistoricalDataManager.h"
#include "IBComm/HistoricalDataRouter.h"

namespace Backtest {

BacktestHistoricalReadAdapter::BacktestHistoricalReadAdapter(HistoricalDataManager* manager)
    : m_manager(manager)
{}

QVector<Pipeline::HistoricalBarSnapshot> BacktestHistoricalReadAdapter::getBars(
    const QString& symbol,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    Pipeline::HistoricalReadPolicy policy)
{
    QVector<Pipeline::HistoricalBarSnapshot> out;
    if (!m_manager)
        return out;

    const QVector<IBComm::HistoricalBar> raw =
        m_manager->getBars(symbol, resolution, dataSourceId, from, to, nullptr, {}, policy);
    out.reserve(raw.size());
    for (const auto& b : raw) {
        Pipeline::HistoricalBarSnapshot s;
        s.timestamp = b.timestamp;
        s.open = b.open;
        s.high = b.high;
        s.low = b.low;
        s.close = b.close;
        s.volume = b.volume;
        out.append(s);
    }
    return out;
}

} // namespace Backtest
