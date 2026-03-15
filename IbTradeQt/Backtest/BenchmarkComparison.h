#ifndef BACKTEST_BENCHMARKCOMPARISON_H
#define BACKTEST_BENCHMARKCOMPARISON_H

#include <QVector>
#include <cmath>
#include "Backtest/BacktestResult.h"
#include "Backtest/LedgerSnapshot.h"

namespace Backtest {

// Computes buy-and-hold benchmark statistics from a series of daily close prices.
//
// Usage:
//   BenchmarkComparison cmp;
//   cmp.setInitialCapital(100000.0);
//   for (const auto& bar : benchmarkBars)
//       cmp.addBar(bar.symbol, bar.timestamp, bar.close);
//   BenchmarkResult result = cmp.compute("SPY");
class BenchmarkComparison {
public:
    struct BarEntry {
        QDateTime   timestamp;
        double      close = 0.0;
    };

    void setInitialCapital(double capital) { m_initialCapital = capital; }

    void addClose(const QDateTime& ts, double close) {
        m_bars.append({ts, close});
    }

    BenchmarkResult compute(const QString& symbol) const {
        BenchmarkResult r;
        r.symbol = symbol;

        if (m_bars.size() < 2) return r;

        r.startPrice = m_bars.first().close;
        r.endPrice   = m_bars.last().close;

        if (r.startPrice <= 0.0) return r;

        r.totalReturn = (r.endPrice - r.startPrice) / r.startPrice;

        // Annualise using actual calendar days
        const double years = static_cast<double>(
            m_bars.first().timestamp.daysTo(m_bars.last().timestamp)) / 365.25;

        if (years > 0.0) {
            r.annualizedReturn = std::pow(1.0 + r.totalReturn, 1.0 / years) - 1.0;
        }

        // Build normalised equity curve and compute max drawdown + Sharpe
        const double shares = m_initialCapital / r.startPrice;
        double peak = m_initialCapital;
        double maxDD = 0.0;

        QVector<double> dailyReturns;
        dailyReturns.reserve(m_bars.size() - 1);

        for (int i = 0; i < m_bars.size(); ++i) {
            const double value = shares * m_bars[i].close;

            LedgerSnapshot snap;
            snap.timestamp      = m_bars[i].timestamp;
            snap.portfolioValue = value;
            snap.cash           = 0.0;
            snap.unrealizedPnl  = value - m_initialCapital;
            snap.realizedPnl    = 0.0;
            r.equityCurve.append(snap);

            if (value > peak) peak = value;
            const double dd = (peak - value) / peak;
            if (dd > maxDD) maxDD = dd;

            if (i > 0) {
                const double prev = shares * m_bars[i - 1].close;
                if (prev > 0.0)
                    dailyReturns.append((value - prev) / prev);
            }
        }

        r.maxDrawdown = maxDD;

        // Sharpe ratio (annualised, risk-free = 0)
        if (!dailyReturns.isEmpty()) {
            double mean = 0.0;
            for (double d : dailyReturns) mean += d;
            mean /= dailyReturns.size();

            double variance = 0.0;
            for (double d : dailyReturns) variance += (d - mean) * (d - mean);
            variance /= dailyReturns.size();

            const double stddev = std::sqrt(variance);
            if (stddev > 0.0)
                r.sharpeRatio = (mean / stddev) * std::sqrt(252.0);
        }

        return r;
    }

private:
    double              m_initialCapital = 100'000.0;
    QVector<BarEntry>   m_bars;
};

} // namespace Backtest

#endif // BACKTEST_BENCHMARKCOMPARISON_H
