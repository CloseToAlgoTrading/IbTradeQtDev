#ifndef TST_PIPELINE_RUNNER_H
#define TST_PIPELINE_RUNNER_H

#include <QtTest>
#include <QSignalSpy>
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/SignalMergePolicies.h"
#include "Pipeline/BlockGraphSerializer.h"
#include "Pipeline/BlockRegistry.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MarketOrderExecutionBlock.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Testing/MockMarketDataRouter.h"

// Inline test alpha: emits Buy when mid > threshold
class RunnerTestAlpha : public Pipeline::IAlphaBlock {
    Q_OBJECT
public:
    explicit RunnerTestAlpha(double threshold = 151.0, QObject* parent = nullptr)
        : IAlphaBlock(parent), m_threshold(threshold) {}

    QString id() const override { return "runner-test-alpha"; }
    QString name() const override { return "Runner Test Alpha"; }
    QString description() const override { return ""; }
    QJsonObject config() const override { return {{"threshold", m_threshold}}; }
    void setConfig(const QJsonObject& c) override { m_threshold = c["threshold"].toDouble(151.0); }
    void initialize() override {}
    void shutdown() override {}

    void onTick(const Pipeline::MarketTick& tick) override {
        if (tick.mid() > m_threshold && !m_fired) {
            Pipeline::Signal sig;
            sig.symbol = tick.symbol;
            sig.direction = Pipeline::Signal::Buy;
            sig.confidence = 0.9;
            sig.alphaBlockId = id();
            sig.timestamp = tick.timestamp;
            emit signalGenerated(sig);
            m_fired = true;
        }
    }

    void resetFired() { m_fired = false; }

private:
    double m_threshold;
    bool m_fired = false;
};


class TestPipelineRunner : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        Pipeline::BlockRegistry::instance().clear();
    }

    void basicEndToEndPipeline()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        RunnerTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        // Tick below threshold -> no signal
        mockRouter.simulateTick("AAPL", 98.0, 99.0);
        QCOMPARE(runner.collectedSignals().size(), 0);

        // Tick above threshold -> signal generated
        mockRouter.simulateTick("AAPL", 101.0, 102.0);
        QCOMPARE(runner.collectedSignals().size(), 1);

        // Run pipeline
        runner.runPipeline();

        QCOMPARE(mockExec.placedOrders().size(), 1);
        QCOMPARE(mockExec.placedOrders()[0].symbol, QString("AAPL"));
        QVERIFY(mockExec.placedOrders()[0].quantity != 0.0);
    }

    void pipelineWithRiskRejection()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        RunnerTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        rebalance.setConfig({{"defaultQuantity", 500.0}});

        Blocks::MaxPositionRiskBlock risk;
        risk.setConfig({{"maxPositionSize", 200.0}, {"maxTotalExposure", 10000.0}});

        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.strategyLevel.risks.append(&risk);
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();

        QSignalSpy riskSpy(&runner, &Pipeline::StrategyPipelineRunner::riskRejection);

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        mockRouter.simulateTick("AAPL", 101.0, 102.0);
        runner.runPipeline();

        // Risk should modify (clamp to 200) not reject, since it's a Modify action
        // 500 > 200 -> Modify to delta that brings position to maxPositionSize
        QCOMPARE(mockExec.placedOrders().size(), 1);
        QVERIFY(std::abs(mockExec.placedOrders()[0].quantity) <= 200.0);
    }

    void pipelineWithMultipleAlphasAndMerge()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        RunnerTestAlpha alpha1(100.0);
        RunnerTestAlpha alpha2(100.0);

        Pipeline::WeightedVoteMerge mergePolicy;
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha1);
        graph.alphaBlocks.append(&alpha2);
        graph.mergePolicy = &mergePolicy;
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        mockRouter.simulateTick("AAPL", 101.0, 102.0);
        QCOMPARE(runner.collectedSignals().size(), 2);

        runner.runPipeline();

        // Merged signals -> one order
        QCOMPARE(mockExec.placedOrders().size(), 1);
    }

    void pipelineCompletedSignal()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        RunnerTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();

        QSignalSpy completedSpy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        mockRouter.simulateTick("AAPL", 101.0, 102.0);
        runner.runPipeline();

        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(completedSpy.at(0).at(1).toInt(), 1); // 1 intent
    }

    void runPipelineWithSignals()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        RunnerTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);

        Pipeline::Signal sig;
        sig.symbol = "MSFT";
        sig.direction = Pipeline::Signal::Buy;
        sig.confidence = 0.8;

        runner.runPipelineWithSignals({sig});

        QCOMPARE(mockExec.placedOrders().size(), 1);
        QCOMPARE(mockExec.placedOrders()[0].symbol, QString("MSFT"));
    }

    void emptyPipelineNoOrders()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        Pipeline::BlockGraph graph;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.runPipeline();

        QCOMPARE(mockExec.placedOrders().size(), 0);
    }

    void positionRepoQueriedDuringRebalance()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;
        mockRepo.setPosition(0, "AAPL", 50.0, 150.0);

        RunnerTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        mockRouter.simulateTick("AAPL", 101.0, 102.0);
        runner.runPipeline();

        QCOMPARE(mockExec.placedOrders().size(), 1);
        // Target is 100, current is 50, delta should be 50
        QCOMPARE(mockExec.placedOrders()[0].quantity, 50.0);
    }

    void executionFallsBackToPort()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        Pipeline::BlockGraph graph;
        // No executionBlock set, but executionPort is available

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);

        Pipeline::Signal sig;
        sig.symbol = "GOOG";
        sig.direction = Pipeline::Signal::Sell;
        sig.confidence = 0.7;

        Blocks::SimpleRebalanceBlock rebalance;
        graph.strategyLevel.rebalance = &rebalance;
        graph.alphaBlocks.clear();

        Pipeline::StrategyPipelineRunner runner2(graph, &mockExec, &mockRepo);
        runner2.runPipelineWithSignals({sig});

        QCOMPARE(mockExec.placedOrders().size(), 1);
    }

    void lastIntentsAccessor()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);

        Pipeline::Signal sig;
        sig.symbol = "TSLA";
        sig.direction = Pipeline::Signal::Buy;
        sig.confidence = 0.95;

        runner.runPipelineWithSignals({sig});

        QCOMPARE(runner.lastIntents().size(), 1);
        QCOMPARE(runner.lastIntents()[0].symbol, QString("TSLA"));
    }

    // --- BlockGraphSerializer tests ---

    void serializeEmptyGraph()
    {
        Pipeline::BlockGraph graph;
        QJsonObject json = Pipeline::BlockGraphSerializer::serialize(graph);

        QVERIFY(json.contains("selectionBlocks"));
        QVERIFY(json.contains("alphaBlocks"));
        QCOMPARE(json["selectionBlocks"].toArray().size(), 0);
        QCOMPARE(json["alphaBlocks"].toArray().size(), 0);
    }

    void serializeGraphWithBlocks()
    {
        Blocks::MomentumAlphaBlock alpha;
        alpha.setConfig({{"period", 30}, {"threshold", 0.05}});
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MaxPositionRiskBlock risk;
        Blocks::MarketOrderExecutionBlock exec;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.strategyLevel.risks.append(&risk);
        graph.executionBlock = &exec;

        QJsonObject json = Pipeline::BlockGraphSerializer::serialize(graph);

        QCOMPARE(json["alphaBlocks"].toArray().size(), 1);
        QCOMPARE(json["alphaBlocks"].toArray()[0].toObject()["id"].toString(),
                 QString("momentum-alpha"));
        QVERIFY(json.contains("executionBlock"));
        QCOMPARE(json["executionBlock"].toObject()["id"].toString(),
                 QString("market-order-execution"));
    }

    void roundTripSerializeDeserialize()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"momentum-alpha", "Momentum Alpha", "Alpha", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }});
        reg.registerBlock({"simple-rebalance", "Simple Rebalance", "Rebalance", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::SimpleRebalanceBlock(); }});
        reg.registerBlock({"max-position-risk", "Max Position Risk", "Risk", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::MaxPositionRiskBlock(); }});
        reg.registerBlock({"market-order-execution", "Market Order Execution", "Execution", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::MarketOrderExecutionBlock(); }});

        Blocks::MomentumAlphaBlock alpha;
        alpha.setConfig({{"period", 15}, {"threshold", 0.03}});
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MaxPositionRiskBlock risk;
        Blocks::MarketOrderExecutionBlock exec;

        Pipeline::BlockGraph original;
        original.alphaBlocks.append(&alpha);
        original.strategyLevel.rebalance = &rebalance;
        original.strategyLevel.risks.append(&risk);
        original.executionBlock = &exec;
        original.config = {{"strategyId", 42}};

        QJsonObject json = Pipeline::BlockGraphSerializer::serialize(original);
        auto result = Pipeline::BlockGraphSerializer::deserialize(json, reg);

        QVERIFY(result.has_value());
        QCOMPARE(result->alphaBlocks.size(), 1);
        QCOMPARE(result->alphaBlocks[0]->id(), QString("momentum-alpha"));
        QVERIFY(result->strategyLevel.rebalance != nullptr);
        QCOMPARE(result->strategyLevel.risks.size(), 1);
        QVERIFY(result->executionBlock != nullptr);
        QCOMPARE(result->config["strategyId"].toInt(), 42);

        // Verify config was preserved
        auto alphaConfig = result->alphaBlocks[0]->config();
        QCOMPARE(alphaConfig["period"].toInt(), 15);
        QCOMPARE(alphaConfig["threshold"].toDouble(), 0.03);

        // Clean up deserialized objects
        for (auto* b : result->alphaBlocks) delete b;
        delete result->strategyLevel.rebalance;
        for (auto* r : result->strategyLevel.risks) delete r;
        delete result->executionBlock;
    }

    void deserializeWithMissingBlockFails()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        QJsonObject json;
        QJsonArray alphas;
        alphas.append(QJsonObject{{"id", "nonexistent-block"}, {"config", QJsonObject()}});
        json["alphaBlocks"] = alphas;
        json["selectionBlocks"] = QJsonArray();
        json["strategyLevel"] = QJsonObject();
        json["portfolioLevel"] = QJsonObject();
        json["accountLevel"] = QJsonObject();
        json["config"] = QJsonObject();

        auto result = Pipeline::BlockGraphSerializer::deserialize(json, reg);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::NotFound);
    }

    void saveAndLoadFile()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"momentum-alpha", "M", "Alpha", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }});
        reg.registerBlock({"simple-rebalance", "S", "Rebalance", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::SimpleRebalanceBlock(); }});
        reg.registerBlock({"market-order-execution", "E", "Execution", "",
                          Pipeline::Scope::Strategy, {},
                          []() -> QObject* { return new Blocks::MarketOrderExecutionBlock(); }});

        Blocks::MomentumAlphaBlock alpha;
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock exec;

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &exec;
        graph.config = {{"name", "test-strategy"}};

        QString tmpPath = QDir::tempPath() + "/test_strategy.json";
        QVERIFY(Pipeline::BlockGraphSerializer::saveToFile(graph, tmpPath));

        auto loaded = Pipeline::BlockGraphSerializer::loadFromFile(tmpPath, reg);
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->alphaBlocks.size(), 1);
        QCOMPARE(loaded->config["name"].toString(), QString("test-strategy"));

        for (auto* b : loaded->alphaBlocks) delete b;
        delete loaded->strategyLevel.rebalance;
        delete loaded->executionBlock;

        QFile::remove(tmpPath);
    }

    void exampleBlocksMomentumAlpha()
    {
        Blocks::MomentumAlphaBlock alpha;
        alpha.setConfig({{"period", 3}, {"threshold", 0.01}});
        alpha.initialize();

        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        QDateTime base(QDate(2026, 3, 4), QTime(10, 0, 0), QTimeZone::utc());
        Pipeline::MarketTick t;
        t.symbol = "AAPL";

        // Build up price history (need > period entries for momentum calculation)
        for (int i = 0; i < 5; ++i) {
            t.bid = 100.0 + i * 2;
            t.ask = 100.5 + i * 2;
            t.timestamp = base.addSecs(i * 60);
            alpha.onTick(t);
        }

        // With 5 ticks over period=3, momentum should be significant
        QVERIFY(spy.count() > 0);

        auto sig = spy.last().at(0).value<Pipeline::Signal>();
        QCOMPARE(sig.symbol, QString("AAPL"));
        QCOMPARE(sig.direction, Pipeline::Signal::Buy);
        QVERIFY(sig.confidence > 0.0);

        alpha.shutdown();
    }

    void exampleBlocksMaxPositionRisk()
    {
        Blocks::MaxPositionRiskBlock risk;
        risk.setConfig({{"maxPositionSize", 200.0}, {"maxTotalExposure", 5000.0}});

        Pipeline::TargetPosition small;
        small.symbol = "AAPL";
        small.targetQuantity = 100.0;

        auto decision = risk.evaluate(small, {small}, {});
        QCOMPARE(decision.action, Pipeline::RiskDecision::Action::Approve);

        Pipeline::TargetPosition large;
        large.symbol = "MSFT";
        large.targetQuantity = 300.0;

        decision = risk.evaluate(large, {large}, {});
        QCOMPARE(decision.action, Pipeline::RiskDecision::Action::Modify);
        QVERIFY(decision.modifiedQuantity.has_value());
    }

    void exampleBlocksMarketOrderExecution()
    {
        MockExecutionAdapter mockExec;
        Blocks::MarketOrderExecutionBlock exec;
        exec.setExecutionPort(&mockExec);

        QSignalSpy orderSpy(&exec, &Pipeline::IExecutionBlock::orderPlaced);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "GOOG";
        intent.quantity = 50.0;
        intent.orderType = Pipeline::ExecutionIntent::Market;

        exec.execute({intent});

        QCOMPARE(orderSpy.count(), 1);
        QCOMPARE(mockExec.placedOrders().size(), 1);
    }

    void exampleBlocksDryRunExecution()
    {
        Blocks::MarketOrderExecutionBlock exec;
        // No execution port set -> dry run

        QSignalSpy orderSpy(&exec, &Pipeline::IExecutionBlock::orderPlaced);

        Pipeline::ExecutionIntent intent;
        intent.symbol = "AMZN";
        intent.quantity = 25.0;

        exec.execute({intent});

        QCOMPARE(orderSpy.count(), 1);
        QCOMPARE(orderSpy.at(0).at(1).toString(), QString("dry-run"));
    }
};

#endif // TST_PIPELINE_RUNNER_H
