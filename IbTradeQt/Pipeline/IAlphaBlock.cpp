#include "IAlphaBlock.h"

namespace Pipeline {

ModelDataList IAlphaBlock::processSemantic(const ModelDataList& in, const QString& correlationId)
{
    Q_UNUSED(correlationId);
    return in;
}

} // namespace Pipeline
