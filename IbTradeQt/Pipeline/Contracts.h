#ifndef PIPELINE_CONTRACTS_H
#define PIPELINE_CONTRACTS_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>
#include <cmath>
#include <optional>

namespace Pipeline {

struct Signal {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double confidence MEMBER confidence)
    Q_PROPERTY(Direction direction MEMBER direction)
    Q_PROPERTY(QString correlationId MEMBER correlationId)

public:
    enum Direction { Buy, Sell, Hold };
    Q_ENUM(Direction)

    QString symbol;
    double confidence = 0.0;
    Direction direction = Hold;
    QString correlationId;
    QDateTime timestamp;
    QString alphaBlockId;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["symbol"] = symbol;
        obj["confidence"] = confidence;
        obj["direction"] = static_cast<int>(direction);
        obj["correlationId"] = correlationId;
        obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
        obj["alphaBlockId"] = alphaBlockId;
        return obj;
    }

    static Signal fromJson(const QJsonObject& obj) {
        Signal s;
        s.symbol = obj["symbol"].toString();
        s.confidence = obj["confidence"].toDouble();
        s.direction = static_cast<Direction>(obj["direction"].toInt());
        s.correlationId = obj["correlationId"].toString();
        s.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        s.alphaBlockId = obj["alphaBlockId"].toString();
        return s;
    }
};

struct TargetPosition {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double targetQuantity MEMBER targetQuantity)
    Q_PROPERTY(QString reason MEMBER reason)
    Q_PROPERTY(QString correlationId MEMBER correlationId)

public:
    QString symbol;
    double targetQuantity = 0.0;
    double currentQuantity = 0.0;
    QString reason;
    QString correlationId;
    QDateTime timestamp;

    double deltaQuantity() const { return targetQuantity - currentQuantity; }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["symbol"] = symbol;
        obj["targetQuantity"] = targetQuantity;
        obj["currentQuantity"] = currentQuantity;
        obj["reason"] = reason;
        obj["correlationId"] = correlationId;
        obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
        return obj;
    }

    static TargetPosition fromJson(const QJsonObject& obj) {
        TargetPosition tp;
        tp.symbol = obj["symbol"].toString();
        tp.targetQuantity = obj["targetQuantity"].toDouble();
        tp.currentQuantity = obj["currentQuantity"].toDouble();
        tp.reason = obj["reason"].toString();
        tp.correlationId = obj["correlationId"].toString();
        tp.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        return tp;
    }
};

struct ExecutionIntent {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double quantity MEMBER quantity)
    Q_PROPERTY(OrderType orderType MEMBER orderType)
    Q_PROPERTY(QString correlationId MEMBER correlationId)

public:
    enum OrderType { Market, Limit, Stop };
    Q_ENUM(OrderType)

    QString symbol;
    double quantity = 0.0; // Signed: positive = buy, negative = sell
    OrderType orderType = Market;
    std::optional<double> limitPrice;
    QString riskApproval;
    QString correlationId;
    QDateTime timestamp;

    QString side() const { return quantity >= 0 ? "Buy" : "Sell"; }
    double absQuantity() const { return std::abs(quantity); }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["symbol"] = symbol;
        obj["quantity"] = quantity;
        obj["orderType"] = static_cast<int>(orderType);
        if (limitPrice) {
            obj["limitPrice"] = *limitPrice;
        }
        obj["riskApproval"] = riskApproval;
        obj["correlationId"] = correlationId;
        obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
        return obj;
    }

    static ExecutionIntent fromJson(const QJsonObject& obj) {
        ExecutionIntent ei;
        ei.symbol = obj["symbol"].toString();
        ei.quantity = obj["quantity"].toDouble();
        ei.orderType = static_cast<OrderType>(obj["orderType"].toInt());
        if (obj.contains("limitPrice")) {
            ei.limitPrice = obj["limitPrice"].toDouble();
        }
        ei.riskApproval = obj["riskApproval"].toString();
        ei.correlationId = obj["correlationId"].toString();
        ei.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        return ei;
    }
};

} // namespace Pipeline

Q_DECLARE_METATYPE(Pipeline::Signal)
Q_DECLARE_METATYPE(Pipeline::TargetPosition)
Q_DECLARE_METATYPE(Pipeline::ExecutionIntent)

#endif // PIPELINE_CONTRACTS_H
