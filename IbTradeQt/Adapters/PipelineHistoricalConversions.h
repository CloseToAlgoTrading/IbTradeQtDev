#ifndef ADAPTERS_PIPELINEHISTORICALCONVERSIONS_H
#define ADAPTERS_PIPELINEHISTORICALCONVERSIONS_H

#include <QVector>
#include "../IBComm/HistoricalDataRouter.h"
#include "../Pipeline/Contracts.h"

namespace Adapters {

/// Convert broker historical bars to pipeline OHLCV at the adapter boundary (single place).
inline QVector<Pipeline::OHLCVBar> toOhlcvBars(const QVector<IBComm::HistoricalBar>& bars)
{
    QVector<Pipeline::OHLCVBar> out;
    out.reserve(bars.size());
    for (const auto& b : bars) {
        Pipeline::OHLCVBar o;
        o.symbol = b.symbol;
        o.open = b.open;
        o.high = b.high;
        o.low = b.low;
        o.close = b.close;
        o.volume = b.volume;
        o.timestamp = b.timestamp;
        out.append(o);
    }
    return out;
}

} // namespace Adapters

#endif
