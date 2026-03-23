#include "IExecutionBlock.h"

#include "SemanticModelDataMapper.h"

namespace Pipeline {

void IExecutionBlock::executeSemantic(
    const ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime)
{
    const QVector<ExecutionIntent> intents =
        SemanticMapping::executionIntentsFromModelData(in, currentPositions, correlationId, eventTime);
    execute(intents);
}

} // namespace Pipeline
