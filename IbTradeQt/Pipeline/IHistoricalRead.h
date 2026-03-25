#ifndef PIPELINE_IHISTORICALREAD_H
#define PIPELINE_IHISTORICALREAD_H

#include "HistoricalReadPolicy.h"
#include <QDateTime>
#include <QString>
#include <QVector>

namespace Pipeline {

struct HistoricalBarSnapshot {
    QDateTime timestamp;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
};

/// Historical bars for alpha/research (backtest: DB/cache; live: broker adapter — same interface).
class IHistoricalRead {
public:
    virtual ~IHistoricalRead() = default;

    virtual QVector<HistoricalBarSnapshot> getBars(
        const QString& symbol,
        const QString& resolution,
        const QString& dataSourceId,
        const QDateTime& from,
        const QDateTime& to,
        HistoricalReadPolicy policy = HistoricalReadPolicy::PreferCache) = 0;
};

} // namespace Pipeline

#endif // PIPELINE_IHISTORICALREAD_H
