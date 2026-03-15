#ifndef BACKTEST_SIMULATEDEXECUTIONADAPTER_H
#define BACKTEST_SIMULATEDEXECUTIONADAPTER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include "Ports/IOrderExecutionPort.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/MarketPriceStore.h"
#include "Common/IClock.h"

namespace Backtest {

// Responsible for order lifecycle only: accepting ExecutionIntent, computing
// fill price from MarketPriceStore, and notifying SimulatedLedger via filled().
// Does NOT cache prices or track positions.
class SimulatedExecutionAdapter : public QObject, public Ports::IOrderExecutionPort {
    Q_OBJECT
public:
    explicit SimulatedExecutionAdapter(FillModelType fillModel,
                                       double slippageBps,
                                       FillTiming fillTiming,
                                       MarketPriceStore* priceStore,
                                       IClock* clock,
                                       QObject* parent = nullptr);

    // IOrderExecutionPort
    Expected<Ports::OrderResult, Error> placeOrder(
        const Pipeline::ExecutionIntent& intent) override;

    Expected<void, Error> cancelOrder(int orderId) override;

    Expected<Ports::OrderResult, Error> getOrderStatus(int orderId) override;

    // Called by BacktestSession replay loop at the start of each new tick.
    // Flushes any pending orders queued during the previous barClose event.
    void onNextTickOpen(const IBComm::MarketTick& openTick);

    const QVector<FilledOrder>& filledOrders() const { return m_filledOrders; }

    void reset();

signals:
    void filled(const Backtest::FilledOrder& fill);

private:
    double computeFillPrice(const Pipeline::ExecutionIntent& intent,
                            const IBComm::MarketTick& tick) const;

    FilledOrder executeFill(const Pipeline::ExecutionIntent& intent,
                            const IBComm::MarketTick& tick);

    FillModelType                       m_fillModel;
    double                              m_slippageBps;
    FillTiming                          m_fillTiming;
    MarketPriceStore*                   m_priceStore;
    IClock*                             m_clock;
    QVector<Pipeline::ExecutionIntent>  m_pendingOrders;
    QVector<FilledOrder>                m_filledOrders;
    QMap<int, FilledOrder>              m_orderById;
    int                                 m_nextOrderId = 1;
};

} // namespace Backtest

#endif // BACKTEST_SIMULATEDEXECUTIONADAPTER_H
