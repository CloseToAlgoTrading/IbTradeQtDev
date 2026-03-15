#ifndef BACKTEST_SIMULATEDLEDGER_H
#define BACKTEST_SIMULATEDLEDGER_H

#include <QObject>
#include <QMap>
#include "Ports/IPositionRepositoryPort.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/LedgerSnapshot.h"
#include "Backtest/MarketPriceStore.h"

namespace Backtest {

// Single source of truth for all mutable financial state during a backtest:
// cash, positions, average cost, realized P&L, and unrealized P&L.
//
// Implements IPositionRepositoryPort so pipeline risk blocks can query it directly.
// updatePosition() is intentionally disabled — position state must only change
// through onFill() to preserve the single-owner invariant.
class SimulatedLedger : public QObject, public Ports::IPositionRepositoryPort {
    Q_OBJECT
public:
    explicit SimulatedLedger(double initialCapital,
                             MarketPriceStore* priceStore,
                             QObject* parent = nullptr);

    // IPositionRepositoryPort — read side used by pipeline risk blocks
    Expected<Ports::PositionRow, Error> getPosition(
        int strategyId, const QString& symbol) override;

    Expected<QVector<Ports::PositionRow>, Error> getAllPositions(
        int strategyId) override;

    // Disabled in backtest mode — position state must only change through onFill().
    // Calls Q_ASSERT_X and returns an error.
    Expected<void, Error> updatePosition(
        const Ports::PositionRow& position) override;

    // Called by SimulatedExecutionAdapter on each fill
    void onFill(const FilledOrder& fill);

    // Called by BacktestSession replay loop at each bar boundary.
    // Marks all positions to market and emits snapshot().
    void onBarClose(const QString& symbol, const QDateTime& ts);

    // State accessors
    double cash() const { return m_cash; }
    double realizedPnl() const { return m_realizedPnl; }
    double unrealizedPnl() const;
    double portfolioValue() const { return m_cash + unrealizedPnl(); }

    void reset(double initialCapital);

signals:
    // Emitted after each onBarClose() — BacktestMetricsCollector subscribes here
    void snapshot(const Backtest::LedgerSnapshot& snap);

private:
    void applyFill(const FilledOrder& fill);

    double                              m_cash;
    double                              m_initialCapital;
    double                              m_realizedPnl = 0.0;
    QMap<QString, Ports::PositionRow>   m_positions;   // symbol -> row
    MarketPriceStore*                   m_priceStore;
    QDateTime                           m_lastBarTimestamp;
};

} // namespace Backtest

#endif // BACKTEST_SIMULATEDLEDGER_H
