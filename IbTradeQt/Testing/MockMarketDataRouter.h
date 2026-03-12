#ifndef TESTING_MOCKMARKETDATAROUTER_H
#define TESTING_MOCKMARKETDATAROUTER_H

#include <QObject>
#include <QVector>
#include "IBComm/MarketDataRouter.h"

class MockMarketDataRouter : public QObject {
    Q_OBJECT

public:
    explicit MockMarketDataRouter(QObject* parent = nullptr)
        : QObject(parent) {}

    void simulateTick(const QString& symbol, double bid, double ask,
                      const QDateTime& timestamp = QDateTime())
    {
        IBComm::MarketTick marketTick;
        marketTick.symbol = symbol;
        marketTick.bid = bid;
        marketTick.ask = ask;
        marketTick.timestamp = timestamp.isValid()
            ? timestamp : QDateTime::currentDateTimeUtc();
        m_emittedTicks.push_back(marketTick);
        emit tick(marketTick);
    }

    void simulateTick(const IBComm::MarketTick& marketTick) {
        m_emittedTicks.push_back(marketTick);
        emit tick(marketTick);
    }

    void simulateBarClose(const QString& symbol,
                          const QDateTime& timestamp = QDateTime())
    {
        QDateTime ts = timestamp.isValid()
            ? timestamp : QDateTime::currentDateTimeUtc();
        emit barClose(symbol, ts);
    }

    void replayTicks(const QVector<IBComm::MarketTick>& ticks) {
        for (const auto& t : ticks) {
            simulateTick(t);
        }
    }

    const QVector<IBComm::MarketTick>& emittedTicks() const {
        return m_emittedTicks;
    }

    void reset() { m_emittedTicks.clear(); }

signals:
    void tick(const IBComm::MarketTick& tick);
    void barClose(const QString& symbol, const QDateTime& timestamp);

private:
    QVector<IBComm::MarketTick> m_emittedTicks;
};

#endif // TESTING_MOCKMARKETDATAROUTER_H
