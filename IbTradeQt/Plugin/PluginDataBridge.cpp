#include "PluginDataBridge.h"

#include "../Strategies/Generic/UnifiedModelData.h"

namespace Plugin {

namespace {

eDirection directionFromInt(int raw)
{
    switch (raw) {
    case 0: return DIRECTION_DOWN;
    case 1: return DIRECTION_UP;
    case 2: return DIRECTION_FLAT;
    default: return DIRECTION_UNDEFINED;
    }
}

int directionToInt(eDirection dir)
{
    return static_cast<int>(dir);
}

} // namespace

QJsonObject PluginDataBridge::toJson(const Pipeline::MarketTick& tick)
{
    QJsonObject obj;
    obj[QStringLiteral("symbol")] = tick.symbol;
    obj[QStringLiteral("bid")] = tick.bid;
    obj[QStringLiteral("ask")] = tick.ask;
    obj[QStringLiteral("volume")] = tick.volume;
    obj[QStringLiteral("timestamp")] = tick.timestamp.toString(Qt::ISODateWithMs);
    obj[QStringLiteral("reqId")] = tick.reqId;
    return obj;
}

QJsonObject PluginDataBridge::toJson(const Pipeline::OHLCVBar& bar)
{
    QJsonObject obj;
    obj[QStringLiteral("symbol")] = bar.symbol;
    obj[QStringLiteral("open")] = bar.open;
    obj[QStringLiteral("high")] = bar.high;
    obj[QStringLiteral("low")] = bar.low;
    obj[QStringLiteral("close")] = bar.close;
    obj[QStringLiteral("volume")] = bar.volume;
    obj[QStringLiteral("timestamp")] = bar.timestamp.toString(Qt::ISODateWithMs);
    return obj;
}

QJsonObject PluginDataBridge::toJson(const Pipeline::TickByTickTrade& trade)
{
    QJsonObject obj;
    obj[QStringLiteral("symbol")] = trade.symbol;
    obj[QStringLiteral("price")] = trade.price;
    obj[QStringLiteral("size")] = trade.size;
    obj[QStringLiteral("timestamp")] = trade.timestamp.toString(Qt::ISODateWithMs);
    obj[QStringLiteral("exchange")] = trade.exchange;
    return obj;
}

QJsonObject PluginDataBridge::toJson(const Pipeline::Signal& signal)
{
    return signal.toJson();
}

QJsonObject PluginDataBridge::toJson(const Pipeline::TargetPosition& target)
{
    return target.toJson();
}

QJsonObject PluginDataBridge::toJson(const Pipeline::ExecutionIntent& intent)
{
    return intent.toJson();
}

QJsonArray PluginDataBridge::toJson(const QVector<Pipeline::Signal>& signalList)
{
    QJsonArray arr;
    for (const auto& signal : signalList) {
        arr.push_back(toJson(signal));
    }
    return arr;
}

QJsonArray PluginDataBridge::toJson(const QVector<Pipeline::TargetPosition>& targets)
{
    QJsonArray arr;
    for (const auto& target : targets) {
        arr.push_back(toJson(target));
    }
    return arr;
}

QJsonArray PluginDataBridge::toJson(const QVector<Pipeline::ExecutionIntent>& intents)
{
    QJsonArray arr;
    for (const auto& intent : intents) {
        arr.push_back(toJson(intent));
    }
    return arr;
}

QJsonObject PluginDataBridge::toJson(const QMap<QString, double>& holdings)
{
    QJsonObject obj;
    for (auto it = holdings.begin(); it != holdings.end(); ++it) {
        obj[it.key()] = it.value();
    }
    return obj;
}

QJsonArray PluginDataBridge::toJson(const Pipeline::ModelDataList& data)
{
    QJsonArray arr;
    if (!data) {
        return arr;
    }
    for (const UnifiedModelData& row : *data) {
        QJsonObject obj;
        obj[QStringLiteral("symbol")] = row.symbol;
        obj[QStringLiteral("direction")] = directionToInt(row.direction);
        obj[QStringLiteral("probability")] = row.probability;
        obj[QStringLiteral("amount")] = row.amount;
        obj[QStringLiteral("currentPrice")] = row.currentPrice;
        arr.push_back(obj);
    }
    return arr;
}

Pipeline::MarketTick PluginDataBridge::marketTickFromJson(const QJsonObject& obj)
{
    Pipeline::MarketTick tick;
    tick.symbol = obj.value(QStringLiteral("symbol")).toString();
    tick.bid = obj.value(QStringLiteral("bid")).toDouble();
    tick.ask = obj.value(QStringLiteral("ask")).toDouble();
    tick.volume = obj.value(QStringLiteral("volume")).toDouble();
    tick.timestamp = QDateTime::fromString(
        obj.value(QStringLiteral("timestamp")).toString(),
        Qt::ISODateWithMs);
    tick.reqId = obj.value(QStringLiteral("reqId")).toInt();
    return tick;
}

Pipeline::OHLCVBar PluginDataBridge::ohlcvBarFromJson(const QJsonObject& obj)
{
    Pipeline::OHLCVBar bar;
    bar.symbol = obj.value(QStringLiteral("symbol")).toString();
    bar.open = obj.value(QStringLiteral("open")).toDouble();
    bar.high = obj.value(QStringLiteral("high")).toDouble();
    bar.low = obj.value(QStringLiteral("low")).toDouble();
    bar.close = obj.value(QStringLiteral("close")).toDouble();
    bar.volume = obj.value(QStringLiteral("volume")).toDouble();
    bar.timestamp = QDateTime::fromString(
        obj.value(QStringLiteral("timestamp")).toString(),
        Qt::ISODateWithMs);
    return bar;
}

Pipeline::TickByTickTrade PluginDataBridge::tickByTickFromJson(const QJsonObject& obj)
{
    Pipeline::TickByTickTrade trade;
    trade.symbol = obj.value(QStringLiteral("symbol")).toString();
    trade.price = obj.value(QStringLiteral("price")).toDouble();
    trade.size = obj.value(QStringLiteral("size")).toDouble();
    trade.timestamp = QDateTime::fromString(
        obj.value(QStringLiteral("timestamp")).toString(),
        Qt::ISODateWithMs);
    trade.exchange = obj.value(QStringLiteral("exchange")).toString();
    return trade;
}

Pipeline::Signal PluginDataBridge::signalFromJson(const QJsonObject& obj)
{
    return Pipeline::Signal::fromJson(obj);
}

Pipeline::TargetPosition PluginDataBridge::targetFromJson(const QJsonObject& obj)
{
    return Pipeline::TargetPosition::fromJson(obj);
}

Pipeline::ExecutionIntent PluginDataBridge::intentFromJson(const QJsonObject& obj)
{
    return Pipeline::ExecutionIntent::fromJson(obj);
}

QVector<QString> PluginDataBridge::stringVectorFromJson(const QJsonArray& arr)
{
    QVector<QString> out;
    out.reserve(arr.size());
    for (const QJsonValue& value : arr) {
        out.push_back(value.toString());
    }
    return out;
}

QVector<Pipeline::Signal> PluginDataBridge::signalsFromJson(const QJsonArray& arr)
{
    QVector<Pipeline::Signal> out;
    out.reserve(arr.size());
    for (const QJsonValue& value : arr) {
        out.push_back(signalFromJson(value.toObject()));
    }
    return out;
}

QVector<Pipeline::TargetPosition> PluginDataBridge::targetsFromJson(const QJsonArray& arr)
{
    QVector<Pipeline::TargetPosition> out;
    out.reserve(arr.size());
    for (const QJsonValue& value : arr) {
        out.push_back(targetFromJson(value.toObject()));
    }
    return out;
}

QVector<Pipeline::ExecutionIntent> PluginDataBridge::intentsFromJson(const QJsonArray& arr)
{
    QVector<Pipeline::ExecutionIntent> out;
    out.reserve(arr.size());
    for (const QJsonValue& value : arr) {
        out.push_back(intentFromJson(value.toObject()));
    }
    return out;
}

QMap<QString, double> PluginDataBridge::holdingsFromJson(const QJsonObject& obj)
{
    QMap<QString, double> holdings;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        holdings[it.key()] = it.value().toDouble();
    }
    return holdings;
}

Pipeline::ModelDataList PluginDataBridge::modelDataFromJson(const QJsonArray& arr)
{
    Pipeline::ModelDataList data = createDataList();
    for (const QJsonValue& value : arr) {
        const QJsonObject obj = value.toObject();
        data->append(UnifiedModelData(
            obj.value(QStringLiteral("symbol")).toString(),
            directionFromInt(obj.value(QStringLiteral("direction")).toInt(DIRECTION_UNDEFINED)),
            obj.value(QStringLiteral("probability")).toDouble(),
            obj.value(QStringLiteral("amount")).toDouble(),
            obj.value(QStringLiteral("currentPrice")).toDouble()));
    }
    return data;
}

} // namespace Plugin
