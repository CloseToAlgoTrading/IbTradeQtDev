#include "PipelineExecutionHost.h"

#include <QMetaObject>

namespace Pipeline {

PipelineExecutionHost::PipelineExecutionHost(StrategyPipelineRunner* runner, QObject* parent)
    : QObject(parent)
    , m_runner(runner)
{}

void PipelineExecutionHost::drainRunnerQueue() const
{
    if (!m_runner)
        return;
    QMetaObject::invokeMethod(m_runner, "ping", Qt::BlockingQueuedConnection);
}

} // namespace Pipeline
