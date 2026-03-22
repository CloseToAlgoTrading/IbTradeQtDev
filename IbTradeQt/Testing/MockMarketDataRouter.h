#ifndef TESTING_MOCKMARKETDATAROUTER_H
#define TESTING_MOCKMARKETDATAROUTER_H

#include <QObject>
#include <QVector>
#include "Pipeline/Contracts.h"

/// Test double matching the pipeline feed contract (`tick` + `ohlcvBar` + optional `tickByTickTrade`).
class MockMarketDataRouter : public QObject {
    Q_OBJECT

public:
    explicit MockMarketDataRouter(QObject* parent = nullptr)
        : QObject(parent) {}

    void simulateTick(const QString& symbol, double bid, double ask,
                      const QDateTime& timestamp = QDateTime())
    {
        Pipeline::MarketTick marketTick;
        marketTick.symbol = symbol;
        marketTick.bid = bid;
        marketTick.ask = ask;
        marketTick.timestamp = timestamp.isValid()
            ? timestamp : QDateTime::currentDateTimeUtc();
        m_emittedTicks.push_back(marketTick);
        emit tick(marketTick);
    }

    void simulateTick(const Pipeline::MarketTick& marketTick) {
        m_emittedTicks.push_back(marketTick);
        emit tick(marketTick);
    }

    void simulateOhlcvBar(const Pipeline::OHLCVBar& bar) {
        m_emittedBars.append(bar);
        emit ohlcvBar(bar);
    }

    void simulateBarClose(const QString& symbol,
                          const QDateTime& timestamp = QDateTime())
    {
        Pipeline::OHLCVBar bar;
        bar.symbol = symbol;
        bar.timestamp = timestamp.isValid()
            ? timestamp : QDateTime::currentDateTimeUtc();
        simulateOhlcvBar(bar);
    }

    void replayTicks(const QVector<Pipeline::MarketTick>& ticks) {
        for (const auto& t : ticks) {
            simulateTick(t);
        }
    }

    const QVector<Pipeline::MarketTick>& emittedTicks() const {
        return m_emittedTicks;
    }

    const QVector<Pipeline::OHLCVBar>& emittedBars() const { return m_emittedBars; }

    void reset() {
        m_emittedTicks.clear();
        m_emittedBars.clear();
    }

signals:
    void tick(const Pipeline::MarketTick& tick);
    void ohlcvBar(const Pipeline::OHLCVBar& bar);
    void tickByTickTrade(const Pipeline::TickByTickTrade& trade);

private:
    QVector<Pipeline::MarketTick> m_emittedTicks;
    QVector<Pipeline::OHLCVBar> m_emittedBars;
};

#endif // TESTING_MOCKMARKETDATAROUTER_H
