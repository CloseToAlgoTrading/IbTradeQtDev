#include "Backtest/HistoricalBarUtils.h"

#include "IBComm/HistoricalDataRouter.h"

namespace Backtest {

const IBComm::HistoricalBar* barForSessionDay(const QVector<IBComm::HistoricalBar>& series,
                                              const QDateTime&                    tradeUtc)
{
    const QDate d = tradeUtc.toUTC().date();
    for (int i = series.size() - 1; i >= 0; --i) {
        if (series[i].timestamp.toUTC().date() == d)
            return &series[i];
    }
    for (int i = series.size() - 1; i >= 0; --i) {
        if (series[i].timestamp <= tradeUtc)
            return &series[i];
    }
    return nullptr;
}

} // namespace Backtest
