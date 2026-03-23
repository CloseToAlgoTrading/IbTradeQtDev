#include "SemanticModelDataMapper.h"

#include <QDateTime>
#include <QMap>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace Pipeline {
namespace SemanticMapping {

static QMap<QString, double> positionMapUpperKeys(const QMap<QString, double>& in)
{
    QMap<QString, double> out;
    for (auto it = in.constBegin(); it != in.constEnd(); ++it)
        out[it.key().trimmed().toUpper()] = it.value();
    return out;
}

/// Long-only: sell intent cannot exceed long size (avoids shorts when sell amount > position).
static double clampSellDeltaForLongOnly(double currentPosition, double delta)
{
    if (delta >= 0.0)
        return delta;
    const double longQty = std::max(0.0, currentPosition);
    return std::max(delta, -longQty);
}

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
    const QMap<QString, double> posUpper = positionMapUpperKeys(currentPositions);
    for (const auto& row : *data) {
        TargetPosition tp;
        tp.symbol = row.symbol;
        tp.currentQuantity = posUpper.value(row.symbol.trimmed().toUpper(), 0.0);
        // Rows from modelDataFromTargetPositions use amount = |delta|; UP/DOWN here mean delta sign (trade
        // direction), not necessarily the alpha row’s original direction.
        // Do not treat DOWN as absolute negative target (-amount); that double-counts vs current.
        if (row.direction == DIRECTION_UP)
            tp.targetQuantity = tp.currentQuantity + (row.amount > 0.0 ? row.amount : 0.0);
        else if (row.direction == DIRECTION_DOWN) {
            const double longQty = std::max(0.0, tp.currentQuantity);
            tp.targetQuantity = std::max(0.0, longQty - (row.amount > 0.0 ? row.amount : 0.0));
        }
        else
            tp.targetQuantity = tp.currentQuantity;
        tp.correlationId = correlationId;
        out.append(tp);
    }
    return out;
}

ModelDataList modelDataFromTargetPositions(const QVector<TargetPosition>& targets)
{
    // UP/DOWN here = sign of (target − current), i.e. buy vs sell delta. This is not the same semantic as
    // alpha `DIRECTION_UP` / `DIRECTION_DOWN` on the incoming model — rebalance only supplied numbers in
    // TargetPosition; we re-label deltas for the wire format to IRiskBlock/IExecutionBlock.
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
    const QMap<QString, double> posUpper = positionMapUpperKeys(currentPositions);
    QVector<ExecutionIntent> intents;
    for (const auto& tp : tps) {
        const QString symKey = tp.symbol.trimmed().toUpper();
        const double current = posUpper.value(symKey, 0.0);
        double delta         = tp.targetQuantity - current;
        delta = clampSellDeltaForLongOnly(current, delta);
        if (qFuzzyIsNull(delta))
            continue;
        ExecutionIntent intent;
        intent.symbol = symKey;
        intent.quantity = delta;
        intent.orderType = ExecutionIntent::Market;
        intent.riskApproval = QStringLiteral("Approved");
        intent.correlationId = tp.correlationId.isEmpty() ? correlationId : tp.correlationId;
        intent.timestamp = eventTime;
        intents.append(intent);
    }
    return intents;
}

QVector<TargetPosition> mergeTargetPositionsBySymbolLastWins(const QVector<TargetPosition>& targets)
{
    QMap<QString, TargetPosition> bySym;
    for (const auto& tp : targets) {
        const QString k = tp.symbol.trimmed().toUpper();
        TargetPosition t = tp;
        t.symbol = k;
        if (t.targetQuantity < 0.0)
            t.targetQuantity = 0.0;
        bySym[k] = t;
    }
    QVector<TargetPosition> out;
    out.reserve(bySym.size());
    for (auto it = bySym.constBegin(); it != bySym.constEnd(); ++it)
        out.append(it.value());
    return out;
}

QVector<ExecutionIntent> executionIntentsFromTargetPositions(
    const QVector<TargetPosition>& targets,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime)
{
    const QMap<QString, double> posUpper = positionMapUpperKeys(currentPositions);
    QVector<ExecutionIntent> intents;
    intents.reserve(targets.size());
    for (const auto& tp : targets) {
        const QString symKey = tp.symbol.trimmed().toUpper();
        const double current = posUpper.value(symKey, 0.0);
        double delta         = tp.targetQuantity - current;
        delta = clampSellDeltaForLongOnly(current, delta);
        if (qFuzzyIsNull(delta))
            continue;
        ExecutionIntent intent;
        intent.symbol        = symKey;
        intent.quantity      = delta;
        intent.orderType     = ExecutionIntent::Market;
        intent.riskApproval  = QStringLiteral("Approved");
        intent.correlationId = tp.correlationId.isEmpty() ? correlationId : tp.correlationId;
        intent.timestamp     = eventTime;
        intents.append(intent);
    }
    std::sort(intents.begin(), intents.end(),
              [](const ExecutionIntent& a, const ExecutionIntent& b) {
                  const bool aSell = a.quantity < 0.0;
                  const bool bSell = b.quantity < 0.0;
                  if (aSell != bSell)
                      return aSell;
                  return a.symbol < b.symbol;
              });
    return intents;
}

} // namespace SemanticMapping
} // namespace Pipeline
