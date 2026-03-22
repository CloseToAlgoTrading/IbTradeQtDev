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

    bool shouldEvaluateNow(const RuntimeState& state, const QDateTime& now) const;
    bool shouldRebalanceNow(const RuntimeState& state, const QDateTime& now) const;

    static StrategyRuntimePolicy fromJson(const QJsonObject& obj);

    static QString evalModeLabel(EvaluationMode mode);
    static QString rebalModeLabel(RebalanceMode mode);

    QString summary() const;
    bool isDefault() const;
    QJsonObject toJson() const;
};

} // namespace Pipeline

#endif // PIPELINE_STRATEGYRUNTIMEPOLICY_H
