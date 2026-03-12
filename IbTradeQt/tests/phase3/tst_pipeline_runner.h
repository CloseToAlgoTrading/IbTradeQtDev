#ifndef TST_PIPELINE_RUNNER_H
#define TST_PIPELINE_RUNNER_H

#include <QtTest>

// Phase 3: StrategyPipelineRunner orchestration tests
// Implement when the full runner is wired with block registry support.
//
// Planned tests:
//  - Single alpha → rebalance → execution (happy path)
//  - Multiple alphas → merge → rebalance → execution
//  - Risk rejection prevents execution for specific symbol
//  - Risk modification adjusts quantity
//  - Multi-scope risk: Strategy → Portfolio → Account ordering
//  - Execution receives only approved intents
//  - CorrelationId flows end-to-end through pipeline
//  - Different merge policies produce different outcomes
//  - Runner handles empty signal list gracefully
//  - Runner with legacy adapter blocks (mixed old + new)

class TestPipelineRunner : public QObject
{
    Q_OBJECT

private slots:
    void placeholder()
    {
        QSKIP("Phase 3 not yet implemented");
    }
};

#endif // TST_PIPELINE_RUNNER_H
