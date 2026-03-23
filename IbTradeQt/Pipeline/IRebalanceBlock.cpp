#include "IRebalanceBlock.h"

#include "PipelineRuntimeContext.h"
#include "SemanticModelDataMapper.h"

namespace Pipeline {

QVector<TargetPosition> IRebalanceBlock::processSemanticTargets(
    const ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    const QMap<QString, double>& pos = holdingsForBlocks(m_runtimeContext, currentPositions);
    const QVector<Signal> sigs =
        SemanticMapping::signalsFromModelData(in, correlationId, id());
    QVector<TargetPosition> targets = rebalance(sigs, pos);
    for (auto& t : targets)
        t.correlationId = correlationId;
    return targets;
}

ModelDataList IRebalanceBlock::processSemantic(
    const ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    const QVector<TargetPosition> targets =
        processSemanticTargets(in, currentPositions, correlationId);
    return SemanticMapping::modelDataFromTargetPositions(targets);
}

} // namespace Pipeline
