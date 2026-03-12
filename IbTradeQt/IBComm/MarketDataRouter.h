#ifndef MARKETDATAROUTER_H
#define MARKETDATAROUTER_H

#include <QObject>
#include <QDateTime>
#include <QMap>
#include <QMetaType>

namespace IBComm {

struct TickByTickTrade {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double price MEMBER price)
    Q_PROPERTY(double size MEMBER size)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp)
    Q_PROPERTY(QString exchange MEMBER exchange)
public:
    QString symbol;
    double price = 0.0;
    double size = 0.0;
    QDateTime timestamp;
    QString exchange;
};

struct MarketTick {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double bid MEMBER bid)
    Q_PROPERTY(double ask MEMBER ask)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp)

public:
    QString symbol;
    double bid = 0.0;
    double ask = 0.0;
    double volume = 0.0;
    QDateTime timestamp;
    int reqId = 0;

    double mid() const { return (bid + ask) / 2.0; }
};

class MarketDataRouter : public QObject {
    Q_OBJECT

public:
    explicit MarketDataRouter(QObject* parent = nullptr)
        : QObject(parent) {}

    void onTickPrice(int reqId, const QString& symbol, double bid, double ask) {
        MarketTick t;
        t.symbol = symbol;
        t.bid = bid;
        t.ask = ask;
        t.timestamp = QDateTime::currentDateTime();
        t.reqId = reqId;

        m_lastPriceCache[symbol] = t;

        emit tick(t);
    }

    void onTickSize(int reqId, const QString& symbol, double volume) {
        Q_UNUSED(reqId)
        if (m_lastPriceCache.contains(symbol)) {
            m_lastPriceCache[symbol].volume = volume;
        }
        emit tickSizeUpdate(symbol, volume);
    }

    void onBarComplete(int reqId, const QString& symbol, const QDateTime& timestamp) {
        Q_UNUSED(reqId)
        emit barClose(symbol, timestamp);
    }

    MarketTick lastPrice(const QString& symbol) const {
        return m_lastPriceCache.value(symbol);
    }

    void onTickByTickAllLast(int reqId, const QString& symbol,
                             double price, double size,
                             const QDateTime& timestamp, const QString& exchange)
    {
        Q_UNUSED(reqId)
        TickByTickTrade trade;
        trade.symbol = symbol;
        trade.price = price;
        trade.size = size;
        trade.timestamp = timestamp;
        trade.exchange = exchange;
        emit tickByTickTrade(trade);
    }

signals:
    void tick(const IBComm::MarketTick& tick);
    void barClose(const QString& symbol, const QDateTime& timestamp);
    void tickSizeUpdate(const QString& symbol, double volume);
    void tickByTickTrade(const IBComm::TickByTickTrade& trade);
    void connectionError(const QString& message);

private:
    QMap<QString, MarketTick> m_lastPriceCache;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::MarketTick)
Q_DECLARE_METATYPE(IBComm::TickByTickTrade)

#endif // MARKETDATAROUTER_H
