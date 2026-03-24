#include "Backtest/EquityCurvePnl.h"

namespace Backtest {

QVector<LedgerSnapshot> equityCurveToPnlSeries(const QVector<LedgerSnapshot>& curve,
                                                double initialCapital)
{
    QVector<LedgerSnapshot> out;
    out.reserve(curve.size());
    for (const LedgerSnapshot& s : curve) {
        LedgerSnapshot p = s;
        p.portfolioValue = s.portfolioValue - initialCapital;
        out.append(p);
    }
    return out;
}

} // namespace Backtest
