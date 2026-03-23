#ifndef BACKTEST_SIMULATEDEXECUTIONADAPTER_H
#define BACKTEST_SIMULATEDEXECUTIONADAPTER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <optional>
#include "Ports/IOrderExecutionPort.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/MarketPriceStore.h"
#include "Pipeline/Contracts.h"
#include "Common/IClock.h"

namespace Pipeline {
class IHistoricalRead;
}

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

    /// Resolves fill price: when the replay clock matches the store's last tick for the symbol,
    /// that tick wins (same bar close the pipeline just saw). Otherwise use historical bars,
    /// then any stale store tick (e.g. casing mismatch until normalized).
    void setHistoricalFillFallback(Pipeline::IHistoricalRead* historical,
                                   const QString& resolution,
                                   const QString& dataSourceId);

    // IOrderExecutionPort
    Expected<Ports::OrderResult, Error> placeOrder(
        const Pipeline::ExecutionIntent& intent) override;

    Expected<void, Error> cancelOrder(int orderId) override;

    Expected<Ports::OrderResult, Error> getOrderStatus(int orderId) override;

    Expected<void, Error> cancelAllPending() override;

    // Called by BacktestSession replay loop at the start of each new tick.
    // Flushes any pending orders queued during the previous barClose event.
    void onNextTickOpen(const Pipeline::MarketTick& openTick);

    const QVector<FilledOrder>& filledOrders() const { return m_filledOrders; }

    void reset();

signals:
    void filled(const Backtest::FilledOrder& fill);

private:
    std::optional<Pipeline::MarketTick> resolveTickForSymbol(const QString& symNorm) const;

    double computeFillPrice(const Pipeline::ExecutionIntent& intent,
                            const Pipeline::MarketTick& tick) const;

    FilledOrder executeFill(const Pipeline::ExecutionIntent& intent,
                            const Pipeline::MarketTick& tick);

    FillModelType                       m_fillModel;
    double                              m_slippageBps;
    FillTiming                          m_fillTiming;
    MarketPriceStore*                   m_priceStore;
    IClock*                             m_clock;
    Pipeline::IHistoricalRead*          m_histFallback = nullptr;
    QString                             m_histResolution;
    QString                             m_histDataSourceId;
    QVector<Pipeline::ExecutionIntent>  m_pendingOrders;
    QVector<FilledOrder>                m_filledOrders;
    QMap<int, FilledOrder>              m_orderById;
    int                                 m_nextOrderId = 1;
};

} // namespace Backtest

#endif // BACKTEST_SIMULATEDEXECUTIONADAPTER_H
