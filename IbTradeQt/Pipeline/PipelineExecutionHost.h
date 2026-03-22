#ifndef PIPELINE_PIPELINEEXECUTIONHOST_H
#define PIPELINE_PIPELINEEXECUTIONHOST_H

#include <QObject>
#include "StrategyPipelineRunner.h"

namespace Pipeline {

/// Cross-thread drain helper: flushes Qt-queued delivery to the runner before teardown.
/// Live StrategyRuntime::stop() invokes runner ping with BlockingQueuedConnection for the same effect.
class PipelineExecutionHost : public QObject {
    Q_OBJECT
public:
    explicit PipelineExecutionHost(StrategyPipelineRunner* runner, QObject* parent = nullptr);

    void drainRunnerQueue() const;

private:
    StrategyPipelineRunner* m_runner = nullptr;
};

} // namespace Pipeline

#endif
