#ifndef MARKETDATAROUTER_H
#define MARKETDATAROUTER_H

#include <QObject>
#include <QDateTime>
#include <QMap>
#include <QMetaType>

namespace IBComm {

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

    void onBarComplete(int reqId, const QString& symbol, const QDateTime& timestamp) {
        Q_UNUSED(reqId)
        emit barClose(symbol, timestamp);
    }

    MarketTick lastPrice(const QString& symbol) const {
        return m_lastPriceCache.value(symbol);
    }

signals:
    void tick(const IBComm::MarketTick& tick);
    void barClose(const QString& symbol, const QDateTime& timestamp);
    void connectionError(const QString& message);

private:
    QMap<QString, MarketTick> m_lastPriceCache;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::MarketTick)

#endif // MARKETDATAROUTER_H
