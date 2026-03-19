#ifndef PIPELINE_STRATEGYRUNTIMEPOLICY_H
#define PIPELINE_STRATEGYRUNTIMEPOLICY_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include "Contracts.h"

namespace Pipeline {

struct PendingSignal {
    Signal signal;
    QDateTime createdAt;
    int createdBarIndex = 0;
};

struct RuntimeState {
    int barsSinceEvaluation = 0;
    int barsSinceRebalance = 0;
    QDateTime lastEvaluationTime;
    QDateTime lastRebalanceTime;
    int totalBarsSeen = 0;
    QVector<PendingSignal> pendingSignals;
};

struct StrategyRuntimePolicy {
    enum class EvaluationMode { EveryTick, EveryBarClose, EveryNBars, EveryNMinutes, EveryNDays };
    EvaluationMode evaluationMode = EvaluationMode::EveryBarClose;
    int evaluationIntervalN = 1;

    enum class RebalanceMode { Immediate, EveryNBars, EveryNMinutes, EveryNDays };
    RebalanceMode rebalanceMode = RebalanceMode::Immediate;
    int rebalanceIntervalN = 1;

    bool accumulateAlphaSignals = true;
    int  signalExpiryBars = 0; // 0 = never expire

    bool riskAlwaysActive = true;
    bool riskCanCancelPendingOrders = true;

    bool executionImmediateAfterApproval = true;

    // barsSince* counts eligible trigger events observed since last completed run.
    // Gate fires when barsSince* >= intervalN.
    // For EveryNMinutes: fires when (now - lastTime) >= intervalN minutes.
    bool shouldEvaluateNow(const RuntimeState& state, const QDateTime& now) const {
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

    bool shouldRebalanceNow(const RuntimeState& state, const QDateTime& now) const {
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

    static StrategyRuntimePolicy fromJson(const QJsonObject& obj) {
        StrategyRuntimePolicy p;
        if (obj.isEmpty()) return p;

        const QString evalMode = obj.value("evaluationMode").toString("EveryBarClose");
        if (evalMode == "EveryTick")          p.evaluationMode = EvaluationMode::EveryTick;
        else if (evalMode == "EveryNBars")    p.evaluationMode = EvaluationMode::EveryNBars;
        else if (evalMode == "EveryNMinutes") p.evaluationMode = EvaluationMode::EveryNMinutes;
        else if (evalMode == "EveryNDays")    p.evaluationMode = EvaluationMode::EveryNDays;
        else                                  p.evaluationMode = EvaluationMode::EveryBarClose;

        p.evaluationIntervalN = obj.value("evaluationIntervalN").toInt(1);

        const QString rebMode = obj.value("rebalanceMode").toString("Immediate");
        if (rebMode == "EveryNBars")         p.rebalanceMode = RebalanceMode::EveryNBars;
        else if (rebMode == "EveryNMinutes") p.rebalanceMode = RebalanceMode::EveryNMinutes;
        else if (rebMode == "EveryNDays")    p.rebalanceMode = RebalanceMode::EveryNDays;
        else                                 p.rebalanceMode = RebalanceMode::Immediate;

        p.rebalanceIntervalN = obj.value("rebalanceIntervalN").toInt(1);

        p.accumulateAlphaSignals = obj.value("accumulateAlphaSignals").toBool(true);
        p.signalExpiryBars       = obj.value("signalExpiryBars").toInt(0);

        p.riskAlwaysActive           = obj.value("riskAlwaysActive").toBool(true);
        p.riskCanCancelPendingOrders = obj.value("riskCanCancelPendingOrders").toBool(true);

        p.executionImmediateAfterApproval = obj.value("executionImmediateAfterApproval").toBool(true);

        return p;
    }

    // --- Display helpers (single source of truth for UI labels) ---

    static QString evalModeLabel(EvaluationMode mode) {
        switch (mode) {
        case EvaluationMode::EveryTick:     return QStringLiteral("Every Tick");
        case EvaluationMode::EveryBarClose: return QStringLiteral("Every Bar Close");
        case EvaluationMode::EveryNBars:    return QStringLiteral("Every N Bars");
        case EvaluationMode::EveryNMinutes: return QStringLiteral("Every N Minutes");
        case EvaluationMode::EveryNDays:    return QStringLiteral("Every N Days");
        }
        return QStringLiteral("Unknown");
    }

    static QString rebalModeLabel(RebalanceMode mode) {
        switch (mode) {
        case RebalanceMode::Immediate:      return QStringLiteral("Immediate");
        case RebalanceMode::EveryNBars:     return QStringLiteral("Every N Bars");
        case RebalanceMode::EveryNMinutes:  return QStringLiteral("Every N Minutes");
        case RebalanceMode::EveryNDays:     return QStringLiteral("Every N Days");
        }
        return QStringLiteral("Unknown");
    }

    QString summary() const {
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

    bool isDefault() const {
        return evaluationMode == EvaluationMode::EveryBarClose
            && rebalanceMode == RebalanceMode::Immediate;
    }

    // --- Serialization ---

    QJsonObject toJson() const {
        QJsonObject obj;

        switch (evaluationMode) {
        case EvaluationMode::EveryTick:     obj["evaluationMode"] = "EveryTick";     break;
        case EvaluationMode::EveryBarClose: obj["evaluationMode"] = "EveryBarClose"; break;
        case EvaluationMode::EveryNBars:    obj["evaluationMode"] = "EveryNBars";    break;
        case EvaluationMode::EveryNMinutes: obj["evaluationMode"] = "EveryNMinutes"; break;
        case EvaluationMode::EveryNDays:    obj["evaluationMode"] = "EveryNDays";    break;
        }
        obj["evaluationIntervalN"] = evaluationIntervalN;

        switch (rebalanceMode) {
        case RebalanceMode::Immediate:      obj["rebalanceMode"] = "Immediate";      break;
        case RebalanceMode::EveryNBars:     obj["rebalanceMode"] = "EveryNBars";     break;
        case RebalanceMode::EveryNMinutes:  obj["rebalanceMode"] = "EveryNMinutes";  break;
        case RebalanceMode::EveryNDays:     obj["rebalanceMode"] = "EveryNDays";     break;
        }
        obj["rebalanceIntervalN"] = rebalanceIntervalN;

        obj["accumulateAlphaSignals"] = accumulateAlphaSignals;
        obj["signalExpiryBars"]       = signalExpiryBars;

        obj["riskAlwaysActive"]           = riskAlwaysActive;
        obj["riskCanCancelPendingOrders"] = riskCanCancelPendingOrders;

        obj["executionImmediateAfterApproval"] = executionImmediateAfterApproval;

        return obj;
    }
};

} // namespace Pipeline

#endif // PIPELINE_STRATEGYRUNTIMEPOLICY_H
