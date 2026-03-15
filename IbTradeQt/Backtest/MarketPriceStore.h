#ifndef BACKTEST_MARKETPRICESTORE_H
#define BACKTEST_MARKETPRICESTORE_H

#include <QObject>
#include <QMap>
#include "IBComm/MarketDataRouter.h"

namespace Backtest {

// Single source of price truth during a backtest session.
// Both SimulatedExecutionAdapter (fill price) and SimulatedLedger (mark-to-market)
// read from here. No other component caches prices independently.
class MarketPriceStore : public QObject {
    Q_OBJECT
public:
    explicit MarketPriceStore(QObject* parent = nullptr) : QObject(parent) {}

    IBComm::MarketTick lastTick(const QString& symbol) const {
        return m_cache.value(symbol);
    }

    bool hasTick(const QString& symbol) const {
        return m_cache.contains(symbol);
    }

    void clear() { m_cache.clear(); }

public slots:
    void onTick(const IBComm::MarketTick& tick) {
        m_cache[tick.symbol] = tick;
    }

private:
    QMap<QString, IBComm::MarketTick> m_cache;
};

} // namespace Backtest

#endif // BACKTEST_MARKETPRICESTORE_H
