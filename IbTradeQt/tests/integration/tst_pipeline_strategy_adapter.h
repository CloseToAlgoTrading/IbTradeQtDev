#ifndef TST_PIPELINE_STRATEGY_ADAPTER_H
#define TST_PIPELINE_STRATEGY_ADAPTER_H

#include <QtTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QUuid>

#include "Pipeline/PipelineFactory.h"
#include "Testing/MockMarketDataRouter.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Supervision/Supervisor.h"
#include "Strategies/Generic/ModelType.h"
#include "Strategies/Generic/cpipelinestrategyadapter.h"
#include "Backtest/MarketPriceStore.h"
#include "Backtest/SimulatedLedger.h"
#include "Common/IClock.h"
#include <QCoreApplication>
#include <memory>

class TestPipelineStrategyAdapter : public QObject
{
    Q_OBJECT

private:
    QJsonObject loadConfig(const QString& filename) {
        QString path = QString(SRCDIR) + "/../Strategies/DefaultPipelines/" + filename;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {};
        return QJsonDocument::fromJson(file.readAll()).object();
    }

    QVariantMap flattenConfig(const QJsonObject& config) {
        QVariantMap params;
        if (config.contains("name"))
            params["pipeline_name"] = config["name"].toString();
        if (config.contains("description"))
            params["pipeline_description"] = config["description"].toString();

        QJsonArray alphas = config.value("alphas").toArray();
        for (int i = 0; i < alphas.size(); ++i) {
            QJsonObject alpha = alphas[i].toObject();
            QString prefix = QString("alpha_%1_").arg(i);
            params[prefix + "blockId"] = alpha.value("blockId").toString();
            QJsonObject cfg = alpha.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it)
                params[prefix + it.key()] = it.value().toVariant();
        }

        QJsonObject rebalance = config.value("rebalance").toObject();
        if (!rebalance.isEmpty()) {
            params["rebalance_blockId"] = rebalance.value("blockId").toString();
            QJsonObject cfg = rebalance.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it)
                params["rebalance_" + it.key()] = it.value().toVariant();
        }

        QJsonArray risks = config.value("risks").toArray();
        for (int i = 0; i < risks.size(); ++i) {
            QJsonObject risk = risks[i].toObject();
            QString prefix = QString("risk_%1_").arg(i);
            params[prefix + "blockId"] = risk.value("blockId").toString();
            QJsonObject cfg = risk.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it)
                params[prefix + it.key()] = it.value().toVariant();
        }

        QJsonObject exec = config.value("execution").toObject();
        if (!exec.isEmpty()) {
            params["execution_blockId"] = exec.value("blockId").toString();
            QJsonObject cfg = exec.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it)
                params["execution_" + it.key()] = it.value().toVariant();
        }

        if (config.contains("mergePolicy"))
            params["mergePolicy"] = config["mergePolicy"].toString();

        return params;
    }

private slots:

    // --- Config flattening produces correct parameters ---

    void configFlattening_simpleMomentum()
    {
        QJsonObject config = loadConfig("simple_momentum_pipeline.json");
        QVERIFY(!config.isEmpty());

        QVariantMap params = flattenConfig(config);
        QCOMPARE(params["pipeline_name"].toString(), QString("Simple Momentum Strategy"));
        QCOMPARE(params["alpha_0_blockId"].toString(), QString("momentum-alpha"));
        QCOMPARE(params["alpha_0_period"].toInt(), 20);
        QCOMPARE(params["alpha_0_threshold"].toDouble(), 0.02);
        QCOMPARE(params["risk_0_blockId"].toString(), QString("max-position-risk"));
        QCOMPARE(params["risk_0_maxPositionSize"].toDouble(), 1000.0);
        QCOMPARE(params["rebalance_blockId"].toString(), QString("simple-rebalance"));
        QCOMPARE(params["execution_blockId"].toString(), QString("market-order-execution"));
    }

    void configFlattening_dualAlpha()
    {
        QJsonObject config = loadConfig("dual_alpha_pipeline.json");
        QVERIFY(!config.isEmpty());

        QVariantMap params = flattenConfig(config);
        QCOMPARE(params["pipeline_name"].toString(), QString("Dual Alpha Strategy"));
        QCOMPARE(params["alpha_0_blockId"].toString(), QString("momentum-alpha"));
        QCOMPARE(params["alpha_1_blockId"].toString(), QString("mean-reversion-alpha"));
        QCOMPARE(params["alpha_1_stdDevThreshold"].toDouble(), 2.0);
        QCOMPARE(params["mergePolicy"].toString(), QString("weighted-vote"));
    }

    // --- Config JSON round-trip ---

    void configJsonRoundTrip()
    {
        QJsonObject original = loadConfig("simple_momentum_pipeline.json");
        QVERIFY(!original.isEmpty());

        QJsonObject wrapper;
        wrapper["pipelineConfig"] = original;
        wrapper["modelType"] = 13; // STRATEGY_PIPELINE enum value
        wrapper["m_Name"] = "Pipeline Test";

        QVERIFY(wrapper.contains("pipelineConfig"));
        QJsonObject restored = wrapper["pipelineConfig"].toObject();
        QCOMPARE(restored["name"].toString(), original["name"].toString());

        QJsonArray alphas = restored["alphas"].toArray();
        QCOMPARE(alphas.size(), original["alphas"].toArray().size());
    }

    // --- PipelineFactory builds graph from config ---

    void factory_buildGraphFromSimpleMomentum()
    {
        QJsonObject config = loadConfig("simple_momentum_pipeline.json");
        MockExecutionAdapter exec;

        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        QCOMPARE(graph.alphaBlocks.size(), 1);
        QCOMPARE(graph.alphaBlocks[0]->id(), QString("momentum-alpha"));
        QVERIFY(graph.strategyLevel.rebalance != nullptr);
        QCOMPARE(graph.strategyLevel.risks.size(), 1);
        QVERIFY(graph.executionBlock != nullptr);
        QVERIFY(graph.mergePolicy == nullptr);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    void factory_buildGraphFromDualAlpha()
    {
        QJsonObject config = loadConfig("dual_alpha_pipeline.json");
        auto graph = Pipeline::PipelineFactory::buildGraph(config);
        QCOMPARE(graph.alphaBlocks.size(), 2);
        QVERIFY(graph.mergePolicy != nullptr);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
        delete graph.mergePolicy;
    }

    // --- Supervisor manages pipeline runtime from factory ---

    void supervisor_managesPipelineRuntime()
    {
        QJsonObject config = loadConfig("simple_momentum_pipeline.json");
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        IBComm::MarketDataRouter router;

        Supervision::Supervisor supervisor;

        QString name = "test-pipeline-strategy";
        supervisor.addStrategy(name, [&]() {
            return Pipeline::PipelineFactory::createRuntime(
                name, config, &exec, &repo, &router);
        });

        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(supervisor.strategyCount(), 1);
        QVERIFY(supervisor.isHealthy(name));

        auto* rt = supervisor.runtime(name);
        QVERIFY(rt != nullptr);
        QCOMPARE(rt->name(), name);

        supervisor.removeStrategy(name);
        QCOMPARE(supervisor.strategyCount(), 0);
    }

    // --- Pipeline runtime receives ticks from router ---

    void runtime_receivesTicksFromRouter()
    {
        QJsonObject config = loadConfig("simple_tick_pipeline.json");
        MockExecutionAdapter exec;
        MockPositionRepository repo;

        auto* runtime = Pipeline::PipelineFactory::createRuntime(
            "tick-test", config, &exec, &repo);

        MockMarketDataRouter mockRouter;
        runtime->connectToMockRouter(&mockRouter);

        for (int i = 0; i < 25; ++i) {
            mockRouter.simulateTick("AAPL", 100.0 + i * 0.5, 100.10 + i * 0.5);
        }

        QVERIFY(runtime->runner()->collectedSignals().size() > 0);

        delete runtime;
    }

    // --- Full lifecycle: create, start, feed ticks, stop ---

    void fullLifecycle_createStartFeedStop()
    {
        QJsonObject config = loadConfig("simple_momentum_pipeline.json");
        MockExecutionAdapter exec;
        MockPositionRepository repo;
        IBComm::MarketDataRouter router;

        Supervision::Supervisor supervisor;

        QString name = "lifecycle-test";
        supervisor.addStrategy(name, [&]() {
            return Pipeline::PipelineFactory::createRuntime(
                name, config, &exec, &repo, &router);
        });

        QThread::msleep(50);
        QCoreApplication::processEvents();

        QCOMPARE(supervisor.strategyCount(), 1);
        QVERIFY(supervisor.isHealthy(name));

        for (int i = 0; i < 25; ++i) {
            router.onTickPrice(1, "TSLA", 300.0 + i * 1.0, 300.10 + i * 1.0);
        }

        QThread::msleep(50);
        QCoreApplication::processEvents();

        supervisor.stopAll();
        QThread::msleep(50);
    }

    // --- Parameter editing updates config ---

    void parameterEditing_updatesConfig()
    {
        QJsonObject config = loadConfig("simple_momentum_pipeline.json");
        QVariantMap params = flattenConfig(config);

        QCOMPARE(params["alpha_0_period"].toInt(), 20);
        params["alpha_0_period"] = 50;

        QJsonArray alphas = config["alphas"].toArray();
        QJsonObject alpha = alphas[0].toObject();
        QJsonObject alphaCfg = alpha["config"].toObject();
        alphaCfg["period"] = params["alpha_0_period"].toInt();
        alpha["config"] = alphaCfg;
        alphas[0] = alpha;
        config["alphas"] = alphas;

        QJsonArray updatedAlphas = config["alphas"].toArray();
        QCOMPARE(updatedAlphas[0].toObject()["config"].toObject()["period"].toInt(), 50);

        MockExecutionAdapter exec;
        auto graph = Pipeline::PipelineFactory::buildGraph(config, &exec);
        QCOMPARE(graph.alphaBlocks.size(), 1);

        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    // --- ModelType enum value is correct ---

    void modelType_strategyPipelineExists()
    {
        QCOMPARE(static_cast<int>(ModelType::STRATEGY_PIPELINE), 13);
    }

    /// CPipelineStrategyAdapter as IDataSubscriptionPort: begin → setDesired → end
    /// runs deferred merge + refresh (pure backtest: no IB; exercises store + refresh path).
    void cpipelinestrategyadapter_subscriptionEpoch_deferredMergeAfterEnd()
    {
        QJsonObject pipelineConfig;
        pipelineConfig[QStringLiteral("alphas")] = QJsonArray();
        pipelineConfig[QStringLiteral("risks")] = QJsonArray();

        CPipelineStrategyAdapter adapter;
        adapter.setId(QUuid::createUuid());
        adapter.setName(QStringLiteral("SubEpochTest"));
        adapter.setPipelineConfig(pipelineConfig);

        Backtest::MarketPriceStore priceStore;
        auto clock = std::make_unique<SimulatedClock>();
        auto ledger = std::make_unique<Backtest::SimulatedLedger>(100'000.0, &priceStore);
        MockExecutionAdapter execPort;

        CPipelineStrategyAdapter::BacktestContext ctx;
        ctx.execPort = &execPort;
        ctx.clock = clock.get();
        ctx.ledger = ledger.get();
        adapter.injectBacktestContext(ctx);

        QVERIFY(adapter.start());
        Pipeline::StrategyPipelineRunner* runner = adapter.backtestPipelineRunner();
        QVERIFY(runner != nullptr);

        adapter.beginPipelineEvaluation();
        adapter.setDesiredSymbols(QStringLiteral("demoOwner"), {QStringLiteral("AAPL")});
        adapter.endPipelineEvaluation();
        QCoreApplication::processEvents();

        QVERIFY(adapter.backtestPipelineRunner() == runner);
    }
};

#endif // TST_PIPELINE_STRATEGY_ADAPTER_H
