#ifndef TST_SUPERVISION_H
#define TST_SUPERVISION_H

#include <QtTest>

// Phase 4: Supervision-lite tests
// Implement when StrategyRuntime, Supervisor, and restart policies are created.
//
// Planned tests:
//  - StrategyRuntime starts on its own QThread
//  - StrategyRuntime receives MarketTick via queued connection
//  - Supervisor detects unhealthy runtime (health flag timeout)
//  - Supervisor restarts failed runtime using factory
//  - Restart policy: max retries respected
//  - Restart policy: backoff delay applied
//  - Crash in one runtime doesn't affect others
//  - Supervisor start/stop lifecycle
//  - Runtime factory recreates runtime from config
//  - Health monitor reports per-strategy status

class TestSupervision : public QObject
{
    Q_OBJECT

private slots:
    void placeholder()
    {
        QSKIP("Phase 4 not yet implemented");
    }
};

#endif // TST_SUPERVISION_H
