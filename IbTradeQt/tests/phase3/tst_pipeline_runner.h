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
#include "Blocks/SimpleRebalanceBlock.h"
#include "Blocks/StaticListSelectionBlock.h"
#include "Pipeline/IDataSubscriptionPort.h"
#include "Pipeline/IHistoricalRead.h"
#include "Pipeline/PipelineRuntimeContext.h"
#include "Pipeline/SemanticModelDataMapper.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Testing/MockMarketDataRouter.h"
#include "Strategies/Generic/UnifiedModelData.h"

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

/// Records IDataSubscriptionPort calls for integration tests (order + payloads).
/// File scope (not anonymous namespace) so moc and TUs agree on one type.
/// Distinct name from phase1/tst_subscription_request_store.h (also has a recorder).
class RunnerRecordingSubscriptionPort : public Pipeline::IDataSubscriptionPort {
public:
    QStringList events;

    void setDesiredSymbols(const QString& ownerId, const QVector<QString>& symbols) override
    {
        QString joined;
        for (const QString& s : symbols) {
            if (!joined.isEmpty())
                joined += QLatin1Char(',');
            joined += s;
        }
        events.append(QStringLiteral("setDesired:%1:%2").arg(ownerId, joined));
    }

    void setDesiredSymbolsWithKinds(const QString& ownerId, const QVector<QString>& symbols,
                                    quint32 kindMask) override
    {
        QString joined;
        for (const QString& s : symbols) {
            if (!joined.isEmpty())
                joined += QLatin1Char(',');
            joined += s;
        }
        events.append(
            QStringLiteral("setDesiredKinds:%1:%2:%3").arg(ownerId, joined).arg(kindMask));
    }

    void clearOwner(const QString& ownerId) override
    {
        events.append(QStringLiteral("clearOwner:%1").arg(ownerId));
    }

    void clearAll() override { events.append(QStringLiteral("clearAll")); }

    void beginPipelineEvaluation() override { events.append(QStringLiteral("begin")); }

    void endPipelineEvaluation() override { events.append(QStringLiteral("end")); }
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

    /// Strong integration: real runner + BlockGraph + recording IDataSubscriptionPort.
    /// Expect begin → StaticListSelectionBlock setDesired → end (per pipelineCompleted path).
    void subscriptionEpoch_recordingPort_seesBeginSelectionThenEnd()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        Blocks::StaticListSelectionBlock selection;
        {
            QJsonObject cfg;
            QJsonArray syms;
            syms.append(QStringLiteral("AAPL"));
            cfg[QStringLiteral("symbols")] = syms;
            selection.setConfig(cfg);
        }

        RunnerTestAlpha alpha(100.0);
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.selectionBlocks.append(&selection);
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;

        RunnerRecordingSubscriptionPort recording;
        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();
        {
            Pipeline::PipelineRuntimeContext ctx;
            ctx.subscription = &recording;
            runner.setRuntimeContext(ctx);
        }

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        runner.setUniverse({QStringLiteral("AAPL")});
        mockRouter.simulateTick(QStringLiteral("AAPL"), 101.0, 102.0);
        runner.runPipeline();

        QVERIFY(!recording.events.isEmpty());
        QCOMPARE(recording.events.first(), QStringLiteral("begin"));
        QCOMPARE(recording.events.last(), QStringLiteral("end"));
        bool sawAapl = false;
        for (const QString& e : recording.events) {
            if (e.startsWith(QStringLiteral("setDesired:")) && e.contains(QStringLiteral("AAPL"))) {
                sawAapl = true;
                break;
            }
        }
        QVERIFY2(sawAapl, "selection block should register AAPL on the subscription port");
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
        // Buy: rebalance produces a default long target without an existing position (Sell would be skipped).
        sig.direction = Pipeline::Signal::Buy;
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
        struct HistoricalStub : public Pipeline::IHistoricalRead {
            QVector<Pipeline::HistoricalBarSnapshot> getBars(
                const QString&,
                const QString&,
                const QString&,
                const QDateTime&,
                const QDateTime&,
                Pipeline::HistoricalReadPolicy = Pipeline::HistoricalReadPolicy::PreferCache) override
            {
                QVector<Pipeline::HistoricalBarSnapshot> bars;
                Pipeline::HistoricalBarSnapshot s;
                s.close = 100.0;
                bars.append(s);
                s.close = 100.0;
                bars.append(s);
                s.close = 110.0;
                bars.append(s);
                return bars;
            }
        };

        HistoricalStub hist;
        Blocks::MomentumAlphaBlock alpha;
        alpha.setConfig({{"period", 2}, {"threshold", 0.01}, {"topN", 1}});
        alpha.initialize();

        Pipeline::PipelineRuntimeContext ctx;
        ctx.historical = &hist;
        alpha.setRuntimeContext(&ctx);

        Pipeline::ModelDataList in =
            Pipeline::SemanticMapping::buildModelDataFromSymbols({QStringLiteral("AAPL")});
        Pipeline::ModelDataList out = alpha.processSemantic(in, QStringLiteral("c1"));
        QVERIFY(out);
        QCOMPARE(out->size(), 1);
        QCOMPARE(out->at(0).symbol, QStringLiteral("AAPL"));
        QCOMPARE(out->at(0).direction, DIRECTION_UP);

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
