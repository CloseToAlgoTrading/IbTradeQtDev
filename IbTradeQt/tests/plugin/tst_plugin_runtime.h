#ifndef TST_PLUGIN_RUNTIME_H
#define TST_PLUGIN_RUNTIME_H

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "Plugin/PluginLoader.h"
#include "Plugin/PluginWrappers.h"
#include "Pipeline/BlockRegistry.h"
#include "Pipeline/PipelineFactory.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Backtest/MarketPriceStore.h"
#include "Backtest/SimulatedLedger.h"
#include "Common/IClock.h"
#include "Strategies/Generic/cpipelinestrategyadapter.h"

class TestPluginRuntime : public QObject
{
    Q_OBJECT

private:
    QString repoRootDir() const
    {
        return QDir(QString::fromUtf8(SRCDIR)).absoluteFilePath(QStringLiteral(".."));
    }

    QString pluginApiIncludeDir() const
    {
        return QDir(repoRootDir()).filePath(QStringLiteral("plugins/api"));
    }

    QString examplePluginDir() const
    {
        return QDir(QString::fromUtf8(SRCDIR))
            .absoluteFilePath(QStringLiteral("../plugins/examples/strategy_suite_plugin"));
    }

    QString exampleManifestPath() const
    {
        return QDir(examplePluginDir()).filePath(QStringLiteral("plugin.json"));
    }

    QJsonObject makeExtensionManifest(const QString& extensionPointId,
                                      const QString& extensionId,
                                      const QString& name,
                                      bool supportsAsyncSemantic = false) const
    {
        return QJsonObject{
            {QStringLiteral("extension_point_id"), extensionPointId},
            {QStringLiteral("extension_id"), extensionId},
            {QStringLiteral("name"), name},
            {QStringLiteral("description"), QStringLiteral("Test extension")},
            {QStringLiteral("scope"), QStringLiteral("Strategy")},
            {QStringLiteral("default_config"), QJsonObject{}},
            {QStringLiteral("supports_async_semantic"), supportsAsyncSemantic}
        };
    }

    void writeFile(const QString& path, const QByteArray& content) const
    {
        QFile file(path);
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate),
                 qPrintable(file.errorString()));
        file.write(content);
        file.close();
    }

    QString buildTempPluginPackage(QTemporaryDir& dir,
                                   const QString& pluginId,
                                   const QByteArray& sourceCode,
                                   const QJsonArray& extensions,
                                   const QString& libraryFileName = QStringLiteral("libtemp_plugin.so"),
                                   const QStringList& extraCompilerArgs = {}) const
    {
        if (!dir.isValid()) {
            qWarning() << "Temporary plugin directory is invalid";
            return {};
        }
        writeFile(dir.filePath(QStringLiteral("plugin.cpp")), sourceCode);

        QProcess proc;
        proc.setWorkingDirectory(dir.path());

        QStringList args{
            QStringLiteral("-std=c++17"),
            QStringLiteral("-shared"),
            QStringLiteral("-fPIC"),
            QStringLiteral("plugin.cpp"),
            QStringLiteral("-I%1").arg(pluginApiIncludeDir()),
            QStringLiteral("-o"),
            libraryFileName
        };
        args.append(extraCompilerArgs);

        proc.start(QStringLiteral("g++"), args);
        if (!proc.waitForFinished(120000)) {
            qWarning() << "Temporary plugin build timed out";
            return {};
        }
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
            qWarning().noquote() << proc.readAllStandardError() + proc.readAllStandardOutput();
            return {};
        }

        const QJsonObject manifest{
            {QStringLiteral("manifest_version"), static_cast<int>(IBTRADE_PLUGIN_MANIFEST_VERSION_V1)},
            {QStringLiteral("plugin_id"), pluginId},
            {QStringLiteral("name"), pluginId},
            {QStringLiteral("version"), QStringLiteral("1.0.0")},
            {QStringLiteral("description"), QStringLiteral("Temporary test plugin")},
            {QStringLiteral("vendor"), QStringLiteral("ibtrade-tests")},
            {QStringLiteral("host_api_version"), static_cast<int>(IBTRADE_PLUGIN_ABI_VERSION_V1)},
            {QStringLiteral("library"), libraryFileName},
            {QStringLiteral("extensions"), extensions}
        };
        writeFile(dir.filePath(QStringLiteral("plugin.json")),
                  QJsonDocument(manifest).toJson(QJsonDocument::Indented));

        return dir.path();
    }

    void buildExamplePlugin()
    {
        QProcess proc;
        proc.setWorkingDirectory(examplePluginDir());
        proc.start(QStringLiteral("/bin/bash"), {QStringLiteral("build.sh")});
        QVERIFY2(proc.waitForFinished(120000), "Example plugin build timed out");
        QVERIFY2(proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0,
                 qPrintable(proc.readAllStandardError() + proc.readAllStandardOutput()));
        QVERIFY(QFileInfo::exists(
            QDir(examplePluginDir()).filePath(QStringLiteral("build/libibtrade_strategy_suite_plugin.so"))));
    }

    void loadExamplePlugin(Plugin::PluginLoader& loader)
    {
        auto result = loader.loadPlugin(examplePluginDir());
        if (!result.has_value()) {
            QFAIL(qPrintable(QString::fromStdString(result.error().message)));
        }
    }

private slots:
    void initTestCase()
    {
        buildExamplePlugin();
    }

    void init()
    {
        Pipeline::BlockRegistry::instance().clear();
    }

    void cleanup()
    {
        Pipeline::BlockRegistry::instance().clear();
    }

    void loadExamplePackage_registersAllExtensions()
    {
        Plugin::PluginLoader loader;
        loadExamplePlugin(loader);

        QCOMPARE(loader.pluginCount(), 1);
        QCOMPARE(loader.extensionCount(), 5);
        QCOMPARE(loader.listPlugins().first().pluginId,
                 QStringLiteral("com.ibtrade.example.strategy_suite"));

        QVERIFY(Pipeline::BlockRegistry::instance().contains(
            QStringLiteral("com.ibtrade.example.strategy_suite/static-selection")));
        QVERIFY(Pipeline::BlockRegistry::instance().contains(
            QStringLiteral("com.ibtrade.example.strategy_suite/semantic-alpha")));
        QVERIFY(Pipeline::BlockRegistry::instance().contains(
            QStringLiteral("com.ibtrade.example.strategy_suite/fixed-rebalance")));
        QVERIFY(Pipeline::BlockRegistry::instance().contains(
            QStringLiteral("com.ibtrade.example.strategy_suite/cap-risk")));
        QVERIFY(Pipeline::BlockRegistry::instance().contains(
            QStringLiteral("com.ibtrade.example.strategy_suite/host-execution")));
    }

    void loadFailsWhenDuplicateBlockIdExists()
    {
        Pipeline::BlockRegistry::instance().registerBlock({
            QStringLiteral("com.ibtrade.example.strategy_suite/static-selection"),
            QStringLiteral("dup"),
            QString(Pipeline::Category::Selection),
            QString(),
            Pipeline::Scope::Strategy,
            {},
            nullptr
        });

        Plugin::PluginLoader loader;
        auto result = loader.loadPlugin(examplePluginDir());
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
    }

    void loadFailsWhenManifestDeclaresDuplicateExtensionIds()
    {
        buildExamplePlugin();

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QFile::copy(exampleManifestPath(), dir.filePath(QStringLiteral("plugin.json"))));
        QVERIFY(QDir().mkpath(dir.filePath(QStringLiteral("build"))));
        QVERIFY(QFile::copy(
            QDir(examplePluginDir()).filePath(QStringLiteral("build/libibtrade_strategy_suite_plugin.so")),
            dir.filePath(QStringLiteral("build/libibtrade_strategy_suite_plugin.so"))));

        QFile manifest(dir.filePath(QStringLiteral("plugin.json")));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        QJsonObject obj = QJsonDocument::fromJson(manifest.readAll()).object();
        manifest.close();

        QJsonArray extensions = obj.value(QStringLiteral("extensions")).toArray();
        QVERIFY(extensions.size() >= 2);
        QJsonObject duplicate = extensions.at(1).toObject();
        duplicate[QStringLiteral("extension_id")] =
            extensions.at(0).toObject().value(QStringLiteral("extension_id")).toString();
        extensions[1] = duplicate;
        obj[QStringLiteral("extensions")] = extensions;

        QVERIFY(manifest.open(QIODevice::WriteOnly | QIODevice::Truncate));
        manifest.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        manifest.close();

        Plugin::PluginLoader loader;
        auto result = loader.loadPlugin(dir.path());
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
        QCOMPARE(loader.pluginCount(), 0);
        QCOMPARE(loader.extensionCount(), 0);
        QVERIFY(!Pipeline::BlockRegistry::instance().contains(
            QStringLiteral("com.ibtrade.example.strategy_suite/static-selection")));
    }

    void missingPluginBlockIdsDoNotFallbackToBuiltins()
    {
        QJsonObject config;
        config[QStringLiteral("selection")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("missing.selection.plugin")},
                {QStringLiteral("config"), QJsonObject{}}
            }
        };
        config[QStringLiteral("alphas")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("missing.alpha.plugin")},
                {QStringLiteral("config"), QJsonObject{}}
            }
        };
        config[QStringLiteral("rebalance")] = QJsonObject{
            {QStringLiteral("blockId"), QStringLiteral("missing.rebalance.plugin")},
            {QStringLiteral("config"), QJsonObject{}}
        };
        config[QStringLiteral("risks")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("missing.risk.plugin")},
                {QStringLiteral("config"), QJsonObject{}}
            }
        };
        config[QStringLiteral("execution")] = QJsonObject{
            {QStringLiteral("blockId"), QStringLiteral("missing.execution.plugin")},
            {QStringLiteral("config"), QJsonObject{}}
        };

        MockExecutionAdapter execPort;
        Pipeline::BlockGraph graph = Pipeline::PipelineFactory::buildGraph(config, &execPort);
        QCOMPARE(graph.selectionBlocks.size(), 0);
        QCOMPARE(graph.alphaBlocks.size(), 0);
        QVERIFY(graph.strategyLevel.rebalance == nullptr);
        QCOMPARE(graph.strategyLevel.risks.size(), 0);
        QVERIFY(graph.executionBlock == nullptr);
    }

    void selectionAndAlphaWrappersBridgeQtSignals()
    {
        Plugin::PluginLoader loader;
        loadExamplePlugin(loader);

        auto selectionResult = Pipeline::BlockRegistry::instance().createBlock(
            QStringLiteral("com.ibtrade.example.strategy_suite/static-selection"));
        QVERIFY(selectionResult.has_value());
        auto* selection = qobject_cast<Pipeline::ISelectionBlock*>(*selectionResult);
        QVERIFY(selection != nullptr);
        selection->setConfig(QJsonObject{{QStringLiteral("limit"), 1}});
        selection->initialize();
        const QVector<QString> selected = selection->select(
            {QStringLiteral("AMD"), QStringLiteral("NVDA"), QStringLiteral("SPY")});
        QCOMPARE(selected.size(), 1);
        QCOMPARE(selected.first(), QStringLiteral("AMD"));

        auto alphaResult = Pipeline::BlockRegistry::instance().createBlock(
            QStringLiteral("com.ibtrade.example.strategy_suite/semantic-alpha"));
        QVERIFY(alphaResult.has_value());
        auto* alpha = qobject_cast<Pipeline::IAlphaBlock*>(*alphaResult);
        QVERIFY(alpha != nullptr);
        alpha->setConfig(QJsonObject{
            {QStringLiteral("fixed_probability"), 0.91},
            {QStringLiteral("fixed_amount"), 42.0}
        });
        Pipeline::PipelineRuntimeContext alphaCtx;
        alphaCtx.holdings.insert(QStringLiteral("AMD"), 2.0);
        alpha->setRuntimeContext(&alphaCtx);
        auto* alphaSink = dynamic_cast<Plugin::PluginEventSink*>(alpha);
        QVERIFY(alphaSink != nullptr);
        QCOMPARE(alphaSink->pluginRuntimeContext(), &alphaCtx);
        alpha->initialize();

        QSignalSpy signalSpy(alpha, &Pipeline::IAlphaBlock::signalGenerated);
        alpha->onTick(Pipeline::MarketTick{
            QStringLiteral("AMD"), 100.0, 101.0, 1000.0, QDateTime::currentDateTime(), 1
        });
        QTRY_VERIFY(signalSpy.count() >= 1);
        const auto emittedSignal =
            qvariant_cast<Pipeline::Signal>(signalSpy.at(0).at(0));
        QCOMPARE(emittedSignal.symbol, QStringLiteral("AMD"));

        DataListPtr input = createDataList();
        input->append(UnifiedModelData(QStringLiteral("AMD")));
        input->append(UnifiedModelData(QStringLiteral("NVDA")));

        const Pipeline::ModelDataList out =
            alpha->processSemantic(input, QStringLiteral("corr-plugin-alpha"));
        QVERIFY(out);
        QCOMPARE(out->size(), 2);
        QCOMPARE(out->at(0).symbol, QStringLiteral("AMD"));
        QCOMPARE(out->at(0).amount, 42.0);
        QCOMPARE(out->at(0).probability, 0.91);

        delete selection;
        delete alpha;
    }

    void rebalanceRiskAndExecutionWrappersUseHostPorts()
    {
        Plugin::PluginLoader loader;
        loadExamplePlugin(loader);

        auto rebalanceResult = Pipeline::BlockRegistry::instance().createBlock(
            QStringLiteral("com.ibtrade.example.strategy_suite/fixed-rebalance"));
        auto riskResult = Pipeline::BlockRegistry::instance().createBlock(
            QStringLiteral("com.ibtrade.example.strategy_suite/cap-risk"));
        auto executionResult = Pipeline::BlockRegistry::instance().createBlock(
            QStringLiteral("com.ibtrade.example.strategy_suite/host-execution"));
        QVERIFY(rebalanceResult.has_value());
        QVERIFY(riskResult.has_value());
        QVERIFY(executionResult.has_value());

        auto* rebalance = qobject_cast<Pipeline::IRebalanceBlock*>(*rebalanceResult);
        auto* risk = qobject_cast<Pipeline::IRiskBlock*>(*riskResult);
        auto* execution = qobject_cast<Pipeline::IExecutionBlock*>(*executionResult);
        QVERIFY(rebalance && risk && execution);

        rebalance->setConfig(QJsonObject{{QStringLiteral("target_size"), 12.0}});
        risk->setConfig(QJsonObject{{QStringLiteral("max_abs_target"), 5.0}});

        MockExecutionAdapter execPort;
        Pipeline::PipelineRuntimeContext ctx;
        ctx.execution = &execPort;
        ctx.holdings.insert(QStringLiteral("AMD"), 2.0);
        rebalance->setRuntimeContext(&ctx);
        risk->setRuntimeContext(&ctx);
        execution->setRuntimeContext(&ctx);

        QVector<Pipeline::Signal> signalList;
        Pipeline::Signal signal;
        signal.symbol = QStringLiteral("AMD");
        signal.direction = Pipeline::Signal::Buy;
        signal.confidence = 0.8;
        signalList.push_back(signal);

        const QVector<Pipeline::TargetPosition> targets =
            rebalance->rebalance(signalList, {});
        QCOMPARE(targets.size(), 1);
        QCOMPARE(targets.first().targetQuantity, 12.0);
        QCOMPARE(targets.first().currentQuantity, 2.0);

        const Pipeline::RiskDecision decision =
            risk->evaluate(targets.first(), targets, {});
        QCOMPARE(decision.action, Pipeline::RiskDecision::Action::Modify);
        QVERIFY(decision.modifiedQuantity.has_value());
        QCOMPARE(*decision.modifiedQuantity, 5.0);

        QSignalSpy placedSpy(execution, &Pipeline::IExecutionBlock::orderPlaced);
        Pipeline::ExecutionIntent intent;
        intent.symbol = QStringLiteral("AMD");
        intent.quantity = 3.0;
        execution->execute({intent});
        QTRY_VERIFY(placedSpy.count() >= 1);
        QCOMPARE(execPort.orderCount(), 1);
        QCOMPARE(execPort.placedOrders().first().symbol, QStringLiteral("AMD"));

        delete rebalance;
        delete risk;
        delete execution;
    }

    void pipelineFactoryBuildsPluginBackedGraph()
    {
        Plugin::PluginLoader loader;
        loadExamplePlugin(loader);

        QJsonObject config;
        config[QStringLiteral("selection")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/static-selection")},
                {QStringLiteral("config"), QJsonObject{{QStringLiteral("limit"), 1}}}
            }
        };
        config[QStringLiteral("alphas")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/semantic-alpha")},
                {QStringLiteral("config"), QJsonObject{{QStringLiteral("fixed_amount"), 7.0}}}
            }
        };
        config[QStringLiteral("rebalance")] = QJsonObject{
            {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/fixed-rebalance")},
            {QStringLiteral("config"), QJsonObject{{QStringLiteral("target_size"), 7.0}}}
        };
        config[QStringLiteral("risks")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/cap-risk")},
                {QStringLiteral("config"), QJsonObject{{QStringLiteral("max_abs_target"), 7.0}}}
            }
        };
        config[QStringLiteral("execution")] = QJsonObject{
            {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/host-execution")},
            {QStringLiteral("config"), QJsonObject{}}
        };

        MockExecutionAdapter execPort;
        Pipeline::BlockGraph graph = Pipeline::PipelineFactory::buildGraph(config, &execPort);
        QCOMPARE(graph.selectionBlocks.size(), 1);
        QCOMPARE(graph.alphaBlocks.size(), 1);
        QVERIFY(graph.strategyLevel.rebalance != nullptr);
        QCOMPARE(graph.strategyLevel.risks.size(), 1);
        QVERIFY(graph.executionBlock != nullptr);
        QCOMPARE(graph.selectionBlocks.first()->id(),
                 QStringLiteral("com.ibtrade.example.strategy_suite/static-selection"));
        QCOMPARE(graph.alphaBlocks.first()->id(),
                 QStringLiteral("com.ibtrade.example.strategy_suite/semantic-alpha"));

        qDeleteAll(graph.selectionBlocks);
        qDeleteAll(graph.alphaBlocks);
        delete graph.strategyLevel.rebalance;
        qDeleteAll(graph.strategyLevel.risks);
        delete graph.executionBlock;
    }

    void pipelineStrategyAdapterRunsPluginBacktestFlow()
    {
        Plugin::PluginLoader loader;
        loadExamplePlugin(loader);

        QJsonObject pipelineConfig;
        pipelineConfig[QStringLiteral("selection")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/static-selection")},
                {QStringLiteral("config"), QJsonObject{{QStringLiteral("limit"), 1}}}
            }
        };
        pipelineConfig[QStringLiteral("alphas")] = QJsonArray();
        pipelineConfig[QStringLiteral("risks")] = QJsonArray{
            QJsonObject{
                {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/cap-risk")},
                {QStringLiteral("config"), QJsonObject{{QStringLiteral("max_abs_target"), 4.0}}}
            }
        };
        pipelineConfig[QStringLiteral("rebalance")] = QJsonObject{
            {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/fixed-rebalance")},
            {QStringLiteral("config"), QJsonObject{{QStringLiteral("target_size"), 9.0}}}
        };
        pipelineConfig[QStringLiteral("execution")] = QJsonObject{
            {QStringLiteral("blockId"), QStringLiteral("com.ibtrade.example.strategy_suite/host-execution")},
            {QStringLiteral("config"), QJsonObject{}}
        };

        CPipelineStrategyAdapter adapter;
        adapter.setId(QUuid::createUuid());
        adapter.setName(QStringLiteral("Plugin Backtest"));
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
        auto* runner = adapter.backtestPipelineRunner();
        QVERIFY(runner != nullptr);

        Pipeline::Signal signal;
        signal.symbol = QStringLiteral("AMD");
        signal.direction = Pipeline::Signal::Buy;
        signal.confidence = 0.8;
        runner->runPipelineWithSignals({signal});
        QCOMPARE(execPort.orderCount(), 1);
        QCOMPARE(execPort.placedOrders().first().symbol, QStringLiteral("AMD"));
        QCOMPARE(execPort.placedOrders().first().quantity, 4.0);
    }

    void malformedPluginApiTableIsRejected()
    {
        QTemporaryDir dir;
        const QString packagePath = buildTempPluginPackage(
            dir,
            QStringLiteral("com.ibtrade.tests.bad_api"),
            QByteArray(R"cpp(
#include "ibtrade_plugin_api.h"

extern "C" IBTRADE_PLUGIN_EXPORT const ibtrade_plugin_api_v1* ibtrade_get_plugin_api_v1(void)
{
    static const ibtrade_plugin_api_v1 api = {
        IBTRADE_PLUGIN_ABI_VERSION_V1,
        sizeof(ibtrade_plugin_api_v1),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };
    return &api;
}
)cpp"),
            QJsonArray{
                makeExtensionManifest(QStringLiteral("pipeline.alpha"),
                                      QStringLiteral("com.ibtrade.tests.bad_api/alpha"),
                                      QStringLiteral("Bad API Alpha"))
            });
        QVERIFY(!packagePath.isEmpty());

        Plugin::PluginLoader loader;
        auto result = loader.loadPlugin(packagePath);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
        QCOMPARE(loader.loadFailures().size(), 1);
        QVERIFY(loader.loadFailures().first().message.contains(QStringLiteral("missing required plugin API functions")));
    }

    void missingPluginExportIsRejected()
    {
        QTemporaryDir dir;
        const QString packagePath = buildTempPluginPackage(
            dir,
            QStringLiteral("com.ibtrade.tests.missing_export"),
            QByteArray(R"cpp(
#include "ibtrade_plugin_api.h"

extern "C" IBTRADE_PLUGIN_EXPORT int not_the_expected_symbol(void)
{
    return 7;
}
)cpp"),
            QJsonArray{
                makeExtensionManifest(QStringLiteral("pipeline.alpha"),
                                      QStringLiteral("com.ibtrade.tests.missing_export/alpha"),
                                      QStringLiteral("Missing Export Alpha"))
            });
        QVERIFY(!packagePath.isEmpty());

        Plugin::PluginLoader loader;
        auto result = loader.loadPlugin(packagePath);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
        QCOMPARE(loader.loadFailures().size(), 1);
        QVERIFY(loader.loadFailures().first().message.contains(QStringLiteral("missing ibtrade_get_plugin_api_v1")));
    }

    void lateAsyncPluginCallbackAfterDestroyIsIgnoredSafely()
    {
        QTemporaryDir dir;
        const QString packagePath = buildTempPluginPackage(
            dir,
            QStringLiteral("com.ibtrade.tests.async_alpha"),
            QByteArray(R"cpp(
#include "ibtrade_plugin_api.h"

#include <chrono>
#include <cstring>
#include <string>
#include <thread>

namespace {

struct PluginState {
    const ibtrade_host_services_v1* host = nullptr;
};

ibtrade_owned_string_v1 make_string(const char* value)
{
    const size_t size = std::strlen(value);
    char* raw = new char[size + 1];
    std::memcpy(raw, value, size + 1);

    ibtrade_owned_string_v1 out{};
    out.data = raw;
    out.size = size;
    out.release = [](const char* data, size_t, void*) {
        delete[] data;
    };
    out.user_data = nullptr;
    return out;
}

} // namespace

extern "C" {

static void* create_extension(const char*, const ibtrade_host_services_v1* host, ibtrade_owned_string_v1* error_out)
{
    auto* state = new PluginState();
    state->host = host;
    if (error_out) {
        *error_out = {};
    }
    return state;
}

static void destroy_extension(void* instance)
{
    delete static_cast<PluginState*>(instance);
}

static int initialize_instance(void*, ibtrade_owned_string_v1* error_out)
{
    if (error_out) {
        *error_out = {};
    }
    return 1;
}

static void shutdown_instance(void*) {}

static int set_config_json(void*, const char*, ibtrade_owned_string_v1* error_out)
{
    if (error_out) {
        *error_out = {};
    }
    return 1;
}

static ibtrade_owned_string_v1 get_state_json(void*)
{
    return make_string("{}");
}

static ibtrade_owned_string_v1 invoke_json(void* instance,
                                           const char* operation,
                                           const char*,
                                           ibtrade_owned_string_v1* error_out)
{
    auto* state = static_cast<PluginState*>(instance);
    if (error_out) {
        *error_out = {};
    }

    if (std::string(operation ? operation : "") == "on_tick") {
        const ibtrade_host_services_v1* host = state->host;
        std::thread([host]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            if (host && host->emit_event_json) {
                host->emit_event_json(
                    host->user_data,
                    "alpha.signal_generated",
                    "{\"symbol\":\"AMD\",\"confidence\":0.5,\"direction\":0,"
                    "\"correlationId\":\"\",\"timestamp\":\"\",\"alphaBlockId\":\"async\","
                    "\"suggestedQuantity\":0.0}");
            }
        }).detach();
    }

    return make_string("{}");
}

static const ibtrade_plugin_api_v1 kPluginApi = {
    IBTRADE_PLUGIN_ABI_VERSION_V1,
    sizeof(ibtrade_plugin_api_v1),
    &create_extension,
    &destroy_extension,
    &initialize_instance,
    &shutdown_instance,
    &set_config_json,
    &get_state_json,
    &invoke_json
};

IBTRADE_PLUGIN_EXPORT const ibtrade_plugin_api_v1* ibtrade_get_plugin_api_v1(void)
{
    return &kPluginApi;
}

} // extern "C"
)cpp"),
            QJsonArray{
                makeExtensionManifest(QStringLiteral("pipeline.alpha"),
                                      QStringLiteral("com.ibtrade.tests.async_alpha/alpha"),
                                      QStringLiteral("Async Alpha"))
            },
            QStringLiteral("libasync_alpha.so"),
            {QStringLiteral("-pthread")});
        QVERIFY(!packagePath.isEmpty());

        Plugin::PluginLoader loader;
        auto loadResult = loader.loadPlugin(packagePath);
        if (!loadResult.has_value()) {
            QFAIL(qPrintable(QString::fromStdString(loadResult.error().message)));
        }

        auto alphaResult = Pipeline::BlockRegistry::instance().createBlock(
            QStringLiteral("com.ibtrade.tests.async_alpha/alpha"));
        QVERIFY(alphaResult.has_value());
        auto* alpha = qobject_cast<Pipeline::IAlphaBlock*>(*alphaResult);
        QVERIFY(alpha != nullptr);
        alpha->initialize();
        alpha->onTick(Pipeline::MarketTick{
            QStringLiteral("AMD"), 100.0, 101.0, 1000.0, QDateTime::currentDateTime(), 1
        });

        delete alpha;
        loader.clear();
        QTest::qWait(100);
        QVERIFY(true);
    }

    void manifestValidationRejectsWrongHostApiVersion()
    {
        buildExamplePlugin();

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QFile::copy(exampleManifestPath(), dir.filePath(QStringLiteral("plugin.json"))));
        QVERIFY(QDir().mkpath(dir.filePath(QStringLiteral("build"))));
        QVERIFY(QFile::copy(
            QDir(examplePluginDir()).filePath(QStringLiteral("build/libibtrade_strategy_suite_plugin.so")),
            dir.filePath(QStringLiteral("build/libibtrade_strategy_suite_plugin.so"))));

        QFile manifest(dir.filePath(QStringLiteral("plugin.json")));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        QJsonObject obj = QJsonDocument::fromJson(manifest.readAll()).object();
        manifest.close();
        obj[QStringLiteral("host_api_version")] = 999;
        QVERIFY(manifest.open(QIODevice::WriteOnly | QIODevice::Truncate));
        manifest.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        manifest.close();

        Plugin::PluginLoader loader;
        auto result = loader.loadPlugin(dir.path());
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
        QCOMPARE(loader.loadFailures().size(), 1);
        QVERIFY(loader.loadFailures().first().pluginId.contains(QStringLiteral("com.ibtrade.example.strategy_suite")));
    }
};

#endif // TST_PLUGIN_RUNTIME_H
