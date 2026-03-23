#include "IRiskBlock.h"

#include "PipelineRuntimeContext.h"
#include "SemanticModelDataMapper.h"

namespace Pipeline {

QVector<TargetPosition> IRiskBlock::processSemanticTargets(
    const QVector<TargetPosition>& targetsIn,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    Q_UNUSED(correlationId);
    const QMap<QString, double>& pos = holdingsForBlocks(m_runtimeContext, currentPositions);
    QVector<TargetPosition> approved;
    approved.reserve(targetsIn.size());
    for (const auto& targetIn : targetsIn) {
        TargetPosition target = targetIn;
        target.currentQuantity = pos.value(target.symbol, 0.0);
        const RiskDecision decision = evaluate(target, targetsIn, pos);

        switch (decision.action) {
            case RiskDecision::Action::Approve:
                approved.append(target);
                break;
            case RiskDecision::Action::Reject:
                emit riskViolation(target.symbol, decision.reason);
                break;
            case RiskDecision::Action::Modify: {
                auto modified = target;
                if (decision.modifiedQuantity) {
                    modified.targetQuantity = modified.currentQuantity + *decision.modifiedQuantity;
                    modified.reason += QStringLiteral(" [RiskModified: %1]").arg(decision.reason);
                }
                approved.append(modified);
                break;
            }
        }
    }
    return approved;
}

ModelDataList IRiskBlock::processSemantic(
    const ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    const QMap<QString, double>& pos = holdingsForBlocks(m_runtimeContext, currentPositions);
    QVector<TargetPosition> targets =
        SemanticMapping::targetPositionsFromModelData(in, pos, correlationId);
    targets = processSemanticTargets(targets, currentPositions, correlationId);
    return SemanticMapping::modelDataFromTargetPositions(targets);
}

} // namespace Pipeline
