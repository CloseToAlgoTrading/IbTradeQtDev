#ifndef PIPELINE_PIPELINEFACTORY_H
#define PIPELINE_PIPELINEFACTORY_H

#include <QJsonObject>
#include "PipelineDefinition.h"
#include "PipelineConstants.h"

namespace Supervision {
class StrategyRuntime;
}

namespace Ports {
class IOrderExecutionPort;
class IPositionRepositoryPort;
}

namespace IBComm {
class MarketDataRouter;
}

namespace Pipeline {

class ISelectionBlock;
class IAlphaBlock;
class IRebalanceBlock;
class IRiskBlock;
class IExecutionBlock;
class ISignalMergePolicy;

class PipelineFactory {
public:
    static BlockGraph buildGraph(
        const QJsonObject& config,
        Ports::IOrderExecutionPort* executionPort = nullptr);

    static PipelineDefinition buildDefinition(
        const QJsonObject& config,
        Ports::IOrderExecutionPort* executionPort = nullptr);

    static Supervision::StrategyRuntime* createRuntime(
        const QString& name,
        const QJsonObject& config,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        IBComm::MarketDataRouter* router = nullptr);

private:
    static ISelectionBlock* createSelectionBlock(const QString& blockId);
    static IAlphaBlock* createAlphaBlock(const QString& blockId);
    static IRebalanceBlock* createRebalanceBlock(const QString& blockId);
    static IRiskBlock* createRiskBlock(const QString& blockId);
    static IExecutionBlock* createExecutionBlock(const QString& blockId);
    static ISignalMergePolicy* createMergePolicy(const QString& policyId);
};

} // namespace Pipeline

#endif // PIPELINE_PIPELINEFACTORY_H
