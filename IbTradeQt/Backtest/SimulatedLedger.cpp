#include "SimulatedLedger.h"
#include <QDebug>

namespace Backtest {

SimulatedLedger::SimulatedLedger(double initialCapital,
                                 MarketPriceStore* priceStore,
                                 QObject* parent)
    : QObject(parent)
    , m_cash(initialCapital)
    , m_initialCapital(initialCapital)
    , m_priceStore(priceStore)
{
    Q_ASSERT(priceStore);
}

Expected<Ports::PositionRow, Error> SimulatedLedger::getPosition(
    int /*strategyId*/, const QString& symbol)
{
    if (m_positions.contains(symbol)) {
        return m_positions[symbol];
    }
    // Return a zero position row rather than an error — callers treat "not found" as flat
    Ports::PositionRow row;
    row.symbol = symbol;
    row.quantity = 0.0;
    row.avgCost = 0.0;
    return row;
}

Expected<QVector<Ports::PositionRow>, Error> SimulatedLedger::getAllPositions(
    int /*strategyId*/)
{
    QVector<Ports::PositionRow> result;
    result.reserve(m_positions.size());
    for (const auto& row : m_positions) {
        result.append(row);
    }
    return result;
}

Expected<void, Error> SimulatedLedger::updatePosition(
    const Ports::PositionRow& /*position*/)
{
    Q_ASSERT_X(false,
               "SimulatedLedger::updatePosition",
               "updatePosition() is disabled in backtest mode. "
               "Position state must only change through onFill().");
    return make_unexpected(Error{
        ErrorCode::ConfigurationError,
        "updatePosition() is disabled in backtest mode",
        "SimulatedLedger"
    });
}

void SimulatedLedger::onFill(const FilledOrder& fill)
{
    applyFill(fill);
}

void SimulatedLedger::onBarClose(const QString& /*symbol*/, const QDateTime& ts)
{
    m_lastBarTimestamp = ts;

    LedgerSnapshot snap;
    snap.timestamp      = ts;
    snap.cash           = m_cash;
    snap.unrealizedPnl  = unrealizedPnl();
    snap.realizedPnl    = m_realizedPnl;
    // Equity = cash + market value of holdings (not cash + unrealized: that double-counts cost)
    double holdingsMtm = 0.0;
    for (auto it = m_positions.cbegin(); it != m_positions.cend(); ++it) {
        const Ports::PositionRow& row = it.value();
        if (qFuzzyIsNull(row.quantity))
            continue;
        double px = row.avgCost;
        if (m_priceStore->hasTick(row.symbol))
            px = m_priceStore->lastTick(row.symbol).mid();
        holdingsMtm += row.quantity * px;
    }
    snap.portfolioValue = m_cash + holdingsMtm;

    emit snapshot(snap);
}

double SimulatedLedger::unrealizedPnl() const
{
    double total = 0.0;
    for (auto it = m_positions.cbegin(); it != m_positions.cend(); ++it) {
        const Ports::PositionRow& row = it.value();
        if (qFuzzyIsNull(row.quantity)) continue;

        if (m_priceStore->hasTick(row.symbol)) {
            double price = m_priceStore->lastTick(row.symbol).mid();
            total += row.quantity * (price - row.avgCost);
        }
    }
    return total;
}

void SimulatedLedger::reset(double initialCapital)
{
    m_cash = initialCapital;
    m_initialCapital = initialCapital;
    m_realizedPnl = 0.0;
    m_positions.clear();
}

void SimulatedLedger::applyFill(const FilledOrder& fill)
{
    const double cost = fill.quantity * fill.fillPrice;
    m_cash -= cost;  // negative for buys, positive for sells

    Ports::PositionRow& row = m_positions[fill.symbol];
    row.symbol = fill.symbol;

    const double prevQty  = row.quantity;
    const double newQty   = prevQty + fill.quantity;

    if (qFuzzyIsNull(prevQty)) {
        // Opening a new position
        row.avgCost = fill.fillPrice;
    } else if ((prevQty > 0 && fill.quantity > 0) ||
               (prevQty < 0 && fill.quantity < 0)) {
        // Adding to existing position — weighted average cost
        row.avgCost = (prevQty * row.avgCost + fill.quantity * fill.fillPrice)
                      / newQty;
    } else {
        // Reducing or reversing position
        double closedQty = qMin(qAbs(fill.quantity), qAbs(prevQty));
        if (prevQty > 0) {
            // Selling long
            m_realizedPnl += closedQty * (fill.fillPrice - row.avgCost);
        } else {
            // Covering short
            m_realizedPnl += closedQty * (row.avgCost - fill.fillPrice);
        }

        if (qFuzzyIsNull(newQty)) {
            row.avgCost = 0.0;
        } else if ((prevQty > 0 && newQty < 0) || (prevQty < 0 && newQty > 0)) {
            // Reversed — new avg cost is the fill price
            row.avgCost = fill.fillPrice;
        }
        // If still same direction, avgCost stays unchanged
    }

    row.quantity = newQty;

    // Remove flat positions to keep the map clean
    if (qFuzzyIsNull(row.quantity)) {
        m_positions.remove(fill.symbol);
    }
}

} // namespace Backtest
