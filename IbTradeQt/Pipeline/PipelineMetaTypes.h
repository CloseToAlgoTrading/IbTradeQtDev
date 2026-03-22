#ifndef PIPELINE_METATYPES_H
#define PIPELINE_METATYPES_H

#include "Contracts.h"
#include <QMetaType>

namespace Pipeline {

/// Call once from application / test main before queued connections to pipeline types.
inline void registerPipelineMetaTypes()
{
    static bool done = false;
    if (done)
        return;
    done = true;
    qRegisterMetaType<Pipeline::MarketTick>("Pipeline::MarketTick");
    qRegisterMetaType<Pipeline::OHLCVBar>("Pipeline::OHLCVBar");
    qRegisterMetaType<Pipeline::TickByTickTrade>("Pipeline::TickByTickTrade");
    qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
    qRegisterMetaType<Pipeline::TargetPosition>("Pipeline::TargetPosition");
    qRegisterMetaType<Pipeline::ExecutionIntent>("Pipeline::ExecutionIntent");
}

} // namespace Pipeline

#endif
