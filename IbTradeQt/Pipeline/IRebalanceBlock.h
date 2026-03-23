#ifndef PIPELINE_IREBALANCEBLOCK_H
#define PIPELINE_IREBALANCEBLOCK_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include "Contracts.h"
#include "SemanticTypes.h"

namespace Pipeline {
struct PipelineRuntimeContext;

class IRebalanceBlock : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~IRebalanceBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual void setRuntimeContext(const PipelineRuntimeContext* ctx) { m_runtimeContext = ctx; }
    const PipelineRuntimeContext* runtimeContext() const { return m_runtimeContext; }

    virtual QVector<TargetPosition> rebalance(
        const QVector<Signal>& inputSignals,
        const QMap<QString, double>& currentPositions
    ) = 0;

    /// Alpha / selection model → **target positions** (absolute share targets per symbol).
    /// Risk and execution consume `TargetPosition` in the native semantic path (`StrategyPipelineRunner`).
    /// Default: `signalsFromModelData` → `rebalance(signals)`.
    virtual QVector<TargetPosition> processSemanticTargets(
        const ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId);

    /// Legacy wire format: `processSemanticTargets` → `modelDataFromTargetPositions` (delta-encoded rows).
    virtual ModelDataList processSemantic(
        const ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId);

signals:
    void rebalanceComplete(const QVector<TargetPosition>& targets);
    void errorOccurred(const QString& message);

protected:
    const PipelineRuntimeContext* m_runtimeContext = nullptr;
};

} // namespace Pipeline

#endif // PIPELINE_IREBALANCEBLOCK_H
