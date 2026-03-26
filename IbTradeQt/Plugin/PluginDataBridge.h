#ifndef PLUGIN_PLUGINDATABRIDGE_H
#define PLUGIN_PLUGINDATABRIDGE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

#include "../Pipeline/Contracts.h"
#include "../Pipeline/SemanticTypes.h"

namespace Plugin {

class PluginDataBridge {
public:
    static QJsonObject toJson(const Pipeline::MarketTick& tick);
    static QJsonObject toJson(const Pipeline::OHLCVBar& bar);
    static QJsonObject toJson(const Pipeline::TickByTickTrade& trade);
    static QJsonObject toJson(const Pipeline::Signal& signal);
    static QJsonObject toJson(const Pipeline::TargetPosition& target);
    static QJsonObject toJson(const Pipeline::ExecutionIntent& intent);
    static QJsonArray toJson(const QVector<Pipeline::Signal>& signalList);
    static QJsonArray toJson(const QVector<Pipeline::TargetPosition>& targets);
    static QJsonArray toJson(const QVector<Pipeline::ExecutionIntent>& intents);
    static QJsonObject toJson(const QMap<QString, double>& holdings);
    static QJsonArray toJson(const Pipeline::ModelDataList& data);

    static Pipeline::MarketTick marketTickFromJson(const QJsonObject& obj);
    static Pipeline::OHLCVBar ohlcvBarFromJson(const QJsonObject& obj);
    static Pipeline::TickByTickTrade tickByTickFromJson(const QJsonObject& obj);
    static Pipeline::Signal signalFromJson(const QJsonObject& obj);
    static Pipeline::TargetPosition targetFromJson(const QJsonObject& obj);
    static Pipeline::ExecutionIntent intentFromJson(const QJsonObject& obj);
    static QVector<QString> stringVectorFromJson(const QJsonArray& arr);
    static QVector<Pipeline::Signal> signalsFromJson(const QJsonArray& arr);
    static QVector<Pipeline::TargetPosition> targetsFromJson(const QJsonArray& arr);
    static QVector<Pipeline::ExecutionIntent> intentsFromJson(const QJsonArray& arr);
    static QMap<QString, double> holdingsFromJson(const QJsonObject& obj);
    static Pipeline::ModelDataList modelDataFromJson(const QJsonArray& arr);
};

} // namespace Plugin

#endif // PLUGIN_PLUGINDATABRIDGE_H
