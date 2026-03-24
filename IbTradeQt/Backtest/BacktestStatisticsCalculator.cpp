#include "Backtest/BacktestStatisticsCalculator.h"

#include "Backtest/FilledOrder.h"
#include "Backtest/LedgerSnapshot.h"

#include <QDate>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace Backtest {

namespace {

struct LongPosition {
    double qty  = 0.0;
    double cost = 0.0; // average cost per share for long qty
};

QVector<double> dailySimpleReturns(const QVector<LedgerSnapshot>& curve)
{
    QVector<double> out;
    out.reserve(qMax(0, curve.size() - 1));
    for (int i = 1; i < curve.size(); ++i) {
        const double prev = curve[i - 1].portfolioValue;
        const double curr = curve[i].portfolioValue;
        if (prev > 0.0)
            out.append((curr - prev) / prev);
    }
    return out;
}

double mean(const QVector<double>& v)
{
    if (v.isEmpty())
        return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double sampleStdDev(const QVector<double>& v, double m)
{
    if (v.size() < 2)
        return 0.0;
    double var = 0.0;
    for (double x : v) {
        const double d = x - m;
        var += d * d;
    }
    var /= (v.size() - 1);
    return qSqrt(var);
}

double sharpeFromDaily(const QVector<double>& dailyReturns)
{
    if (dailyReturns.size() < 2)
        return 0.0;
    const double m = mean(dailyReturns);
    const double s = sampleStdDev(dailyReturns, m);
    if (qFuzzyIsNull(s))
        return 0.0;
    return (m / s) * qSqrt(252.0);
}

double sortinoFromDaily(const QVector<double>& dailyReturns)
{
    QVector<double> downside;
    downside.reserve(dailyReturns.size());
    for (double r : dailyReturns) {
        if (r < 0.0)
            downside.append(r);
    }
    if (downside.size() < 2) {
        if (downside.isEmpty())
            return 0.0;
        const double m = mean(dailyReturns);
        const double s = qAbs(downside.first());
        return qFuzzyIsNull(s) ? 0.0 : (m / s) * qSqrt(252.0);
    }
    const double dm = mean(downside);
    double var      = 0.0;
    for (double r : downside) {
        const double d = r - dm;
        var += d * d;
    }
    var /= (downside.size() - 1);
    const double ds = qSqrt(var);
    if (qFuzzyIsNull(ds))
        return 0.0;
    const double m = mean(dailyReturns);
    return (m / ds) * qSqrt(252.0);
}

double maxDrawdownFromCurve(const QVector<LedgerSnapshot>& curve)
{
    if (curve.isEmpty())
        return 0.0;
    double peak = curve.first().portfolioValue;
    double maxDD = 0.0;
    for (const auto& s : curve) {
        if (s.portfolioValue > peak)
            peak = s.portfolioValue;
        if (peak > 0.0) {
            const double dd = (peak - s.portfolioValue) / peak;
            if (dd > maxDD)
                maxDD = dd;
        }
    }
    return maxDD;
}

double averageExposurePctFromCurve(const QVector<LedgerSnapshot>& curve)
{
    if (curve.isEmpty())
        return 0.0;
    double sum = 0.0;
    int    n   = 0;
    for (const auto& s : curve) {
        if (s.portfolioValue <= 1e-15)
            continue;
        const double invested = s.portfolioValue - s.cash;
        sum += qMax(0.0, invested) / s.portfolioValue;
        ++n;
    }
    return n > 0 ? (sum / n) : 0.0;
}

double turnoverAnnualizedFrom(const BacktestResult& result)
{
    const auto& curve = result.equityCurve;
    if (curve.isEmpty())
        return 0.0;
    double sumNotional = 0.0;
    for (const FilledOrder& f : result.tradeLog)
        sumNotional += qAbs(f.quantity * f.fillPrice);

    double sumEq = 0.0;
    for (const auto& s : curve)
        sumEq += s.portfolioValue;
    const double meanEq = sumEq / double(curve.size());
    if (meanEq <= 1e-15)
        return 0.0;

    const int nInt = qMax(1, curve.size() - 1);
    const double annualScale = 252.0 / double(nInt);
    return (sumNotional / (2.0 * meanEq)) * annualScale;
}

QJsonArray periodReturnsFromCurve(const QVector<LedgerSnapshot>& curve, bool byYear)
{
    QJsonArray out;
    if (curve.isEmpty())
        return out;

    QMap<QString, QPair<int, int>> bounds;
    for (int i = 0; i < curve.size(); ++i) {
        const QDate d = curve[i].timestamp.date();
        const QString key =
            byYear ? QString::number(d.year()) : d.toString(QStringLiteral("yyyy-MM"));
        if (!bounds.contains(key))
            bounds.insert(key, qMakePair(i, i));
        else
            bounds[key].second = i;
    }

    for (auto it = bounds.constBegin(); it != bounds.constEnd(); ++it) {
        const int a = it.value().first;
        const int b = it.value().second;
        const double v0 = curve[a].portfolioValue;
        const double v1 = curve[b].portfolioValue;
        double       r  = 0.0;
        if (v0 > 1e-15)
            r = (v1 - v0) / v0;
        QJsonObject row;
        row[QStringLiteral("period")] = it.key();
        row[QStringLiteral("return")] = r;
        out.append(row);
    }
    return out;
}

} // namespace

BacktestStatistics BacktestStatisticsCalculator::compute(const BacktestResult& result) const
{
    BacktestStatistics st;
    st.metricDefinitionsVersion = 1;

    const QVector<double> daily = dailySimpleReturns(result.equityCurve);
    st.sharpeRatio              = sharpeFromDaily(daily);
    st.sortinoRatio             = sortinoFromDaily(daily);
    st.maxDrawdown              = maxDrawdownFromCurve(result.equityCurve);

    st.totalReturn      = result.totalReturn;
    st.annualizedReturn = result.annualizedReturn;
    st.winRate          = result.winRate;
    st.totalTrades      = result.totalTrades;

    st.calmarRatio = (st.maxDrawdown > 1e-12) ? (st.annualizedReturn / st.maxDrawdown) : 0.0;

    // Profit factor (long-only FIFO realized P&L on sells)
    double grossProfit = 0.0;
    double grossLoss   = 0.0;
    QHash<QString, LongPosition> longPos;
    for (const FilledOrder& f : result.tradeLog) {
        const QString sym = f.symbol.trimmed().toUpper();
        if (f.quantity > 0.0) {
            LongPosition& p = longPos[sym];
            const double q  = f.quantity;
            const double newQty = p.qty + q;
            if (newQty > 1e-15)
                p.cost = (p.cost * p.qty + f.fillPrice * q) / newQty;
            else
                p.cost = f.fillPrice;
            p.qty = newQty;
        } else if (f.quantity < 0.0) {
            LongPosition& p = longPos[sym];
            double         qRem = -f.quantity;
            while (qRem > 1e-12 && p.qty > 1e-12) {
                const double take = qMin(qRem, p.qty);
                const double pnl  = (f.fillPrice - p.cost) * take;
                if (pnl >= 0.0)
                    grossProfit += pnl;
                else
                    grossLoss += pnl;
                p.qty -= take;
                qRem -= take;
            }
        }
    }
    if (grossLoss < -1e-12)
        st.profitFactor = grossProfit / (-grossLoss);
    else
        st.profitFactor = grossProfit > 1e-12 ? 1.0e6 : 0.0;

    const int n = result.totalTrades;
    if (n > 0) {
        st.expectancy = (result.finalCapital - result.initialCapital) / double(n);
        st.averageTradePnl = st.expectancy;
    }

    st.averageExposurePct   = averageExposurePctFromCurve(result.equityCurve);
    st.turnoverAnnualized   = turnoverAnnualizedFrom(result);
    st.monthlyReturns       = periodReturnsFromCurve(result.equityCurve, false);
    st.yearlyReturns        = periodReturnsFromCurve(result.equityCurve, true);

    return st;
}

} // namespace Backtest
