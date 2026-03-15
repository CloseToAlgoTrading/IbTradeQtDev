#ifndef BACKTEST_BACKTESTMETRICSCOLLECTOR_H
#define BACKTEST_BACKTESTMETRICSCOLLECTOR_H

#include <QObject>
#include <QVector>
#include "Backtest/LedgerSnapshot.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/BacktestResult.h"

namespace Backtest {

// Pure read-only observer. Subscribes to SimulatedLedger::snapshot() and
// SimulatedExecutionAdapter::filled(). Computes all metrics in finalize() only.
// Never writes to any position or cash state.
class BacktestMetricsCollector : public QObject {
    Q_OBJECT
public:
    explicit BacktestMetricsCollector(double initialCapital, QObject* parent = nullptr);

    // Compute all metrics from accumulated snapshots and trades.
    BacktestResult finalize(const QDateTime& start, const QDateTime& end) const;

public slots:
    void onSnapshot(const Backtest::LedgerSnapshot& snap);
    void onFill(const Backtest::FilledOrder& fill);

private:
    double computeSharpe(const QVector<double>& dailyReturns) const;
    double computeMaxDrawdown(const QVector<LedgerSnapshot>& curve) const;

    double                      m_initialCapital;
    QVector<LedgerSnapshot>     m_equityCurve;
    QVector<FilledOrder>        m_trades;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTMETRICSCOLLECTOR_H
