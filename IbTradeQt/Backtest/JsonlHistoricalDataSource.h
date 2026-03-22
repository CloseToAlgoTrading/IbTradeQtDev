#ifndef BACKTEST_JSONLHISTORICALDATASOURCE_H
#define BACKTEST_JSONLHISTORICALDATASOURCE_H

#include <QObject>
#include "Backtest/IHistoricalDataSource.h"
#include "Replay/MarketDataReplayer.h"

namespace Backtest {

// Synchronous historical data source backed by a JSONL recording file.
// Wraps MarketDataReplayer::loadRecording() and re-emits its ticks as
// tickLoaded() signals. Supports real tick-level replay (DataQuality::RealTicks).
//
// The dataPath in BacktestConfig is the path to the .jsonl file.
class JsonlHistoricalDataSource : public IHistoricalDataSource {
    Q_OBJECT
public:
    explicit JsonlHistoricalDataSource(const QString& filePath,
                                       QObject* parent = nullptr)
        : IHistoricalDataSource(parent)
        , m_filePath(filePath)
    {}

    QString sourceId() const override { return "jsonl"; }
    bool requiresLiveBroker() const override { return false; }

    bool supportsResolution(BarResolution r) const override {
        // JSONL recordings contain real ticks — all resolutions are supported
        Q_UNUSED(r);
        return true;
    }

    void requestBars(const QStringList& symbols,
                     const QDateTime& from,
                     const QDateTime& to,
                     BarResolution /*resolution*/) override
    {
        MarketDataReplayer replayer;
        if (!replayer.loadRecording(m_filePath)) {
            emit loadFailed("Failed to load JSONL recording: " + m_filePath);
            return;
        }

        for (const Pipeline::MarketTick& tick : replayer.ticks()) {
            if (!symbols.isEmpty() && !symbols.contains(tick.symbol)) continue;
            if (from.isValid() && tick.timestamp < from) continue;
            if (to.isValid()   && tick.timestamp > to)   continue;
            emit tickLoaded(tick);
        }

        emit loadFinished();
    }

private:
    QString m_filePath;
};

} // namespace Backtest

#endif // BACKTEST_JSONLHISTORICALDATASOURCE_H
