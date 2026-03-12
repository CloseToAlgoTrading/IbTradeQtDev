#ifndef TST_SUPERVISION_H
#define TST_SUPERVISION_H

#include <QtTest>
#include <QSignalSpy>
#include <QThread>
#include "Supervision/BoundedQueue.h"
#include "Supervision/StrategyRuntime.h"
#include "Supervision/Supervisor.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MarketOrderExecutionBlock.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Testing/MockMarketDataRouter.h"

namespace {

Pipeline::BlockGraph makeTestGraph() {
    Pipeline::BlockGraph graph;
    graph.alphaBlocks.append(new Blocks::MomentumAlphaBlock());
    graph.alphaBlocks.last()->setConfig({{"period", 3}, {"threshold", 0.01}});
    graph.strategyLevel.rebalance = new Blocks::SimpleRebalanceBlock();
    graph.executionBlock = new Blocks::MarketOrderExecutionBlock();
    return graph;
}

void deleteGraph(Pipeline::BlockGraph& graph) {
    for (auto* a : graph.alphaBlocks) delete a;
    if (graph.strategyLevel.rebalance) delete graph.strategyLevel.rebalance;
    if (graph.executionBlock) delete graph.executionBlock;
    for (auto* r : graph.strategyLevel.risks) delete r;
    graph.alphaBlocks.clear();
    graph.strategyLevel.rebalance = nullptr;
    graph.executionBlock = nullptr;
    graph.strategyLevel.risks.clear();
}

} // anon

class TestSupervision : public QObject
{
    Q_OBJECT

private slots:

    // --- BoundedQueue tests ---

    void boundedQueue_pushPop()
    {
        Supervision::BoundedQueue<int> q(5);
        QVERIFY(q.push(1));
        QVERIFY(q.push(2));
        QVERIFY(q.push(3));

        auto val = q.pop(std::chrono::milliseconds(100));
        QVERIFY(val.has_value());
        QCOMPARE(*val, 1);

        val = q.pop(std::chrono::milliseconds(100));
        QCOMPARE(*val, 2);
    }

    void boundedQueue_dropOldest()
    {
        Supervision::BoundedQueue<int> q(3, Supervision::OverflowPolicy::DropOldest);
        q.push(1);
        q.push(2);
        q.push(3);
        q.push(4); // drops 1

        QCOMPARE(q.droppedCount(), size_t(1));

        auto val = q.pop(std::chrono::milliseconds(100));
        QCOMPARE(*val, 2); // oldest surviving
    }

    void boundedQueue_dropNewest()
    {
        Supervision::BoundedQueue<int> q(2, Supervision::OverflowPolicy::DropNewest);
        QVERIFY(q.push(1));
        QVERIFY(q.push(2));
        QVERIFY(!q.push(3)); // rejected

        QCOMPARE(q.droppedCount(), size_t(1));

        auto val = q.pop(std::chrono::milliseconds(100));
        QCOMPARE(*val, 1);
    }

    void boundedQueue_timeout()
    {
        Supervision::BoundedQueue<int> q(5);
        auto val = q.pop(std::chrono::milliseconds(50));
        QVERIFY(!val.has_value());
    }

    void boundedQueue_shutdown()
    {
        Supervision::BoundedQueue<int> q(5);
        q.push(1);
        q.shutdown();

        QVERIFY(!q.isRunning());
        auto val = q.pop(std::chrono::milliseconds(50));
        // After shutdown, pop should return the remaining item or nullopt
        // It returns existing items first
        if (val.has_value()) {
            QCOMPARE(*val, 1);
        }
    }

    void boundedQueue_sizeTracking()
    {
        Supervision::BoundedQueue<int> q(10);
        QCOMPARE(q.size(), size_t(0));
        q.push(1);
        q.push(2);
        QCOMPARE(q.size(), size_t(2));
        q.pop(std::chrono::milliseconds(10));
        QCOMPARE(q.size(), size_t(1));
    }

    void boundedQueue_stringType()
    {
        Supervision::BoundedQueue<QString> q(3);
        q.push("hello");
        q.push("world");

        auto val = q.pop(std::chrono::milliseconds(100));
        QCOMPARE(*val, QString("hello"));
    }

    // --- StrategyRuntime tests ---

    void runtime_createAndStart()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("test-strategy", graph, &exec, &repo);
        QCOMPARE(runtime.name(), QString("test-strategy"));
        QVERIFY(!runtime.isRunning());
        QVERIFY(!runtime.isCrashed());
        QVERIFY(runtime.isHealthy());

        runtime.start();
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QVERIFY(runtime.isRunning());
        QVERIFY(runtime.isHealthy());

        runtime.stop();
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QVERIFY(!runtime.isRunning());
    }

    void runtime_startEmitsSignal()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("sig-test", graph, &exec, &repo);
        QSignalSpy startSpy(&runtime, &Supervision::StrategyRuntime::started);

        runtime.start();
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(startSpy.at(0).at(0).toString(), QString("sig-test"));

        runtime.stop();
        QThread::msleep(50);
    }

    void runtime_stopEmitsSignal()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("stop-test", graph, &exec, &repo);
        runtime.start();
        QThread::msleep(50);

        QSignalSpy stopSpy(&runtime, &Supervision::StrategyRuntime::stopped);
        runtime.stop();
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(stopSpy.count(), 1);
    }

    void runtime_markCrashed()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("crash-test", graph, &exec, &repo);
        runtime.start();
        QThread::msleep(50);

        QSignalSpy crashSpy(&runtime, &Supervision::StrategyRuntime::crashed);
        runtime.markCrashed("Test crash reason");
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QVERIFY(runtime.isCrashed());
        QVERIFY(!runtime.isHealthy());
        QCOMPARE(runtime.lastError(), QString("Test crash reason"));
        QCOMPARE(crashSpy.count(), 1);
    }

    void runtime_heartbeat()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("hb-test", graph, &exec, &repo);
        QCOMPARE(runtime.pipelineRunCount(), 0);

        runtime.heartbeat();
        runtime.heartbeat();
        runtime.heartbeat();

        QCOMPARE(runtime.pipelineRunCount(), 3);
        QVERIFY(runtime.lastHeartbeat().isValid());
    }

    void runtime_restartCount()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("rc-test", graph, &exec, &repo);
        QCOMPARE(runtime.restartCount(), 0);
        runtime.incrementRestartCount();
        runtime.incrementRestartCount();
        QCOMPARE(runtime.restartCount(), 2);
    }

    void runtime_doubleStartIgnored()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("dbl-test", graph, &exec, &repo);
        QSignalSpy startSpy(&runtime, &Supervision::StrategyRuntime::started);

        runtime.start();
        runtime.start(); // should be ignored
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(startSpy.count(), 1);
        runtime.stop();
        QThread::msleep(50);
    }

    void runtime_hasRunner()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        auto graph = makeTestGraph();

        Supervision::StrategyRuntime runtime("runner-test", graph, &exec, &repo);
        QVERIFY(runtime.runner() != nullptr);
    }

    // --- Supervisor tests ---

    void supervisor_addStrategy()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        QSignalSpy addSpy(&supervisor, &Supervision::Supervisor::strategyAdded);

        supervisor.addStrategy("strategy-1", [&]() {
            return new Supervision::StrategyRuntime(
                "strategy-1", makeTestGraph(), &exec, &repo);
        });

        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(supervisor.strategyCount(), 1);
        QCOMPARE(addSpy.count(), 1);
        QVERIFY(supervisor.isHealthy("strategy-1"));

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_removeStrategy()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("removable", [&]() {
            return new Supervision::StrategyRuntime(
                "removable", makeTestGraph(), &exec, &repo);
        });
        QThread::msleep(50);

        QSignalSpy removeSpy(&supervisor, &Supervision::Supervisor::strategyRemoved);
        supervisor.removeStrategy("removable");
        QCoreApplication::processEvents();

        QCOMPARE(supervisor.strategyCount(), 0);
        QCOMPARE(removeSpy.count(), 1);
    }

    void supervisor_multipleStrategies()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;

        supervisor.addStrategy("strat-a", [&]() {
            return new Supervision::StrategyRuntime(
                "strat-a", makeTestGraph(), &exec, &repo);
        });
        supervisor.addStrategy("strat-b", [&]() {
            return new Supervision::StrategyRuntime(
                "strat-b", makeTestGraph(), &exec, &repo);
        });
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(supervisor.strategyCount(), 2);
        QVERIFY(supervisor.isHealthy("strat-a"));
        QVERIFY(supervisor.isHealthy("strat-b"));

        auto names = supervisor.strategyNames();
        QVERIFY(names.contains("strat-a"));
        QVERIFY(names.contains("strat-b"));

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_crashAndRestart()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        int factoryCalls = 0;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("crash-restart", [&]() {
            factoryCalls++;
            return new Supervision::StrategyRuntime(
                "crash-restart", makeTestGraph(), &exec, &repo);
        }, Supervision::RestartPolicy::OnFailure);

        QThread::msleep(50);
        QCoreApplication::processEvents();
        QCOMPARE(factoryCalls, 1);

        // Simulate crash
        auto* rt = supervisor.runtime("crash-restart");
        QVERIFY(rt != nullptr);
        rt->markCrashed("Simulated crash");
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QVERIFY(supervisor.isCrashed("crash-restart"));

        QSignalSpy restartSpy(&supervisor, &Supervision::Supervisor::strategyRestarted);
        supervisor.checkHealth();
        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(restartSpy.count(), 1);
        QCOMPARE(supervisor.restartCount("crash-restart"), 1);
        QCOMPARE(factoryCalls, 2);

        // New runtime should be healthy
        QVERIFY(supervisor.isHealthy("crash-restart"));

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_neverRestartPolicy()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        int factoryCalls = 0;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("never-restart", [&]() {
            factoryCalls++;
            return new Supervision::StrategyRuntime(
                "never-restart", makeTestGraph(), &exec, &repo);
        }, Supervision::RestartPolicy::Never);

        QThread::msleep(50);
        QCoreApplication::processEvents();

        auto* rt = supervisor.runtime("never-restart");
        rt->markCrashed("Crash");
        QThread::msleep(50);

        supervisor.checkHealth();
        QCoreApplication::processEvents();

        // Should NOT restart
        QCOMPARE(factoryCalls, 1);
        QCOMPARE(supervisor.restartCount("never-restart"), 0);

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_maxRestartExceeded()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        supervisor.setMaxRestartAttempts(2);

        QSignalSpy gaveUpSpy(&supervisor, &Supervision::Supervisor::strategyGaveUp);

        supervisor.addStrategy("limited", [&]() {
            return new Supervision::StrategyRuntime(
                "limited", makeTestGraph(), &exec, &repo);
        }, Supervision::RestartPolicy::OnFailure);

        QThread::msleep(50);
        QCoreApplication::processEvents();

        // Crash and restart twice
        for (int i = 0; i < 2; ++i) {
            auto* rt = supervisor.runtime("limited");
            rt->markCrashed("Crash #" + QString::number(i + 1));
            QThread::msleep(50);
            supervisor.checkHealth();
            QThread::msleep(50);
            QCoreApplication::processEvents();
        }

        QCOMPARE(supervisor.restartCount("limited"), 2);

        // Third crash - should give up
        auto* rt = supervisor.runtime("limited");
        rt->markCrashed("Crash #3");
        QThread::msleep(50);
        supervisor.checkHealth();
        QCoreApplication::processEvents();

        QCOMPARE(gaveUpSpy.count(), 1);
        // Restart count should still be 2 (no new restart happened)
        QCOMPARE(supervisor.restartCount("limited"), 2);

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_healthCheckCompleted()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("health-test", [&]() {
            return new Supervision::StrategyRuntime(
                "health-test", makeTestGraph(), &exec, &repo);
        });

        QThread::msleep(50);
        QCoreApplication::processEvents();

        QSignalSpy healthSpy(&supervisor, &Supervision::Supervisor::healthCheckCompleted);
        supervisor.checkHealth();
        QCoreApplication::processEvents();

        QCOMPARE(healthSpy.count(), 1);

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_duplicateAddIgnored()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("dup", [&]() {
            return new Supervision::StrategyRuntime(
                "dup", makeTestGraph(), &exec, &repo);
        });
        supervisor.addStrategy("dup", [&]() {
            return new Supervision::StrategyRuntime(
                "dup", makeTestGraph(), &exec, &repo);
        }); // should be ignored

        QCOMPARE(supervisor.strategyCount(), 1);

        supervisor.stopAll();
        QThread::msleep(50);
    }

    void supervisor_isHealthyUnknownStrategy()
    {
        Supervision::Supervisor supervisor;
        QVERIFY(!supervisor.isHealthy("nonexistent"));
        QVERIFY(!supervisor.isCrashed("nonexistent"));
        QCOMPARE(supervisor.restartCount("nonexistent"), -1);
    }

    void supervisor_runtimeAccessor()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("accessor-test", [&]() {
            return new Supervision::StrategyRuntime(
                "accessor-test", makeTestGraph(), &exec, &repo);
        });

        QThread::msleep(50);

        auto* rt = supervisor.runtime("accessor-test");
        QVERIFY(rt != nullptr);
        QCOMPARE(rt->name(), QString("accessor-test"));

        QVERIFY(supervisor.runtime("nonexistent") == nullptr);

        supervisor.stopAll();
        QThread::msleep(50);
    }
};

#endif // TST_SUPERVISION_H
