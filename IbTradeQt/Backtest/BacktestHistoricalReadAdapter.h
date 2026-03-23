#ifndef BACKTEST_BACKTESTHISTORICALREADADAPTER_H
#define BACKTEST_BACKTESTHISTORICALREADADAPTER_H

#include "../Pipeline/IHistoricalRead.h"

namespace Backtest {

class HistoricalDataManager;

/// Bridges `HistoricalDataManager` to `Pipeline::IHistoricalRead` (same strategy code, backtest endpoint).
class BacktestHistoricalReadAdapter : public Pipeline::IHistoricalRead {
public:
    explicit BacktestHistoricalReadAdapter(HistoricalDataManager* manager);

    QVector<Pipeline::HistoricalBarSnapshot> getBars(
        const QString& symbol,
        const QString& resolution,
        const QString& dataSourceId,
        const QDateTime& from,
        const QDateTime& to) override;

private:
    HistoricalDataManager* m_manager = nullptr;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTHISTORICALREADADAPTER_H
