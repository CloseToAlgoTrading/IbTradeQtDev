#include "IRiskBlock.h"

#include "SemanticModelDataMapper.h"

namespace Pipeline {

ModelDataList IRiskBlock::processSemantic(
    const ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    QVector<TargetPosition> targets =
        SemanticMapping::targetPositionsFromModelData(in, currentPositions, correlationId);

    QVector<TargetPosition> approved;
    for (const auto& target : targets) {
        const RiskDecision decision = evaluate(target, targets, currentPositions);

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

    return SemanticMapping::modelDataFromTargetPositions(approved);
}

} // namespace Pipeline
