#include "BacktestMetricsCollector.h"
#include <QtMath>
#include <numeric>

namespace Backtest {

BacktestMetricsCollector::BacktestMetricsCollector(double initialCapital, QObject* parent)
    : QObject(parent)
    , m_initialCapital(initialCapital)
{}

void BacktestMetricsCollector::onSnapshot(const Backtest::LedgerSnapshot& snap)
{
    m_equityCurve.append(snap);
}

void BacktestMetricsCollector::onFill(const Backtest::FilledOrder& fill)
{
    m_trades.append(fill);
}

BacktestResult BacktestMetricsCollector::finalize(
    const QDateTime& start, const QDateTime& end) const
{
    BacktestResult result;
    result.tradeLog      = m_trades;
    result.equityCurve   = m_equityCurve;
    result.initialCapital = m_initialCapital;
    result.startDate     = start;
    result.endDate       = end;
    result.totalTrades   = m_trades.size();

    if (m_equityCurve.isEmpty()) {
        result.finalCapital = m_initialCapital;
        return result;
    }

    result.finalCapital = m_equityCurve.last().portfolioValue;

    if (m_initialCapital > 0.0) {
        result.totalReturn = (result.finalCapital - m_initialCapital) / m_initialCapital;
    }

    // Annualized return using 252 trading days convention
    const qint64 totalDays = start.daysTo(end);
    if (totalDays > 0) {
        const double years = totalDays / 365.25;
        if (years > 0.0 && result.totalReturn > -1.0) {
            result.annualizedReturn = qPow(1.0 + result.totalReturn, 1.0 / years) - 1.0;
        }
    }

    // Daily returns from equity curve snapshots
    QVector<double> dailyReturns;
    dailyReturns.reserve(m_equityCurve.size());
    for (int i = 1; i < m_equityCurve.size(); ++i) {
        const double prev = m_equityCurve[i - 1].portfolioValue;
        const double curr = m_equityCurve[i].portfolioValue;
        if (prev > 0.0) {
            dailyReturns.append((curr - prev) / prev);
        }
    }

    result.sharpeRatio  = computeSharpe(dailyReturns);
    result.maxDrawdown  = computeMaxDrawdown(m_equityCurve);

    // Win rate — a trade is a win if the round-trip P&L > 0
    // We pair fills by symbol: each buy followed by a sell (or vice versa)
    // For simplicity in v1 we use a per-fill sign: buy fill followed by higher-price sell
    int wins = 0;
    QMap<QString, FilledOrder> openBuys;
    for (const FilledOrder& fill : m_trades) {
        if (fill.quantity > 0) {
            openBuys[fill.symbol] = fill;
        } else if (fill.quantity < 0 && openBuys.contains(fill.symbol)) {
            const FilledOrder& buy = openBuys[fill.symbol];
            if (fill.fillPrice > buy.fillPrice) ++wins;
            openBuys.remove(fill.symbol);
        }
    }
    if (result.totalTrades > 0) {
        result.winRate = static_cast<double>(wins) / result.totalTrades;
    }

    // Commission / fees are zero in v1 — fields reserved
    result.totalCommission = 0.0;
    result.totalFees       = 0.0;

    return result;
}

double BacktestMetricsCollector::computeSharpe(
    const QVector<double>& dailyReturns) const
{
    if (dailyReturns.size() < 2) return 0.0;

    const double mean = std::accumulate(dailyReturns.begin(), dailyReturns.end(), 0.0)
                        / dailyReturns.size();

    double variance = 0.0;
    for (double r : dailyReturns) {
        variance += (r - mean) * (r - mean);
    }
    variance /= (dailyReturns.size() - 1);

    const double stddev = qSqrt(variance);
    if (qFuzzyIsNull(stddev)) return 0.0;

    return (mean / stddev) * qSqrt(252.0);
}

double BacktestMetricsCollector::computeMaxDrawdown(
    const QVector<LedgerSnapshot>& curve) const
{
    if (curve.isEmpty()) return 0.0;

    double peak = curve.first().portfolioValue;
    double maxDD = 0.0;

    for (const LedgerSnapshot& snap : curve) {
        if (snap.portfolioValue > peak) {
            peak = snap.portfolioValue;
        }
        if (peak > 0.0) {
            const double dd = (peak - snap.portfolioValue) / peak;
            if (dd > maxDD) maxDD = dd;
        }
    }
    return maxDD;
}

} // namespace Backtest
