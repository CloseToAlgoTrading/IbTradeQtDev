#ifndef MARKETDATAROUTER_H
#define MARKETDATAROUTER_H

#include <QObject>
#include <QDateTime>
#include <QMap>
#include <QMetaType>
#include "../Pipeline/Contracts.h"

namespace IBComm {

/// Legacy IB-shaped tick (still used outside the pipeline domain).
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

struct GenericTick {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(int tickType MEMBER tickType)
    Q_PROPERTY(double value MEMBER value)
public:
    QString symbol;
    int tickType = 0;
    double value = 0.0;
};

struct StringTick {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(int tickType MEMBER tickType)
    Q_PROPERTY(QString value MEMBER value)
public:
    QString symbol;
    int tickType = 0;
    QString value;
};

struct OptionComputation {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(int tickType MEMBER tickType)
    Q_PROPERTY(double impliedVol MEMBER impliedVol)
    Q_PROPERTY(double delta MEMBER delta)
    Q_PROPERTY(double optPrice MEMBER optPrice)
    Q_PROPERTY(double gamma MEMBER gamma)
    Q_PROPERTY(double vega MEMBER vega)
    Q_PROPERTY(double theta MEMBER theta)
    Q_PROPERTY(double undPrice MEMBER undPrice)
public:
    QString symbol;
    int tickType = 0;
    double impliedVol = 0.0;
    double delta = 0.0;
    double optPrice = 0.0;
    double pvDividend = 0.0;
    double gamma = 0.0;
    double vega = 0.0;
    double theta = 0.0;
    double undPrice = 0.0;
};

class MarketDataRouter : public QObject {
    Q_OBJECT

public:
    explicit MarketDataRouter(QObject* parent = nullptr)
        : QObject(parent) {}

    void onTickPrice(int reqId, const QString& symbol, double bid, double ask) {
        Pipeline::MarketTick t;
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

    /// Authoritative completed bar from IB real-time bars (or adapter).
    void onOhlcvBarComplete(const Pipeline::OHLCVBar& bar) {
        emit ohlcvBar(bar);
    }

    Pipeline::MarketTick lastPrice(const QString& symbol) const {
        return m_lastPriceCache.value(symbol);
    }

    void onTickByTickAllLast(int reqId, const QString& symbol,
                             double price, double size,
                             const QDateTime& timestamp, const QString& exchange)
    {
        Q_UNUSED(reqId)
        Pipeline::TickByTickTrade trade;
        trade.symbol = symbol;
        trade.price = price;
        trade.size = size;
        trade.timestamp = timestamp;
        trade.exchange = exchange;
        emit tickByTickTrade(trade);
    }

    void onTickGeneric(int reqId, int tickType, double value) {
        GenericTick gt;
        QString sym = symbolForReqId(reqId);
        gt.symbol = sym.isEmpty() ? QString::number(reqId) : sym;
        gt.tickType = tickType;
        gt.value = value;
        emit tickGenericReceived(gt);
    }

    void onTickString(int reqId, int tickType, const QString& value) {
        StringTick st;
        QString sym = symbolForReqId(reqId);
        st.symbol = sym.isEmpty() ? QString::number(reqId) : sym;
        st.tickType = tickType;
        st.value = value;
        emit tickStringReceived(st);
    }

    void onTickOptionComputation(int reqId, int tickType, double impliedVol, double delta,
                                  double optPrice, double pvDividend, double gamma,
                                  double vega, double theta, double undPrice) {
        OptionComputation oc;
        oc.symbol = symbolForReqId(reqId);
        oc.tickType = tickType;
        oc.impliedVol = impliedVol;
        oc.delta = delta;
        oc.optPrice = optPrice;
        oc.pvDividend = pvDividend;
        oc.gamma = gamma;
        oc.vega = vega;
        oc.theta = theta;
        oc.undPrice = undPrice;
        emit optionComputationReceived(oc);
    }

    void registerReqIdSymbol(int reqId, const QString& symbol) {
        m_reqIdToSymbol[reqId] = symbol;
    }

    void onSubscriptionRestarted() { emit subscriptionRestarted(); }
    void onSubscriptionError(int reqId, int errorCode, const QString& msg) {
        emit subscriptionError(reqId, errorCode, msg);
    }

signals:
    void tick(const Pipeline::MarketTick& tick);
    void ohlcvBar(const Pipeline::OHLCVBar& bar);
    void tickSizeUpdate(const QString& symbol, double volume);
    void tickByTickTrade(const Pipeline::TickByTickTrade& trade);
    void connectionError(const QString& message);
    void subscriptionRestarted();
    void subscriptionError(int reqId, int errorCode, const QString& msg);
    void tickGenericReceived(const IBComm::GenericTick& tick);
    void tickStringReceived(const IBComm::StringTick& tick);
    void optionComputationReceived(const IBComm::OptionComputation& data);

private:
    QString symbolForReqId(int reqId) const { return m_reqIdToSymbol.value(reqId); }
    QMap<QString, Pipeline::MarketTick> m_lastPriceCache;
    QMap<int, QString> m_reqIdToSymbol;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::MarketTick)
Q_DECLARE_METATYPE(IBComm::TickByTickTrade)
Q_DECLARE_METATYPE(IBComm::GenericTick)
Q_DECLARE_METATYPE(IBComm::StringTick)
Q_DECLARE_METATYPE(IBComm::OptionComputation)

#endif // MARKETDATAROUTER_H
