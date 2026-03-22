#include "SimulatedExecutionAdapter.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSimExec, "backtest.exec")

namespace Backtest {

SimulatedExecutionAdapter::SimulatedExecutionAdapter(
    FillModelType fillModel,
    double slippageBps,
    FillTiming fillTiming,
    MarketPriceStore* priceStore,
    IClock* clock,
    QObject* parent)
    : QObject(parent)
    , m_fillModel(fillModel)
    , m_slippageBps(slippageBps)
    , m_fillTiming(fillTiming)
    , m_priceStore(priceStore)
    , m_clock(clock)
{
    Q_ASSERT(priceStore);
    Q_ASSERT(clock);
}

Expected<Ports::OrderResult, Error> SimulatedExecutionAdapter::placeOrder(
    const Pipeline::ExecutionIntent& intent)
{
    if (intent.symbol.isEmpty()) {
        return make_unexpected(Error{
            ErrorCode::InvalidArgument,
            "Empty symbol in ExecutionIntent",
            "SimulatedExecutionAdapter::placeOrder"
        });
    }

    if (m_fillTiming == FillTiming::SignalOnClose_FillNextBarOpen) {
        m_pendingOrders.append(intent);

        Ports::OrderResult result;
        result.orderId   = m_nextOrderId++;
        result.symbol    = intent.symbol;
        result.quantity  = intent.quantity;
        result.status    = "Queued";
        result.timestamp = m_clock->now();
        return result;
    }

    // Immediate fill (SignalOnTick_FillAtBidAsk or SignalOnClose_FillAtClose)
    if (!m_priceStore->hasTick(intent.symbol)) {
        return make_unexpected(Error{
            ErrorCode::NotFound,
            "No price available for symbol: " + intent.symbol.toStdString(),
            "SimulatedExecutionAdapter::placeOrder"
        });
    }

    Pipeline::MarketTick tick = m_priceStore->lastTick(intent.symbol);
    FilledOrder fill = executeFill(intent, tick);

    m_filledOrders.append(fill);
    m_orderById[fill.orderId] = fill;
    emit filled(fill);

    Ports::OrderResult result;
    result.orderId   = fill.orderId;
    result.symbol    = fill.symbol;
    result.quantity  = fill.quantity;
    result.status    = "Filled";
    result.timestamp = fill.timestamp;
    return result;
}

Expected<void, Error> SimulatedExecutionAdapter::cancelOrder(int orderId)
{
    // Remove from pending queue if present
    for (int i = 0; i < m_pendingOrders.size(); ++i) {
        if (m_pendingOrders[i].correlationId.contains(QString::number(orderId))) {
            m_pendingOrders.removeAt(i);
            return {};
        }
    }
    return make_unexpected(Error{
        ErrorCode::NotFound,
        "Order not found: " + std::to_string(orderId),
        "SimulatedExecutionAdapter::cancelOrder"
    });
}

Expected<Ports::OrderResult, Error> SimulatedExecutionAdapter::getOrderStatus(int orderId)
{
    if (m_orderById.contains(orderId)) {
        const FilledOrder& fill = m_orderById[orderId];
        Ports::OrderResult result;
        result.orderId   = fill.orderId;
        result.symbol    = fill.symbol;
        result.quantity  = fill.quantity;
        result.status    = "Filled";
        result.timestamp = fill.timestamp;
        return result;
    }
    return make_unexpected(Error{
        ErrorCode::NotFound,
        "Order not found: " + std::to_string(orderId),
        "SimulatedExecutionAdapter::getOrderStatus"
    });
}

Expected<void, Error> SimulatedExecutionAdapter::cancelAllPending()
{
    m_pendingOrders.clear();
    return {};
}

void SimulatedExecutionAdapter::onNextTickOpen(const Pipeline::MarketTick& openTick)
{
    if (m_pendingOrders.isEmpty()) return;

    QVector<Pipeline::ExecutionIntent> toFill;
    toFill.swap(m_pendingOrders);

    for (const Pipeline::ExecutionIntent& intent : toFill) {
        // Use the provided openTick if symbol matches, otherwise fall back to price store
        Pipeline::MarketTick tick = openTick;
        if (intent.symbol != openTick.symbol) {
            if (!m_priceStore->hasTick(intent.symbol)) {
                qCWarning(lcSimExec) << "SimulatedExecutionAdapter: no price for" << intent.symbol
                           << "— skipping pending order";
                continue;
            }
            tick = m_priceStore->lastTick(intent.symbol);
        }

        FilledOrder fill = executeFill(intent, tick);
        m_filledOrders.append(fill);
        m_orderById[fill.orderId] = fill;
        emit filled(fill);
    }
}

void SimulatedExecutionAdapter::reset()
{
    m_pendingOrders.clear();
    m_filledOrders.clear();
    m_orderById.clear();
    m_nextOrderId = 1;
}

double SimulatedExecutionAdapter::computeFillPrice(
    const Pipeline::ExecutionIntent& intent,
    const Pipeline::MarketTick& tick) const
{
    const bool isBuy = (intent.quantity > 0);

    switch (m_fillModel) {
    case FillModelType::Instant:
    case FillModelType::MidPrice:
        return tick.mid();

    case FillModelType::BidAsk:
        return isBuy ? tick.ask : tick.bid;

    case FillModelType::SlippageBps: {
        const double base   = isBuy ? tick.ask : tick.bid;
        const double factor = isBuy
            ? (1.0 + m_slippageBps / 10000.0)
            : (1.0 - m_slippageBps / 10000.0);
        return base * factor;
    }
    }
    return tick.mid();
}

FilledOrder SimulatedExecutionAdapter::executeFill(
    const Pipeline::ExecutionIntent& intent,
    const Pipeline::MarketTick& tick)
{
    FilledOrder fill;
    fill.orderId       = m_nextOrderId++;
    fill.symbol        = intent.symbol;
    fill.quantity      = intent.quantity;
    fill.fillPrice     = computeFillPrice(intent, tick);
    fill.timestamp     = m_clock->now();
    fill.correlationId = intent.correlationId;
    fill.commission    = 0.0;
    fill.fees          = 0.0;
    return fill;
}

} // namespace Backtest
