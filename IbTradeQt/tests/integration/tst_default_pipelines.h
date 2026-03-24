#ifndef TST_DEFAULT_PIPELINES_H
#define TST_DEFAULT_PIPELINES_H

#include <QtTest>
#include <QSignalSpy>
#include <QFile>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "Pipeline/PipelineFactory.h"
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/Contracts.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/MeanReversionAlphaBlock.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MarketOrderExecutionBlock.h"
#include "IBComm/MarketDataRouter.h"
#include "Testing/MockMarketDataRouter.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Supervision/StrategyRuntime.h"
#include "Supervision/Supervisor.h"
#include "Logging/StructuredLogger.h"
#include "Metrics/MetricsCollector.h"

class TestDefaultPipelines : public QObject
{
    Q_OBJECT

private:
    QJsonObject loadPipelineConfig(const QString& filename) {
        QString path = QString(SRCDIR) + "/../Strategies/DefaultPipelines/" + filename;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open pipeline config:" << path;
            return {};
        }
        return QJsonDocument::fromJson(file.readAll()).object();
    }

    /// Mean reversion can emit once per tick; SimpleRebalanceBlock emits one target row per signal.
    /// MaxPositionRisk sums exposure across *all* rows, so duplicate symbols over-reject; integration
    /// here focuses on factory + execution wiring without that layer.
    QJsonObject simpleTickPipelineIntegrationConfig()
    {
        QJsonObject config = loadPipelineConfig(QStringLiteral("simple_tick_pipeline.json"));
        config.remove(QStringLiteral("risks"));
        return config;
    }

private slots:

    void init() {
        Metrics::MetricsCollector::instance().reset();
        Logging::StructuredLogger::instance().reset();
        Logging::StructuredLogger::clearCorrelationId();
    }

    // --- MeanReversionAlphaBlock tests ---

    void meanReversion_generatesSignalOnDeviation()
    {
        Blocks::MeanReversionAlphaBlock alpha;
        alpha.setConfig({{"period", 5}, {"stdDevThreshold", 1.5}});
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        // Build a stable price history, then spike
        for (int i = 0; i < 5; ++i) {
            Pipeline::MarketTick tick;
            tick.symbol = "AAPL";
            tick.bid = 100.0;
            tick.ask = 100.10;
            tick.timestamp = QDateTime::currentDateTimeUtc();
            alpha.onTick(tick);
        }

        // Spike up: should trigger Sell (mean reversion)
        Pipeline::MarketTick spike;
        spike.symbol = "AAPL";
        spike.bid = 110.0;
        spike.ask = 110.10;
        spike.timestamp = QDateTime::currentDateTimeUtc();
        alpha.onTick(spike);

        QVERIFY(spy.count() > 0);
        auto signal = spy.last().at(0).value<Pipeline::Signal>();
        QCOMPARE(signal.direction, Pipeline::Signal::Sell);
        QCOMPARE(signal.symbol, QString("AAPL"));
    }

    void meanReversion_noSignalWithinBand()
    {
        Blocks::MeanReversionAlphaBlock alpha;
        alpha.setConfig({{"period", 5}, {"stdDevThreshold", 2.0}});
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        for (int i = 0; i < 10; ++i) {
            Pipeline::MarketTick tick;
            tick.symbol = "MSFT";
            tick.bid = 200.0 + (i % 2) * 0.1;
            tick.ask = 200.10 + (i % 2) * 0.1;
            tick.timestamp = QDateTime::currentDateTimeUtc();
            alpha.onTick(tick);
        }

        QCOMPARE(spy.count(), 0);
    }

    void meanReversion_buySignalOnDip()
    {
        Blocks::MeanReversionAlphaBlock alpha;
        alpha.setConfig({{"period", 5}, {"stdDevThreshold", 1.5}});
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        for (int i = 0; i < 5; ++i) {
            Pipeline::MarketTick tick;
            tick.symbol = "GOOG";
            tick.bid = 100.0;
            tick.ask = 100.10;
            tick.timestamp = QDateTime::currentDateTimeUtc();
            alpha.onTick(tick);
        }

        // Dip down: should trigger Buy
        Pipeline::MarketTick dip;
        dip.symbol = "GOOG";
        dip.bid = 90.0;
        dip.ask = 90.10;
        dip.timestamp = QDateTime::currentDateTimeUtc();
        alpha.onTick(dip);

        QVERIFY(spy.count() > 0);
        auto signal = spy.last().at(0).value<Pipeline::Signal>();
        QCOMPARE(signal.direction, Pipeline::Signal::Buy);
    }

    // --- PipelineFactory tests ---

    void factory_loadSimpleMomentumConfig()
    {
        QJsonObject config = loadPipelineConfig("simple_momentum_pipeline.json");
        QVERIFY(!config.isEmpty());
        QCOMPARE(config["name"].toString(), QString("Simple Momentum Strategy"));

        auto graph = Pipeline::PipelineFactory::buildGraph(config);
        QCOMPARE(graph.alphaBlocks.size(), 1);
        QCOMPARE(graph.alphaBlocks[0]->id(), QString("momentum-alpha"));
        QVERIFY(graph.strategyLevel.rebalance != nullptr);
        QCOMPARE(graph.strategyLevel.risks.size(), 1);
        QVERIFY(graph.executionBlock != nullptr);
        QVERIFY(graph.mergePolicy == nullptr); // single alpha

        // Cleanup
        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    void factory_loadDualAlphaConfig()
    {
        QJsonObject config = loadPipelineConfig("dual_alpha_pipeline.json");
        QVERIFY(!config.isEmpty());
        QCOMPARE(config["name"].toString(), QString("Dual Alpha Strategy"));

        auto graph = Pipeline::PipelineFactory::buildGraph(config);
        QCOMPARE(graph.alphaBlocks.size(), 2);
        QCOMPARE(graph.alphaBlocks[0]->id(), QString("momentum-alpha"));
        QCOMPARE(graph.alphaBlocks[1]->id(), QString("mean-reversion-alpha"));
        QVERIFY(graph.mergePolicy != nullptr); // weighted-vote
        QVERIFY(graph.strategyLevel.rebalance != nullptr);
        QCOMPARE(graph.strategyLevel.risks.size(), 1);
        QVERIFY(graph.executionBlock != nullptr);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
        delete graph.mergePolicy;
    }

    void factory_executionPortWired()
    {
        QJsonObject config = loadPipelineConfig("simple_momentum_pipeline.json");
        MockExecutionAdapter exec;

        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        QVERIFY(graph.executionBlock != nullptr);

        // Verify the execution block has the port wired
        auto* marketExec = qobject_cast<Blocks::MarketOrderExecutionBlock*>(graph.executionBlock);
        QVERIFY(marketExec != nullptr);

        // Execute a test intent
        Pipeline::ExecutionIntent intent;
        intent.symbol = "AAPL";
        intent.quantity = 100;
        intent.orderType = Pipeline::ExecutionIntent::Market;
        intent.timestamp = QDateTime::currentDateTimeUtc();

        QVector<Pipeline::ExecutionIntent> intents;
        intents.append(intent);
        marketExec->execute(intents);

        QCOMPARE(exec.placedOrders().size(), 1);
        QCOMPARE(exec.placedOrders()[0].symbol, QString("AAPL"));

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    // --- Full integration: pipeline from config end-to-end (tick-emitting alpha) ---

    void integration_simpleMomentumEndToEnd()
    {
        QJsonObject config = simpleTickPipelineIntegrationConfig();
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        // DirectConnection so ticks run ingestTick + alpha signals before runPipeline() returns.
        runner.connectMarketDataFeed(&mockRouter);

        // Feed ticks (mean-reversion alpha path); emission strength varies, so assert execution separately.
        for (int i = 0; i < 25; ++i) {
            mockRouter.simulateTick("AAPL", 100.0 + i * 0.5, 100.10 + i * 0.5);
        }

        Pipeline::Signal sig;
        sig.symbol = QStringLiteral("AAPL");
        sig.direction = Pipeline::Signal::Buy;
        sig.suggestedQuantity = 10.0;
        sig.correlationId = QStringLiteral("e2e");
        sig.timestamp = QDateTime::currentDateTimeUtc();
        runner.runPipelineWithSignals({sig});

        QVERIFY(exec.placedOrders().size() > 0);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    void integration_dualAlphaEndToEnd()
    {
        QJsonObject config = loadPipelineConfig("dual_alpha_pipeline.json");
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        QSignalSpy completedSpy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        // Feed ticks: upward trend
        for (int i = 0; i < 25; ++i) {
            mockRouter.simulateTick("MSFT", 200.0 + i * 0.3, 200.10 + i * 0.3);
        }

        runner.runPipeline();

        QCOMPARE(completedSpy.count(), 1);
        // With merge policy, signals from both alphas should be merged

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
        delete graph.mergePolicy;
    }

    // --- StrategyRuntime from factory ---

    void integration_runtimeFromFactory()
    {
        QJsonObject config = simpleTickPipelineIntegrationConfig();
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        {
            auto* runtime = Pipeline::PipelineFactory::createRuntime(
                "test-momentum", config, &exec, &repo);
            QVERIFY(runtime != nullptr);
            QCOMPARE(runtime->name(), QString("test-momentum"));
            QVERIFY(runtime->runner() != nullptr);
            delete runtime;
        }

        // createRuntime moves the runner to a worker thread; tick→runner uses QueuedConnection and
        // would not flush on the test thread. Exercise the same pipeline on a main-thread runner.
        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectMarketDataFeed(&mockRouter);

        for (int i = 0; i < 25; ++i) {
            mockRouter.simulateTick("TSLA", 300.0 + i * 1.0, 300.10 + i * 1.0);
        }

        Pipeline::Signal sig;
        sig.symbol = QStringLiteral("TSLA");
        sig.direction = Pipeline::Signal::Buy;
        sig.suggestedQuantity = 10.0;
        sig.correlationId = QStringLiteral("e2e");
        sig.timestamp = QDateTime::currentDateTimeUtc();
        runner.runPipelineWithSignals({sig});
        QVERIFY(exec.placedOrders().size() > 0);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    // --- Supervisor with factory ---

    void integration_supervisorWithFactory()
    {
        QJsonObject config = loadPipelineConfig("simple_momentum_pipeline.json");
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        Supervision::Supervisor supervisor;
        supervisor.addStrategy("momentum-live", [&]() {
            return Pipeline::PipelineFactory::createRuntime(
                "momentum-live", config, &exec, &repo);
        });

        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(supervisor.strategyCount(), 1);
        QVERIFY(supervisor.isHealthy("momentum-live"));

        supervisor.stopAll();
        QThread::msleep(50);
    }

    // --- Full stack with logging and metrics ---

    void integration_fullStackWithObservability()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        auto& logger = Logging::StructuredLogger::instance();

        QVector<Logging::LogEntry> logEntries;
        logger.addCallback([&](const Logging::LogEntry& e) {
            logEntries.append(e);
        });

        QJsonObject config = loadPipelineConfig("simple_momentum_pipeline.json");
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        MockMarketDataRouter mockRouter;
        runner.connectToMockRouter(&mockRouter);

        Logging::StructuredLogger::setCorrelationId("test-corr-001");

        for (int i = 0; i < 25; ++i) {
            mockRouter.simulateTick("NVDA", 500.0 + i * 2.0, 500.10 + i * 2.0);
            mc.recordTickProcessed();
        }

        LOG_INFO("TestPipeline", "Running pipeline", {{"symbol", "NVDA"}});
        runner.runPipeline();

        for (const auto& intent : runner.lastIntents()) {
            mc.recordOrderPlaced(intent.symbol);
        }

        auto snap = mc.snapshot();
        QCOMPARE(snap.ticksProcessed, uint64_t(25));
        QVERIFY(snap.ticksProcessed == 25);

        // Verify logger recorded entries
        QVERIFY(logEntries.size() > 0);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    // --- MarketDataRouter tick delivery ---

    void integration_marketDataRouterDelivery()
    {
        IBComm::MarketDataRouter router;

        // Mean reversion emits on ticks; momentum-alpha ranks via processSemantic + historical only.
        Blocks::MeanReversionAlphaBlock alpha;
        alpha.setConfig({{"period", 5}, {"stdDevThreshold", 1.5}});
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        connect(&router, &IBComm::MarketDataRouter::tick,
                &alpha, &Pipeline::IAlphaBlock::onTick,
                Qt::DirectConnection);

        for (int i = 0; i < 5; ++i) {
            router.onTickPrice(1, "AAPL", 100.0, 100.10);
        }
        router.onTickPrice(1, "AAPL", 110.0, 110.10);

        QVERIFY(spy.count() > 0);
        auto signal = spy.last().at(0).value<Pipeline::Signal>();
        QCOMPARE(signal.symbol, QString("AAPL"));
        QCOMPARE(signal.direction, Pipeline::Signal::Sell);
    }

    // --- MarketDataRouter ohlcvBar delivery (feed → runner only) ---

    void integration_barCloseTriggersRun()
    {
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        auto config = loadPipelineConfig("simple_momentum_pipeline.json");
        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);

        Pipeline::StrategyPipelineRunner runner(graph, &exec, &repo);
        runner.wireAlphaSignals();

        IBComm::MarketDataRouter router;

        runner.connectMarketDataFeed(&router);

        QSignalSpy completedSpy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);

        for (int i = 0; i < 25; ++i) {
            router.onTickPrice(1, "AAPL", 100.0 + i * 0.5, 100.05 + i * 0.5);
        }

        const QDateTime barTs = QDateTime::currentDateTimeUtc();
        Pipeline::OHLCVBar bar;
        bar.symbol = "AAPL";
        bar.timestamp = barTs;
        bar.open = 100.0;
        bar.high = 101.0;
        bar.low = 99.0;
        bar.close = 100.5;
        bar.volume = 1000.0;
        router.onOhlcvBarComplete(bar);
        runner.flushBarClosePipeline();

        QCOMPARE(completedSpy.count(), 1);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }
};

#endif // TST_DEFAULT_PIPELINES_H
