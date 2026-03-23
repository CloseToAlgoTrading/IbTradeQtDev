#include "IRebalanceBlock.h"

#include "SemanticModelDataMapper.h"

namespace Pipeline {

ModelDataList IRebalanceBlock::processSemantic(
    const ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    const QVector<Signal> sigs =
        SemanticMapping::signalsFromModelData(in, correlationId, id());
    QVector<TargetPosition> targets = rebalance(sigs, currentPositions);
    for (auto& t : targets)
        t.correlationId = correlationId;
    return SemanticMapping::modelDataFromTargetPositions(targets);
}

} // namespace Pipeline
