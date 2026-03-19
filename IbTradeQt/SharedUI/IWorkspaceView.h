#ifndef IWORKSPACEVIEW_H
#define IWORKSPACEVIEW_H

#include "ViewModels.h"
#include "MetricsStrip.h"

class IWorkspaceView
{
public:
    virtual ~IWorkspaceView() = default;

    virtual void applyHeader(const VM::WorkspaceHeader& header) = 0;
    virtual void applyMetrics(const QList<MetricCard>& cards) = 0;
    virtual void clearView() = 0;
};

#endif // IWORKSPACEVIEW_H
