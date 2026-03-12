#ifndef IBCOMM_HISTORICALDATAROUTER_H
#define IBCOMM_HISTORICALDATAROUTER_H

#include <QObject>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QMetaType>

namespace IBComm {

struct HistoricalBar {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp)
    Q_PROPERTY(double open MEMBER open)
    Q_PROPERTY(double high MEMBER high)
    Q_PROPERTY(double low MEMBER low)
    Q_PROPERTY(double close MEMBER close)
    Q_PROPERTY(double volume MEMBER volume)
public:
    QString symbol;
    QDateTime timestamp;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
    int count = 0;
};

class HistoricalDataRouter : public QObject {
    Q_OBJECT
public:
    explicit HistoricalDataRouter(QObject* parent = nullptr) : QObject(parent) {}

    void setReqIdSymbol(int reqId, const QString& symbol) {
        m_reqIdToSymbol[reqId] = symbol;
    }

public slots:
    void onHistoricalBar(int reqId, const QString& dateStr,
                         double open, double high, double low, double close,
                         double volume, int count) {
        HistoricalBar bar;
        bar.symbol = m_reqIdToSymbol.value(reqId);
        bar.timestamp = QDateTime::fromString(dateStr, Qt::ISODate);
        if (!bar.timestamp.isValid()) {
            bar.timestamp = QDateTime::fromString(dateStr, "yyyyMMdd  HH:mm:ss");
        }
        if (!bar.timestamp.isValid()) {
            bar.timestamp = QDateTime::fromString(dateStr, "yyyyMMdd");
        }
        bar.open = open;
        bar.high = high;
        bar.low = low;
        bar.close = close;
        bar.volume = volume;
        bar.count = count;
        m_pendingBars[reqId].append(bar);
        emit historicalBar(reqId, bar);
    }

    void onHistoricalDataEnd(int reqId) {
        auto bars = m_pendingBars.take(reqId);
        QString symbol = m_reqIdToSymbol.take(reqId);
        emit barsReceived(reqId, symbol, bars);
    }

signals:
    void historicalBar(int requestId, const IBComm::HistoricalBar& bar);
    void barsReceived(int requestId, const QString& symbol,
                      const QVector<IBComm::HistoricalBar>& bars);

private:
    QMap<int, QVector<HistoricalBar>> m_pendingBars;
    QMap<int, QString> m_reqIdToSymbol;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::HistoricalBar)

#endif // IBCOMM_HISTORICALDATAROUTER_H
