#ifndef PIPELINE_PIPELINEDEFINITION_H
#define PIPELINE_PIPELINEDEFINITION_H

#include "StrategyPipelineRunner.h"
#include "StrategyRuntimePolicy.h"

namespace Pipeline {

// Separates structural topology (BlockGraph) from runtime orchestration
// (StrategyRuntimePolicy). PipelineFactory::buildDefinition() returns this.
struct PipelineDefinition {
    BlockGraph graph;
    StrategyRuntimePolicy runtimePolicy;
};

} // namespace Pipeline

#endif // PIPELINE_PIPELINEDEFINITION_H
