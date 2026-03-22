#include "StrategyRuntimePolicy.h"

namespace Pipeline {

bool StrategyRuntimePolicy::shouldEvaluateNow(const RuntimeState& state, const QDateTime& now) const
{
    switch (evaluationMode) {
    case EvaluationMode::EveryTick:
    case EvaluationMode::EveryBarClose:
        return true;
    case EvaluationMode::EveryNBars:
        return state.barsSinceEvaluation >= evaluationIntervalN;
    case EvaluationMode::EveryNMinutes:
        if (!state.lastEvaluationTime.isValid()) return true;
        return state.lastEvaluationTime.secsTo(now) >= (evaluationIntervalN * 60);
    case EvaluationMode::EveryNDays:
        if (!state.lastEvaluationTime.isValid()) return true;
        return state.lastEvaluationTime.secsTo(now) >= (evaluationIntervalN * 86400);
    }
    return true;
}

bool StrategyRuntimePolicy::shouldRebalanceNow(const RuntimeState& state, const QDateTime& now) const
{
    switch (rebalanceMode) {
    case RebalanceMode::Immediate:
        return true;
    case RebalanceMode::EveryNBars:
        return state.barsSinceRebalance >= rebalanceIntervalN;
    case RebalanceMode::EveryNMinutes:
        if (!state.lastRebalanceTime.isValid()) return true;
        return state.lastRebalanceTime.secsTo(now) >= (rebalanceIntervalN * 60);
    case RebalanceMode::EveryNDays:
        if (!state.lastRebalanceTime.isValid()) return true;
        return state.lastRebalanceTime.secsTo(now) >= (rebalanceIntervalN * 86400);
    }
    return true;
}

StrategyRuntimePolicy StrategyRuntimePolicy::fromJson(const QJsonObject& obj)
{
    StrategyRuntimePolicy p;
    if (obj.isEmpty()) return p;

    const QString evalMode = obj.value(QStringLiteral("evaluationMode")).toString(QStringLiteral("EveryBarClose"));
    if (evalMode == QStringLiteral("EveryTick"))          p.evaluationMode = EvaluationMode::EveryTick;
    else if (evalMode == QStringLiteral("EveryNBars"))    p.evaluationMode = EvaluationMode::EveryNBars;
    else if (evalMode == QStringLiteral("EveryNMinutes")) p.evaluationMode = EvaluationMode::EveryNMinutes;
    else if (evalMode == QStringLiteral("EveryNDays"))    p.evaluationMode = EvaluationMode::EveryNDays;
    else                                  p.evaluationMode = EvaluationMode::EveryBarClose;

    p.evaluationIntervalN = obj.value(QStringLiteral("evaluationIntervalN")).toInt(1);

    const QString rebMode = obj.value(QStringLiteral("rebalanceMode")).toString(QStringLiteral("Immediate"));
    if (rebMode == QStringLiteral("EveryNBars"))         p.rebalanceMode = RebalanceMode::EveryNBars;
    else if (rebMode == QStringLiteral("EveryNMinutes")) p.rebalanceMode = RebalanceMode::EveryNMinutes;
    else if (rebMode == QStringLiteral("EveryNDays"))    p.rebalanceMode = RebalanceMode::EveryNDays;
    else                                 p.rebalanceMode = RebalanceMode::Immediate;

    p.rebalanceIntervalN = obj.value(QStringLiteral("rebalanceIntervalN")).toInt(1);

    p.accumulateAlphaSignals = obj.value(QStringLiteral("accumulateAlphaSignals")).toBool(true);
    p.signalExpiryBars       = obj.value(QStringLiteral("signalExpiryBars")).toInt(0);

    p.riskAlwaysActive           = obj.value(QStringLiteral("riskAlwaysActive")).toBool(true);
    p.riskCanCancelPendingOrders = obj.value(QStringLiteral("riskCanCancelPendingOrders")).toBool(true);

    p.executionImmediateAfterApproval = obj.value(QStringLiteral("executionImmediateAfterApproval")).toBool(true);

    return p;
}

QString StrategyRuntimePolicy::evalModeLabel(EvaluationMode mode)
{
    switch (mode) {
    case EvaluationMode::EveryTick:     return QStringLiteral("Every Tick");
    case EvaluationMode::EveryBarClose: return QStringLiteral("Every Bar Close");
    case EvaluationMode::EveryNBars:    return QStringLiteral("Every N Bars");
    case EvaluationMode::EveryNMinutes: return QStringLiteral("Every N Minutes");
    case EvaluationMode::EveryNDays:    return QStringLiteral("Every N Days");
    }
    return QStringLiteral("Unknown");
}

QString StrategyRuntimePolicy::rebalModeLabel(RebalanceMode mode)
{
    switch (mode) {
    case RebalanceMode::Immediate:      return QStringLiteral("Immediate");
    case RebalanceMode::EveryNBars:     return QStringLiteral("Every N Bars");
    case RebalanceMode::EveryNMinutes:  return QStringLiteral("Every N Minutes");
    case RebalanceMode::EveryNDays:     return QStringLiteral("Every N Days");
    }
    return QStringLiteral("Unknown");
}

QString StrategyRuntimePolicy::summary() const
{
    QString eval;
    switch (evaluationMode) {
    case EvaluationMode::EveryTick:
        eval = QStringLiteral("Eval: Every Tick"); break;
    case EvaluationMode::EveryBarClose:
        eval = QStringLiteral("Eval: Bar Close"); break;
    case EvaluationMode::EveryNBars:
        eval = QStringLiteral("Eval: Every %1 Bars").arg(evaluationIntervalN); break;
    case EvaluationMode::EveryNMinutes:
        eval = QStringLiteral("Eval: Every %1 Min").arg(evaluationIntervalN); break;
    case EvaluationMode::EveryNDays:
        eval = QStringLiteral("Eval: Every %1 Days").arg(evaluationIntervalN); break;
    }

    QString rebal;
    switch (rebalanceMode) {
    case RebalanceMode::Immediate:
        rebal = QStringLiteral("Rebal: Immediate"); break;
    case RebalanceMode::EveryNBars:
        rebal = QStringLiteral("Rebal: Every %1 Bars").arg(rebalanceIntervalN); break;
    case RebalanceMode::EveryNMinutes:
        rebal = QStringLiteral("Rebal: Every %1 Min").arg(rebalanceIntervalN); break;
    case RebalanceMode::EveryNDays:
        rebal = QStringLiteral("Rebal: Every %1 Days").arg(rebalanceIntervalN); break;
    }

    return eval + QStringLiteral("  |  ") + rebal;
}

bool StrategyRuntimePolicy::isDefault() const
{
    return evaluationMode == EvaluationMode::EveryBarClose
        && rebalanceMode == RebalanceMode::Immediate;
}

QJsonObject StrategyRuntimePolicy::toJson() const
{
    QJsonObject obj;

    switch (evaluationMode) {
    case EvaluationMode::EveryTick:     obj[QStringLiteral("evaluationMode")] = QStringLiteral("EveryTick");     break;
    case EvaluationMode::EveryBarClose: obj[QStringLiteral("evaluationMode")] = QStringLiteral("EveryBarClose"); break;
    case EvaluationMode::EveryNBars:    obj[QStringLiteral("evaluationMode")] = QStringLiteral("EveryNBars");    break;
    case EvaluationMode::EveryNMinutes: obj[QStringLiteral("evaluationMode")] = QStringLiteral("EveryNMinutes"); break;
    case EvaluationMode::EveryNDays:    obj[QStringLiteral("evaluationMode")] = QStringLiteral("EveryNDays");    break;
    }
    obj[QStringLiteral("evaluationIntervalN")] = evaluationIntervalN;

    switch (rebalanceMode) {
    case RebalanceMode::Immediate:      obj[QStringLiteral("rebalanceMode")] = QStringLiteral("Immediate");      break;
    case RebalanceMode::EveryNBars:     obj[QStringLiteral("rebalanceMode")] = QStringLiteral("EveryNBars");     break;
    case RebalanceMode::EveryNMinutes:  obj[QStringLiteral("rebalanceMode")] = QStringLiteral("EveryNMinutes");  break;
    case RebalanceMode::EveryNDays:     obj[QStringLiteral("rebalanceMode")] = QStringLiteral("EveryNDays");     break;
    }
    obj[QStringLiteral("rebalanceIntervalN")] = rebalanceIntervalN;

    obj[QStringLiteral("accumulateAlphaSignals")] = accumulateAlphaSignals;
    obj[QStringLiteral("signalExpiryBars")]       = signalExpiryBars;

    obj[QStringLiteral("riskAlwaysActive")]           = riskAlwaysActive;
    obj[QStringLiteral("riskCanCancelPendingOrders")] = riskCanCancelPendingOrders;

    obj[QStringLiteral("executionImmediateAfterApproval")] = executionImmediateAfterApproval;

    return obj;
}

} // namespace Pipeline
