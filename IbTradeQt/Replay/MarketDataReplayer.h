#ifndef REPLAY_MARKETDATAREPLAYER_H
#define REPLAY_MARKETDATAREPLAYER_H

#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include "IBComm/MarketDataRouter.h"
#include "Pipeline/Contracts.h"

class MarketDataReplayer : public QObject {
    Q_OBJECT

public:
    explicit MarketDataReplayer(QObject* parent = nullptr)
        : QObject(parent) {}

    explicit MarketDataReplayer(const QString& filename, QObject* parent = nullptr)
        : QObject(parent)
    {
        loadRecording(filename);
    }

    bool loadRecording(const QString& filename) {
        m_ticks.clear();
        m_expectedIntents.clear();
        m_blockGraphConfig = QJsonObject();

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "MarketDataReplayer: Failed to open" << filename;
            return false;
        }

        while (!file.atEnd()) {
            QByteArray line = file.readLine().trimmed();
            if (line.isEmpty()) continue;

            QJsonParseError parseErr;
            QJsonDocument doc = QJsonDocument::fromJson(line, &parseErr);
            if (parseErr.error != QJsonParseError::NoError) {
                qWarning() << "MarketDataReplayer: Skipping malformed line:" << parseErr.errorString();
                continue;
            }

            QJsonObject obj = doc.object();
            QString type = obj["type"].toString();

            if (type == "MarketTick") {
                IBComm::MarketTick t;
                t.symbol = obj["symbol"].toString();
                t.bid = obj["bid"].toDouble();
                t.ask = obj["ask"].toDouble();
                t.timestamp = QDateTime::fromString(
                    obj["timestamp"].toString(), Qt::ISODateWithMs);
                t.reqId = obj["reqId"].toInt();
                m_ticks.append(t);
            }
            else if (type == "ExecutionIntent") {
                Pipeline::ExecutionIntent ei;
                ei.symbol = obj["symbol"].toString();
                ei.quantity = obj["quantity"].toDouble();
                ei.orderType = static_cast<Pipeline::ExecutionIntent::OrderType>(
                    obj["orderType"].toInt());
                ei.correlationId = obj["correlationId"].toString();
                ei.riskApproval = obj["riskApproval"].toString();
                ei.timestamp = QDateTime::fromString(
                    obj["timestamp"].toString(), Qt::ISODateWithMs);
                if (obj.contains("limitPrice")) {
                    ei.limitPrice = obj["limitPrice"].toDouble();
                }
                m_expectedIntents.append(ei);
            }
            else if (type == "BarClose") {
                BarCloseEvent bc;
                bc.symbol = obj["symbol"].toString();
                bc.timestamp = QDateTime::fromString(
                    obj["timestamp"].toString(), Qt::ISODateWithMs);
                m_barCloses.append(bc);
            }
            else if (type == "BlockGraphConfig") {
                m_blockGraphConfig = obj["config"].toObject();
            }
        }

        return !m_ticks.isEmpty();
    }

    void replay() {
        int barIdx = 0;
        for (const auto& t : m_ticks) {
            emit tick(t);

            while (barIdx < m_barCloses.size()
                   && m_barCloses[barIdx].timestamp <= t.timestamp) {
                emit barClose(m_barCloses[barIdx].symbol,
                              m_barCloses[barIdx].timestamp);
                ++barIdx;
            }
        }
        while (barIdx < m_barCloses.size()) {
            emit barClose(m_barCloses[barIdx].symbol,
                          m_barCloses[barIdx].timestamp);
            ++barIdx;
        }
    }

    void replayUntil(const QDateTime& until) {
        for (const auto& t : m_ticks) {
            if (t.timestamp > until) break;
            emit tick(t);
        }
    }

    const QVector<IBComm::MarketTick>& ticks() const { return m_ticks; }
    const QVector<Pipeline::ExecutionIntent>& expectedIntents() const {
        return m_expectedIntents;
    }
    const QJsonObject& blockGraphConfig() const { return m_blockGraphConfig; }
    int tickCount() const { return m_ticks.size(); }

signals:
    void tick(const IBComm::MarketTick& tick);
    void barClose(const QString& symbol, const QDateTime& timestamp);

private:
    struct BarCloseEvent {
        QString symbol;
        QDateTime timestamp;
    };

    QVector<IBComm::MarketTick> m_ticks;
    QVector<Pipeline::ExecutionIntent> m_expectedIntents;
    QVector<BarCloseEvent> m_barCloses;
    QJsonObject m_blockGraphConfig;
};

#endif // REPLAY_MARKETDATAREPLAYER_H
