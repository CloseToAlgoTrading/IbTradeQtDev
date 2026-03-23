#ifndef PIPELINE_IRISKBLOCK_H
#define PIPELINE_IRISKBLOCK_H

#include <QObject>
#include <QVector>
#include <QJsonObject>
#include <optional>
#include "Contracts.h"
#include "Scope.h"
#include "SemanticTypes.h"

namespace Pipeline {

struct PipelineRuntimeContext;

struct RiskDecision {
    enum class Action { Approve, Reject, Modify };

    Action action;
    QString reason;
    std::optional<double> modifiedQuantity; // If Modify: new DELTA quantity (not absolute target)
    QString blockId;
};

class IRiskBlock : public QObject {
    Q_OBJECT
    Q_PROPERTY(Pipeline::Scope scope READ scope CONSTANT)

public:
    using QObject::QObject;
    virtual ~IRiskBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual Scope scope() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual void setRuntimeContext(const PipelineRuntimeContext* ctx) { m_runtimeContext = ctx; }
    const PipelineRuntimeContext* runtimeContext() const { return m_runtimeContext; }

    // Called on every market tick so risk blocks can monitor live prices
    // and emit proactive signals (e.g. stop-loss, trailing stop).
    // Default implementation is a no-op — override only when needed.
    virtual void onTick(const Pipeline::MarketTick& tick) { Q_UNUSED(tick); }

    // Evaluate a proposed target position and decide whether to approve,
    // reject, or modify it. Called once per target during pipeline execution.
    virtual RiskDecision evaluate(
        const TargetPosition& target,
        const QVector<TargetPosition>& allTargets,
        const QMap<QString, double>& currentPositions
    ) = 0;

    /// Native semantic path: approve/modify `TargetPosition` rows from rebalance (no `UnifiedModelData`
    /// delta round-trip).
    virtual QVector<TargetPosition> processSemanticTargets(
        const QVector<TargetPosition>& targetsIn,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId);

    /// Legacy: model rows → `targetPositionsFromModelData` → `processSemanticTargets` →
    /// `modelDataFromTargetPositions`.
    virtual ModelDataList processSemantic(
        const ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId);

signals:
    // Emitted when a risk block proactively generates an exit signal
    // (e.g. stop-loss triggered). The pipeline runner subscribes to this
    // and feeds it back into the pipeline as an alpha signal.
    void riskSignalGenerated(const Pipeline::Signal& signal);

    void riskViolation(const QString& symbol, const QString& reason);

protected:
    const PipelineRuntimeContext* m_runtimeContext = nullptr;
};

} // namespace Pipeline

#endif // PIPELINE_IRISKBLOCK_H
