#include "Backtest/BacktestFillValidation.h"

#include "Backtest/HistoricalBarUtils.h"

#include <cmath>
#include <limits>

namespace Backtest {

QVector<FillHistMismatch> fillPriceVsHistoricalCloseMismatches(
    const BacktestResult&                               result,
    const QMap<QString, QVector<IBComm::HistoricalBar>>& barsBySymbol)
{
    QVector<FillHistMismatch> out;
    for (const auto& t : result.tradeLog) {
        const QString u = t.symbol.trimmed().toUpper();
        const QVector<IBComm::HistoricalBar> series = barsBySymbol.value(u);
        const IBComm::HistoricalBar* pb = barForSessionDay(series, t.timestamp);
        if (!pb) {
            FillHistMismatch m;
            m.symbol     = u;
            m.tradeUtc   = t.timestamp;
            m.fillPrice  = t.fillPrice;
            m.histClose  = 0.0;
            m.absDiff    = std::numeric_limits<double>::infinity();
            m.tolerance  = 0.0;
            out.append(m);
            continue;
        }
        const double histC = pb->close;
        const double diff  = std::abs(t.fillPrice - histC);
        const double tol   = std::max(0.05, 2e-3 * std::abs(histC));
        if (diff > tol) {
            FillHistMismatch m;
            m.symbol    = u;
            m.tradeUtc  = t.timestamp;
            m.fillPrice = t.fillPrice;
            m.histClose = histC;
            m.absDiff   = diff;
            m.tolerance = tol;
            out.append(m);
        }
    }
    return out;
}

} // namespace Backtest
