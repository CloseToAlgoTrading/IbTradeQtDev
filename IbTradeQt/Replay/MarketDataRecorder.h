#ifndef REPLAY_MARKETDATARECORDER_H
#define REPLAY_MARKETDATARECORDER_H

#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include "Pipeline/Contracts.h"

class MarketDataRecorder : public QObject {
    Q_OBJECT

public:
    explicit MarketDataRecorder(const QString& filename, QObject* parent = nullptr)
        : QObject(parent)
        , m_file(filename)
    {
        m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    }

    ~MarketDataRecorder() override {
        if (m_file.isOpen()) {
            m_file.close();
        }
    }

    bool isOpen() const { return m_file.isOpen(); }
    int tickCount() const { return m_tickCount; }
    int intentCount() const { return m_intentCount; }

    void writeBlockGraphConfig(const QJsonObject& config) {
        QJsonObject obj;
        obj["type"] = "BlockGraphConfig";
        obj["config"] = config;
        writeLine(obj);
    }

public slots:
    void onMarketTick(const Pipeline::MarketTick& tick) {
        QJsonObject obj;
        obj["type"] = "MarketTick";
        obj["symbol"] = tick.symbol;
        obj["bid"] = tick.bid;
        obj["ask"] = tick.ask;
        obj["volume"] = tick.volume;
        obj["timestamp"] = tick.timestamp.toString(Qt::ISODateWithMs);
        obj["reqId"] = tick.reqId;
        writeLine(obj);
        ++m_tickCount;
    }

    void onExecutionIntent(const Pipeline::ExecutionIntent& intent) {
        QJsonObject obj;
        obj["type"] = "ExecutionIntent";
        obj["symbol"] = intent.symbol;
        obj["quantity"] = intent.quantity;
        obj["orderType"] = static_cast<int>(intent.orderType);
        obj["correlationId"] = intent.correlationId;
        obj["riskApproval"] = intent.riskApproval;
        obj["timestamp"] = intent.timestamp.toString(Qt::ISODateWithMs);
        if (intent.limitPrice.has_value()) {
            obj["limitPrice"] = *intent.limitPrice;
        }
        writeLine(obj);
        ++m_intentCount;
    }

    void onOhlcvBar(const Pipeline::OHLCVBar& bar) {
        QJsonObject obj;
        obj["type"] = "BarClose";
        obj["symbol"] = bar.symbol;
        obj["open"] = bar.open;
        obj["high"] = bar.high;
        obj["low"] = bar.low;
        obj["close"] = bar.close;
        obj["volume"] = bar.volume;
        obj["timestamp"] = bar.timestamp.toString(Qt::ISODateWithMs);
        writeLine(obj);
    }

private:
    void writeLine(const QJsonObject& obj) {
        if (!m_file.isOpen()) return;
        m_file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        m_file.write("\n");
        m_file.flush();
    }

    QFile m_file;
    int m_tickCount = 0;
    int m_intentCount = 0;
};

#endif // REPLAY_MARKETDATARECORDER_H
