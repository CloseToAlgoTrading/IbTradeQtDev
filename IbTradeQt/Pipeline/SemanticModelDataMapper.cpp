#include "SemanticModelDataMapper.h"

#include <QDateTime>
#include <QtMath>
#include <cmath>

namespace Pipeline {
namespace SemanticMapping {

ModelDataList buildModelDataFromSymbols(const QVector<QString>& symbols)
{
    ModelDataList list = createDataList();
    for (const QString& sym : symbols) {
        const QString s = sym.trimmed().toUpper();
        if (!s.isEmpty())
            list->append(UnifiedModelData(s, DIRECTION_UNDEFINED, 0.0, 0.0, 0.0));
    }
    return list;
}

QStringList symbolsFromModelData(const ModelDataList& data)
{
    QStringList out;
    if (!data)
        return out;
    for (const auto& row : *data)
        out.append(row.symbol);
    return out;
}

static Signal::Direction directionFromLegacy(eDirection d)
{
    switch (d) {
        case DIRECTION_UP:   return Signal::Buy;
        case DIRECTION_DOWN: return Signal::Sell;
        default:             return Signal::Hold;
    }
}

QVector<Signal> signalsFromModelData(
    const ModelDataList& data,
    const QString& correlationId,
    const QString& alphaBlockId)
{
    QVector<Signal> out;
    if (!data)
        return out;
    for (const auto& row : *data) {
        if (row.direction == DIRECTION_UNDEFINED && row.amount == 0.0 && row.probability == 0.0)
            continue;
        Signal s;
        s.symbol = row.symbol;
        s.confidence = row.probability;
        s.direction = directionFromLegacy(row.direction);
        s.correlationId = correlationId;
        s.alphaBlockId = alphaBlockId;
        s.suggestedQuantity = row.amount;
        s.timestamp = QDateTime::currentDateTimeUtc();
        out.append(s);
    }
    return out;
}

static eDirection legacyDirectionFromSignal(Signal::Direction d)
{
    switch (d) {
        case Signal::Buy:  return DIRECTION_UP;
        case Signal::Sell: return DIRECTION_DOWN;
        default:           return DIRECTION_UNDEFINED;
    }
}

ModelDataList modelDataFromSignals(const QVector<Signal>& inputSignals)
{
    ModelDataList list = createDataList();
    for (const auto& s : inputSignals) {
        UnifiedModelData u(
            s.symbol,
            legacyDirectionFromSignal(s.direction),
            s.confidence,
            s.suggestedQuantity,
            0.0);
        list->append(u);
    }
    return list;
}

QVector<TargetPosition> targetPositionsFromModelData(
    const ModelDataList& data,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    QVector<TargetPosition> out;
    if (!data)
        return out;
    for (const auto& row : *data) {
        TargetPosition tp;
        tp.symbol = row.symbol;
        tp.currentQuantity = currentPositions.value(row.symbol, 0.0);
        if (row.direction == DIRECTION_UP)
            tp.targetQuantity = row.amount > 0.0 ? row.amount : 0.0;
        else if (row.direction == DIRECTION_DOWN)
            tp.targetQuantity = row.amount > 0.0 ? -row.amount : 0.0;
        else
            tp.targetQuantity = tp.currentQuantity;
        tp.correlationId = correlationId;
        out.append(tp);
    }
    return out;
}

ModelDataList modelDataFromTargetPositions(const QVector<TargetPosition>& targets)
{
    ModelDataList list = createDataList();
    for (const auto& tp : targets) {
        const double delta = tp.deltaQuantity();
        eDirection dir = DIRECTION_UNDEFINED;
        double amount = std::abs(tp.targetQuantity - tp.currentQuantity);
        if (delta > 0)
            dir = DIRECTION_UP;
        else if (delta < 0)
            dir = DIRECTION_DOWN;
        list->append(UnifiedModelData(tp.symbol, dir, 1.0, amount, 0.0));
    }
    return list;
}

QVector<ExecutionIntent> executionIntentsFromModelData(
    const ModelDataList& data,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime)
{
    const QVector<TargetPosition> tps =
        targetPositionsFromModelData(data, currentPositions, correlationId);
    QVector<ExecutionIntent> intents;
    for (const auto& tp : tps) {
        const double delta = tp.deltaQuantity();
        if (qFuzzyIsNull(delta))
            continue;
        ExecutionIntent intent;
        intent.symbol = tp.symbol;
        intent.quantity = delta;
        intent.orderType = ExecutionIntent::Market;
        intent.riskApproval = QStringLiteral("Approved");
        intent.correlationId = tp.correlationId.isEmpty() ? correlationId : tp.correlationId;
        intent.timestamp = eventTime;
        intents.append(intent);
    }
    return intents;
}

} // namespace SemanticMapping
} // namespace Pipeline
