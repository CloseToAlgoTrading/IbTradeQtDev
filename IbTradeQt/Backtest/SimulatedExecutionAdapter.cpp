#include "SimulatedExecutionAdapter.h"
#include "Pipeline/IHistoricalRead.h"
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

void SimulatedExecutionAdapter::setHistoricalFillFallback(Pipeline::IHistoricalRead* historical,
                                                          const QString& resolution,
                                                          const QString& dataSourceId)
{
    m_histFallback       = historical;
    m_histResolution     = resolution;
    m_histDataSourceId   = dataSourceId;
}

std::optional<Pipeline::MarketTick> SimulatedExecutionAdapter::resolveTickForSymbol(
    const QString& symNorm) const
{
    const QDateTime toUtc = m_clock ? m_clock->now().toUTC() : QDateTime::currentDateTimeUtc();
    const QDateTime from  = toUtc.addYears(-2);

    auto tickFromHistoricalClose = [&](const Pipeline::HistoricalBarSnapshot& b) {
        Pipeline::MarketTick t;
        t.symbol    = symNorm;
        t.bid       = b.close;
        t.ask       = b.close;
        t.timestamp = b.timestamp;
        return t;
    };

    // Prefer IHistoricalRead when configured: same getBars(...) as SimpleRebalanceBlock sizing
    // (last bar close in [from, toUtc]). Replay ticks can diverge if the clock instant does not
    // exactly match tick timestamps (QDateTime equality) or if cache vs replay ordering differs.
    if (m_histFallback) {
        const QVector<Pipeline::HistoricalBarSnapshot> bars =
            m_histFallback->getBars(symNorm, m_histResolution, m_histDataSourceId, from, toUtc);
        if (!bars.isEmpty()) {
            const auto& b = bars.last();
            if (b.close > 0.0)
                return tickFromHistoricalClose(b);
        }
    }

    if (m_priceStore->hasTick(symNorm)) {
        const Pipeline::MarketTick storeTick = m_priceStore->lastTick(symNorm);
        if (storeTick.timestamp.toUTC() == toUtc)
            return storeTick;
    }

    if (m_priceStore->hasTick(symNorm))
        return m_priceStore->lastTick(symNorm);

    return std::nullopt;
}

Expected<Ports::OrderResult, Error> SimulatedExecutionAdapter::placeOrder(
    const Pipeline::ExecutionIntent& intent)
{
    const QString symNorm = intent.symbol.trimmed().toUpper();
    if (symNorm.isEmpty()) {
        return make_unexpected(Error{
            ErrorCode::InvalidArgument,
            "Empty symbol in ExecutionIntent",
            "SimulatedExecutionAdapter::placeOrder"
        });
    }

    if (m_fillTiming == FillTiming::SignalOnClose_FillNextBarOpen) {
        Pipeline::ExecutionIntent q = intent;
        q.symbol                    = symNorm;
        m_pendingOrders.append(q);

        Ports::OrderResult result;
        result.orderId   = m_nextOrderId++;
        result.symbol    = symNorm;
        result.quantity  = intent.quantity;
        result.status    = "Queued";
        result.timestamp = m_clock->now();
        return result;
    }

    // Immediate fill (SignalOnTick_FillAtBidAsk or SignalOnClose_FillAtClose)
    auto tickOpt = resolveTickForSymbol(symNorm);
    if (!tickOpt) {
        return make_unexpected(Error{
            ErrorCode::NotFound,
            "No price available for symbol: " + symNorm.toStdString(),
            "SimulatedExecutionAdapter::placeOrder"
        });
    }

    Pipeline::ExecutionIntent intentNorm = intent;
    intentNorm.symbol                      = symNorm;
    Pipeline::MarketTick tick            = *tickOpt;
    // Ledger MTM reads MarketPriceStore; seed store when fill used historical fallback
    m_priceStore->onTick(tick);
    FilledOrder fill = executeFill(intentNorm, tick);

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

    for (Pipeline::ExecutionIntent intent : toFill) {
        const QString symNorm = intent.symbol.trimmed().toUpper();
        intent.symbol         = symNorm;

        // Use the provided openTick if symbol matches, otherwise fall back to price store / history
        Pipeline::MarketTick tick = openTick;
        const QString openSym     = openTick.symbol.trimmed().toUpper();
        if (symNorm != openSym) {
            auto tickOpt = resolveTickForSymbol(symNorm);
            if (!tickOpt) {
                qCWarning(lcSimExec) << "SimulatedExecutionAdapter: no price for" << symNorm
                                     << "— skipping pending order";
                continue;
            }
            tick = *tickOpt;
        }

        m_priceStore->onTick(tick);
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
    fill.marketPrice   = tick.mid();
    fill.fillPrice     = computeFillPrice(intent, tick);
    fill.timestamp     = m_clock->now();
    fill.correlationId = intent.correlationId;
    fill.commission    = 0.0;
    fill.fees          = 0.0;
    return fill;
}

} // namespace Backtest
