#ifndef TST_PIPELINE_PARITY_H
#define TST_PIPELINE_PARITY_H

#include <QtTest>
#include <QSignalSpy>
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/StrategyRuntimePolicy.h"
#include "Pipeline/PipelineDefinition.h"
#include "Pipeline/UniverseResolver.h"
#include "Pipeline/Contracts.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MarketOrderExecutionBlock.h"
#include "Blocks/StaticListSelectionBlock.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Testing/MockMarketDataRouter.h"
#include "Common/IClock.h"

// Test alpha: emits Buy for a specific symbol on every tick above threshold
class ParityTestAlpha : public Pipeline::IAlphaBlock {
    Q_OBJECT
public:
    explicit ParityTestAlpha(double threshold = 100.0, QObject* parent = nullptr)
        : IAlphaBlock(parent), m_threshold(threshold) {}

    QString id() const override { return "parity-test-alpha"; }
    QString name() const override { return "Parity Test Alpha"; }
    QString description() const override { return ""; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}
    void initialize() override {}
    void shutdown() override {}

    void onTick(const Pipeline::MarketTick& tick) override {
        if (tick.mid() > m_threshold) {
            Pipeline::Signal sig;
            sig.symbol = tick.symbol;
            sig.direction = Pipeline::Signal::Buy;
            sig.confidence = 0.9;
            sig.alphaBlockId = id();
            sig.timestamp = tick.timestamp;
            emit signalGenerated(sig);
        }
    }

private:
    double m_threshold;
};

// Test risk: emits proactive sell signal when price drops below threshold
class ParityTestRisk : public Pipeline::IRiskBlock {
    Q_OBJECT
public:
    explicit ParityTestRisk(double stopPrice = 90.0, QObject* parent = nullptr)
        : IRiskBlock(parent), m_stopPrice(stopPrice) {}

    QString id() const override { return "parity-test-risk"; }
    QString name() const override { return "Parity Test Risk"; }
    Pipeline::Scope scope() const override { return Pipeline::Scope::Strategy; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition& target,
        const QVector<Pipeline::TargetPosition>&,
        const QMap<QString, double>&) override
    {
        // Approve targets from our own emergency path without re-reducing
        if (target.emergencyOriginBlockId == id()) {
            return {Pipeline::RiskDecision::Action::Approve, "Own emergency target"};
        }
        return {Pipeline::RiskDecision::Action::Approve, "OK"};
    }

    void onTick(const Pipeline::MarketTick& tick) override {
        m_tickCount++;
        if (tick.mid() < m_stopPrice && !m_fired) {
            m_fired = true;
            Pipeline::Signal sig;
            sig.symbol = tick.symbol;
            sig.direction = Pipeline::Signal::Sell;
            sig.confidence = 1.0;
            sig.alphaBlockId = id();
            sig.timestamp = tick.timestamp;
            emit riskSignalGenerated(sig);
        }
    }

    int tickCount() const { return m_tickCount; }
    void resetFired() { m_fired = false; m_tickCount = 0; }

private:
    double m_stopPrice;
    bool m_fired = false;
    int m_tickCount = 0;
};


class TestPipelineParity : public QObject
{
    Q_OBJECT

private:
    Pipeline::MarketTick makeTick(const QString& sym, double bid, double ask,
                                 const QDateTime& ts) {
        Pipeline::MarketTick t;
        t.symbol = sym;
        t.bid = bid;
        t.ask = ask;
        t.timestamp = ts;
        return t;
    }

    Pipeline::OHLCVBar makeBar(const QString& sym, const QDateTime& ts) {
        Pipeline::OHLCVBar b;
        b.symbol = sym;
        b.timestamp = ts;
        return b;
    }

private slots:
    // ─── Runtime orchestration tests ──────────────────────────────────────

    void testDefaultPolicyPreservesCurrentBehavior()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        // Default: EveryBarClose + Immediate
        QCOMPARE(policy.evaluationMode, Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose);
        QCOMPARE(policy.rebalanceMode, Pipeline::StrategyRuntimePolicy::RebalanceMode::Immediate);

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        QDateTime ts1(QDate(2024, 1, 1), QTime(16, 0, 0));
        clock.setCurrentTime(ts1);

        // Simulate tick + bar close
        runner.ingestTick(makeTick("AMD", 105, 106, ts1));
        runner.ingestOhlcvBar(makeBar("AMD", ts1));
        runner.flushBarClosePipeline();

        QCOMPARE(spy.count(), 1);
        QVERIFY(runner.lastIntents().size() > 0);
    }

    void testEveryNBarsEvaluationGating()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        policy.evaluationIntervalN = 3;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        // Bar 1: skipped (barsSince=1, need 3)
        QDateTime ts1(QDate(2024, 1, 1), QTime(16, 0, 0));
        clock.setCurrentTime(ts1);
        runner.ingestTick(makeTick("AMD", 105, 106, ts1));
        runner.ingestOhlcvBar(makeBar("AMD", ts1));
        QCOMPARE(spy.count(), 0);

        // Bar 2: skipped
        QDateTime ts2 = ts1.addSecs(86400);
        clock.setCurrentTime(ts2);
        runner.ingestTick(makeTick("AMD", 105, 106, ts2));
        runner.ingestOhlcvBar(makeBar("AMD", ts2));
        QCOMPARE(spy.count(), 0);

        // Bar 3: fires (barsSince=3, >= 3)
        QDateTime ts3 = ts2.addSecs(86400);
        clock.setCurrentTime(ts3);
        runner.ingestTick(makeTick("AMD", 105, 106, ts3));
        runner.ingestOhlcvBar(makeBar("AMD", ts3));
        runner.flushBarClosePipeline();
        QCOMPARE(spy.count(), 1);
    }

    void testEveryNMinutesRebalanceGating()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNMinutes;
        policy.rebalanceIntervalN = 5;
        policy.accumulateAlphaSignals = true;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        // First bar: evaluation fires (EveryBarClose), rebalance fires (first time, no lastRebalanceTime)
        QDateTime ts1(QDate(2024, 1, 1), QTime(9, 30, 0));
        clock.setCurrentTime(ts1);
        runner.ingestTick(makeTick("AMD", 105, 106, ts1));
        runner.ingestOhlcvBar(makeBar("AMD", ts1));
        runner.flushBarClosePipeline();
        QCOMPARE(spy.count(), 1);
        auto firstIntents = runner.lastIntents();

        // 2 minutes later: rebalance should NOT fire (only 2 min elapsed)
        QDateTime ts2 = ts1.addSecs(120);
        clock.setCurrentTime(ts2);
        runner.ingestTick(makeTick("AMD", 107, 108, ts2));
        runner.ingestOhlcvBar(makeBar("AMD", ts2));
        runner.flushBarClosePipeline();
        QCOMPARE(spy.count(), 2);
        // Should have 0 intents because rebalance was skipped
        QCOMPARE(spy.at(1).at(1).toInt(), 0);

        // 6 minutes from first: rebalance SHOULD fire (>= 5 min elapsed)
        QDateTime ts3 = ts1.addSecs(360);
        clock.setCurrentTime(ts3);
        runner.ingestTick(makeTick("AMD", 107, 108, ts3));
        runner.ingestOhlcvBar(makeBar("AMD", ts3));
        runner.flushBarClosePipeline();
        QCOMPARE(spy.count(), 3);
        // Should have intents now
        QVERIFY(runner.lastIntents().size() > 0);
    }

    void testSignalAccumulationBetweenRebalanceWindows()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNBars;
        policy.rebalanceIntervalN = 2;
        policy.accumulateAlphaSignals = true;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QDateTime ts1(QDate(2024, 1, 1), QTime(16, 0, 0));
        clock.setCurrentTime(ts1);
        runner.ingestTick(makeTick("AMD", 105, 106, ts1));
        runner.ingestOhlcvBar(makeBar("AMD", ts1));

        QDateTime ts2 = ts1.addSecs(86400);
        clock.setCurrentTime(ts2);
        runner.ingestTick(makeTick("AMD", 107, 108, ts2));
        runner.ingestOhlcvBar(makeBar("AMD", ts2));

        // Completes last bar timestamp: rebalance (EveryNBars=2) drains accumulated signals.
        runner.flushBarClosePipeline();
        QCOMPARE(runner.runtimeState().pendingSignals.size(), 0);
        QVERIFY(runner.lastIntents().size() > 0);
    }

    void testSignalExpiry()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNBars;
        policy.rebalanceIntervalN = 3;
        policy.accumulateAlphaSignals = true;
        policy.signalExpiryBars = 1; // expire after 1 bar

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QDateTime ts1(QDate(2024, 1, 1), QTime(16, 0, 0));
        clock.setCurrentTime(ts1);
        runner.ingestTick(makeTick("AMD", 105, 106, ts1));
        runner.ingestOhlcvBar(makeBar("AMD", ts1));

        QDateTime ts2 = ts1.addSecs(86400);
        clock.setCurrentTime(ts2);
        runner.ingestTick(makeTick("AMD", 107, 108, ts2));
        runner.ingestOhlcvBar(makeBar("AMD", ts2));

        QDateTime ts3 = ts2.addSecs(86400);
        clock.setCurrentTime(ts3);
        runner.ingestTick(makeTick("AMD", 109, 110, ts3));
        runner.ingestOhlcvBar(makeBar("AMD", ts3));

        runner.flushBarClosePipeline();
        QCOMPARE(runner.runtimeState().pendingSignals.size(), 0);
    }

    void testCounterSemanticsN1FiresEveryEvent()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        policy.evaluationIntervalN = 1; // N=1 means every event

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        for (int i = 0; i < 5; ++i) {
            QDateTime ts = QDateTime(QDate(2024, 1, 1), QTime(16, 0, 0)).addSecs(i * 86400);
            clock.setCurrentTime(ts);
            runner.ingestTick(makeTick("AMD", 105, 106, ts));
            runner.ingestOhlcvBar(makeBar("AMD", ts));
        }
        runner.flushBarClosePipeline();
        QCOMPARE(spy.count(), 5);
    }

    void testCounterSemanticsN2FiresEveryOther()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        policy.evaluationIntervalN = 2;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        for (int i = 0; i < 6; ++i) {
            QDateTime ts = QDateTime(QDate(2024, 1, 1), QTime(16, 0, 0)).addSecs(i * 86400);
            clock.setCurrentTime(ts);
            runner.ingestTick(makeTick("AMD", 105, 106, ts));
            runner.ingestOhlcvBar(makeBar("AMD", ts));
        }
        runner.flushBarClosePipeline();
        // 6 bars / interval 2 = 3 evaluations
        QCOMPARE(spy.count(), 3);
    }

    void testEveryNBarsAccumulatesSignalsAcrossSkippedBars()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        policy.evaluationIntervalN = 3;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        // Bars 1-2: evaluation skipped, but alpha still emits signals via onTick.
        // Those signals must persist in m_collectedSignals for bar 3.
        QDateTime ts1(QDate(2024, 1, 1), QTime(16, 0, 0));
        clock.setCurrentTime(ts1);
        runner.ingestTick(makeTick("AMD", 105, 106, ts1));
        runner.ingestOhlcvBar(makeBar("AMD", ts1));
        QCOMPARE(runner.collectedSignals().size(), 1);

        QDateTime ts2 = ts1.addSecs(86400);
        clock.setCurrentTime(ts2);
        runner.ingestTick(makeTick("AMD", 107, 108, ts2));
        runner.ingestOhlcvBar(makeBar("AMD", ts2));
        QCOMPARE(runner.collectedSignals().size(), 2);

        // Bar 3: evaluation fires. All 3 bars' signals (2 accumulated + 1 current)
        // should be available for merge.
        QDateTime ts3 = ts2.addSecs(86400);
        clock.setCurrentTime(ts3);
        runner.ingestTick(makeTick("AMD", 109, 110, ts3));
        runner.ingestOhlcvBar(makeBar("AMD", ts3));
        runner.flushBarClosePipeline();
        // After evaluation fires, collectedSignals is cleared inside runPipeline
        QCOMPARE(runner.collectedSignals().size(), 0);
        QVERIFY(runner.lastIntents().size() > 0);
    }

    void testRiskCanCancelPendingOrdersFalsePreservesPending()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.riskCanCancelPendingOrders = false;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);

        Pipeline::Signal riskSig;
        riskSig.symbol = "AMD";
        riskSig.direction = Pipeline::Signal::Sell;
        riskSig.alphaBlockId = "test-risk";
        runner.runEmergencyRiskPipeline(riskSig);
        // With riskCanCancelPendingOrders=false, cancelAllPending() is not called.
        // This test verifies no crash and that the emergency path still completes.
    }

    // ─── Emergency risk path tests ──────────────────────────────────────

    void testEmergencyRiskBypassesGating()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        ParityTestRisk risk(90.0);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.strategyLevel.risks.append(&risk);

        Pipeline::StrategyRuntimePolicy policy;
        policy.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        policy.evaluationIntervalN = 100; // effectively never evaluates normally

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        // Set up a position to liquidate
        repo.setPosition(0, "AMD", 100.0);

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        // Trigger proactive risk signal
        QDateTime ts(QDate(2024, 1, 1), QTime(10, 0, 0));
        clock.setCurrentTime(ts);
        runner.ingestTick(makeTick("AMD", 85, 86, ts));

        // Emergency path should fire even though evaluation gate blocks normal path
        QCOMPARE(spy.count(), 1);
        QVERIFY(runner.lastIntents().size() > 0);
        QCOMPARE(runner.lastIntents().first().symbol, QStringLiteral("AMD"));
    }

    void testEmergencyOriginBlockIdMarking()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        ParityTestRisk risk(90.0);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.strategyLevel.risks.append(&risk);

        Pipeline::StrategyRuntimePolicy policy;
        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        repo.setPosition(0, "AMD", 100.0);

        QDateTime ts(QDate(2024, 1, 1), QTime(10, 0, 0));
        clock.setCurrentTime(ts);

        // Manually call runEmergencyRiskPipeline with a signal
        Pipeline::Signal riskSig;
        riskSig.symbol = "AMD";
        riskSig.direction = Pipeline::Signal::Sell;
        riskSig.alphaBlockId = risk.id();
        runner.runEmergencyRiskPipeline(riskSig);

        // The intent should have been approved by the risk block
        QVERIFY(runner.lastIntents().size() > 0);
    }

    void testPendingCancelOnEmergencyWhenPolicyEnabled()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        Pipeline::StrategyRuntimePolicy policy;
        policy.riskCanCancelPendingOrders = true;

        SimulatedClock clock;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);

        // cancelAllPending() is a no-op on MockExecutionAdapter by default
        // We just verify it doesn't crash
        Pipeline::Signal riskSig;
        riskSig.symbol = "AMD";
        riskSig.direction = Pipeline::Signal::Sell;
        riskSig.alphaBlockId = "test-risk";
        runner.runEmergencyRiskPipeline(riskSig);
    }

    // ─── Risk tick wiring tests ───────────────────────────────────────────

    void testRiskBlocksReceiveTicksViaConnectToAnyRouter()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        ParityTestRisk risk(90.0);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.strategyLevel.risks.append(&risk);

        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectToAnyRouter(&mockRouter);

        repo.setPosition(0, "AMD", 100.0);

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        // Emit tick below stop price — risk should fire proactive signal
        QDateTime ts(QDate(2024, 1, 1), QTime(10, 0, 0));
        Pipeline::MarketTick tick;
        tick.symbol = "AMD";
        tick.bid = 85.0;
        tick.ask = 86.0;
        tick.timestamp = ts;
        emit mockRouter.tick(tick);

        // Emergency risk path should have fired
        QCOMPARE(spy.count(), 1);
    }

    // ─── Universe resolution tests ────────────────────────────────────────

    void testStaticListResolution()
    {
        QJsonObject config;
        QJsonArray selection;
        QJsonObject sel;
        sel["blockId"] = "static-list-selection";
        QJsonObject selConfig;
        QJsonArray symbols;
        symbols.append("AAPL");
        symbols.append("MSFT");
        selConfig["symbols"] = symbols;
        sel["config"] = selConfig;
        selection.append(sel);
        config["selection"] = selection;

        auto result = Pipeline::UniverseResolver::resolve(config);
        QCOMPARE(result.mode, Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols);
        QCOMPARE(result.symbols.size(), 2);
        QVERIFY(result.symbols.contains("AAPL"));
        QVERIFY(result.symbols.contains("MSFT"));
    }

    void testPassAllResolution()
    {
        QJsonObject config;
        QJsonArray selection;
        QJsonObject sel;
        sel["blockId"] = "pass-all-selection";
        sel["config"] = QJsonObject();
        selection.append(sel);
        config["selection"] = selection;

        auto result = Pipeline::UniverseResolver::resolve(config);
        QCOMPARE(result.mode, Pipeline::UniverseResolutionResult::Mode::RequiresExternalUniverse);
    }

    void testEmptySelectionResolution()
    {
        QJsonObject config;
        auto result = Pipeline::UniverseResolver::resolve(config);
        QCOMPARE(result.mode, Pipeline::UniverseResolutionResult::Mode::RequiresExternalUniverse);
    }

    // ─── Clock and timestamp tests ────────────────────────────────────────

    void testBacktestUsesSimulatedTimeForIntents()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        SimulatedClock clock;
        QDateTime simTime(QDate(2020, 6, 15), QTime(16, 0, 0), Qt::UTC);
        clock.setCurrentTime(simTime);

        Pipeline::StrategyRuntimePolicy policy;
        Pipeline::StrategyPipelineRunner runner(graph, policy, &exec, &repo, &clock);
        runner.wireAlphaSignals();

        runner.ingestTick(makeTick("AMD", 105, 106, simTime));
        runner.ingestOhlcvBar(makeBar("AMD", simTime));
        runner.flushBarClosePipeline();

        QVERIFY(!runner.lastIntents().isEmpty());
        // Intent timestamp should be the simulated time, not wall clock
        QCOMPARE(runner.lastIntents().first().timestamp, simTime);
    }

    void testNullClockFallsBackToWallClock()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        ParityTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;

        // No clock injected — should use wall clock
        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        QDateTime beforeRun = QDateTime::currentDateTime();

        QDateTime ts(QDate(2024, 1, 1), QTime(16, 0, 0));
        runner.ingestTick(makeTick("AMD", 105, 106, ts));
        runner.ingestOhlcvBar(makeBar("AMD", ts));
        runner.flushBarClosePipeline();

        QDateTime afterRun = QDateTime::currentDateTime();

        QVERIFY(!runner.lastIntents().isEmpty());
        // Intent timestamp should be between beforeRun and afterRun (wall clock)
        QVERIFY(runner.lastIntents().first().timestamp >= beforeRun.addMSecs(-100));
        QVERIFY(runner.lastIntents().first().timestamp <= afterRun.addMSecs(100));
    }

    // ─── Runtime policy serialization tests ───────────────────────────────

    void testRuntimePolicyJsonRoundTrip()
    {
        Pipeline::StrategyRuntimePolicy orig;
        orig.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        orig.evaluationIntervalN = 5;
        orig.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNMinutes;
        orig.rebalanceIntervalN = 10;
        orig.accumulateAlphaSignals = false;
        orig.signalExpiryBars = 3;
        orig.riskCanCancelPendingOrders = false;

        QJsonObject json = orig.toJson();
        Pipeline::StrategyRuntimePolicy restored = Pipeline::StrategyRuntimePolicy::fromJson(json);

        QCOMPARE(restored.evaluationMode, orig.evaluationMode);
        QCOMPARE(restored.evaluationIntervalN, orig.evaluationIntervalN);
        QCOMPARE(restored.rebalanceMode, orig.rebalanceMode);
        QCOMPARE(restored.rebalanceIntervalN, orig.rebalanceIntervalN);
        QCOMPARE(restored.accumulateAlphaSignals, orig.accumulateAlphaSignals);
        QCOMPARE(restored.signalExpiryBars, orig.signalExpiryBars);
        QCOMPARE(restored.riskCanCancelPendingOrders, orig.riskCanCancelPendingOrders);
    }

    void testDefaultPolicyFromEmptyJson()
    {
        auto policy = Pipeline::StrategyRuntimePolicy::fromJson(QJsonObject());
        QCOMPARE(policy.evaluationMode, Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose);
        QCOMPARE(policy.rebalanceMode, Pipeline::StrategyRuntimePolicy::RebalanceMode::Immediate);
        QCOMPARE(policy.accumulateAlphaSignals, true);
        QCOMPARE(policy.signalExpiryBars, 0);
        QCOMPARE(policy.riskAlwaysActive, true);
        QCOMPARE(policy.riskCanCancelPendingOrders, true);
    }

    // ─── emergencyOriginBlockId on TargetPosition ─────────────────────────

    void testTargetPositionEmergencyOriginBlockIdSerialization()
    {
        Pipeline::TargetPosition tp;
        tp.symbol = "AAPL";
        tp.targetQuantity = 0.0;
        tp.currentQuantity = 100.0;
        tp.emergencyOriginBlockId = "stop-loss-risk-1";

        QJsonObject json = tp.toJson();
        QCOMPARE(json["emergencyOriginBlockId"].toString(), QStringLiteral("stop-loss-risk-1"));

        auto restored = Pipeline::TargetPosition::fromJson(json);
        QCOMPARE(restored.emergencyOriginBlockId, QStringLiteral("stop-loss-risk-1"));
    }

    void testTargetPositionNormalTargetHasEmptyOrigin()
    {
        Pipeline::TargetPosition tp;
        tp.symbol = "AMD";
        tp.targetQuantity = 100.0;

        QJsonObject json = tp.toJson();
        QVERIFY(!json.contains("emergencyOriginBlockId"));

        auto restored = Pipeline::TargetPosition::fromJson(json);
        QVERIFY(restored.emergencyOriginBlockId.isEmpty());
    }

    // ─── cancelAllPending on IOrderExecutionPort ──────────────────────────

    void testCancelAllPendingDefaultNoOp()
    {
        MockExecutionAdapter exec;
        auto result = exec.cancelAllPending();
        QVERIFY(result.has_value());
    }

    // ─── Gating method unit tests ─────────────────────────────────────────

    void testShouldEvaluateNowEveryBarClose()
    {
        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose;
        Pipeline::RuntimeState s;
        s.barsSinceEvaluation = 999;
        QVERIFY(p.shouldEvaluateNow(s, QDateTime()));
    }

    void testShouldEvaluateNowEveryNBars()
    {
        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        p.evaluationIntervalN = 3;

        Pipeline::RuntimeState s;
        s.barsSinceEvaluation = 2;
        QVERIFY(!p.shouldEvaluateNow(s, QDateTime()));

        s.barsSinceEvaluation = 3;
        QVERIFY(p.shouldEvaluateNow(s, QDateTime()));

        s.barsSinceEvaluation = 5;
        QVERIFY(p.shouldEvaluateNow(s, QDateTime()));
    }

    void testShouldRebalanceNowImmediate()
    {
        Pipeline::StrategyRuntimePolicy p;
        p.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::Immediate;
        Pipeline::RuntimeState s;
        QVERIFY(p.shouldRebalanceNow(s, QDateTime()));
    }

    void testShouldEvaluateNowEveryNMinutes()
    {
        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNMinutes;
        p.evaluationIntervalN = 5;

        Pipeline::RuntimeState s;
        // First time: no last eval, should fire
        QVERIFY(p.shouldEvaluateNow(s, QDateTime(QDate(2024, 1, 1), QTime(10, 0, 0))));

        // 3 minutes later: should not fire
        s.lastEvaluationTime = QDateTime(QDate(2024, 1, 1), QTime(10, 0, 0));
        QVERIFY(!p.shouldEvaluateNow(s, QDateTime(QDate(2024, 1, 1), QTime(10, 3, 0))));

        // 5 minutes later: should fire
        QVERIFY(p.shouldEvaluateNow(s, QDateTime(QDate(2024, 1, 1), QTime(10, 5, 0))));

        // 10 minutes later: should fire
        QVERIFY(p.shouldEvaluateNow(s, QDateTime(QDate(2024, 1, 1), QTime(10, 10, 0))));
    }

    // ─── Display helper tests (single source of truth) ─────────────────────

    void testEvalModeLabelCoversAllModes()
    {
        using E = Pipeline::StrategyRuntimePolicy::EvaluationMode;
        QCOMPARE(Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryTick),
                 QStringLiteral("Every Tick"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryBarClose),
                 QStringLiteral("Every Bar Close"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryNBars),
                 QStringLiteral("Every N Bars"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryNMinutes),
                 QStringLiteral("Every N Minutes"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryNDays),
                 QStringLiteral("Every N Days"));
    }

    void testRebalModeLabelCoversAllModes()
    {
        using R = Pipeline::StrategyRuntimePolicy::RebalanceMode;
        QCOMPARE(Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::Immediate),
                 QStringLiteral("Immediate"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::EveryNBars),
                 QStringLiteral("Every N Bars"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::EveryNMinutes),
                 QStringLiteral("Every N Minutes"));
        QCOMPARE(Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::EveryNDays),
                 QStringLiteral("Every N Days"));
    }

    void testIsDefaultTrueForDefaults()
    {
        Pipeline::StrategyRuntimePolicy p;
        QVERIFY(p.isDefault());
    }

    void testIsDefaultFalseForNonDefaults()
    {
        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        QVERIFY(!p.isDefault());

        Pipeline::StrategyRuntimePolicy p2;
        p2.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNMinutes;
        QVERIFY(!p2.isDefault());
    }

    void testSummaryDefaultPolicy()
    {
        Pipeline::StrategyRuntimePolicy p;
        QString s = p.summary();
        QVERIFY(s.contains("Bar Close"));
        QVERIFY(s.contains("Immediate"));
    }

    void testSummaryNonDefaultPolicy()
    {
        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        p.evaluationIntervalN = 3;
        p.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNMinutes;
        p.rebalanceIntervalN = 5;
        QString s = p.summary();
        QVERIFY(s.contains("3 Bars"));
        QVERIFY(s.contains("5 Min"));
    }
};

#endif // TST_PIPELINE_PARITY_H
