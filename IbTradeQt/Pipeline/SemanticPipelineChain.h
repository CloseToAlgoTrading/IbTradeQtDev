#ifndef PIPELINE_SEMANTICPIPELINECHAIN_H
#define PIPELINE_SEMANTICPIPELINECHAIN_H

#include "SemanticTypes.h"
#include "Contracts.h"
#include <QVector>

namespace Pipeline {

/// Single place to merge alpha semantic `ModelDataList` with tick-accumulated `Signal`s
/// before converting to `Signal` for rebalance (Option C — one conversion to derived types).
ModelDataList mergeModelDataWithTickSignals(
    const ModelDataList& modelAfterAlpha,
    const QVector<Signal>& tickSignals,
    bool combineTick,
    const QString& correlationId,
    const QString& semanticAlphaBlockId);

} // namespace Pipeline

#endif // PIPELINE_SEMANTICPIPELINECHAIN_H
