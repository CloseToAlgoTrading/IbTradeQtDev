#include <QApplication>
#include <QtTest>
#include "Pipeline/PipelineMetaTypes.h"

#include "phase1/tst_contracts.h"
#include "phase1/tst_merge_policies.h"
#include "phase1/tst_market_data_router.h"
#include "phase1/tst_block_interfaces.h"
#include "phase1/tst_expected.h"
#include "phase1/tst_scope.h"
#include "phase1/tst_subscription_request_store.h"
#include "phase2/tst_replay.h"
#include "phase2/tst_adapters.h"
#include "phase2/tst_integration.h"
#include "phase3/tst_block_registry.h"
#include "plugin/tst_plugin_runtime.h"
#include "phase3/tst_pipeline_runner.h"
#include "phase3/tst_semantic_mapping.h"
#include "phase4/tst_supervision.h"
#include "phase5/tst_observability.h"
#include "phase6/tst_benchmark.h"
#include "integration/tst_default_pipelines.h"
#include "integration/tst_pipeline_strategy_adapter.h"
#include "integration/tst_live_execution_wiring.h"
#include "integration/tst_typed_routers.h"
#include "integration/tst_phase_b_migration.h"
#include "integration/tst_phase_d_dispatcher_removal.h"
#include "integration/tst_phase_e_remaining_routers.h"
#include "backtest/tst_backtest.h"
#include "backtest/tst_yahoo_backtest.h"
#include "backtest/tst_market_session_utils.h"
#include "backtest/tst_instrument_metadata_resolver.h"
#include "backtest/tst_historical_data_manager_cache.h"
#include "backtest/tst_historical_range_normalizer.h"
#include "backtest/tst_yahoo_universe_validator.h"
#include "backtest/tst_backtest_preflight_coordinator.h"
#include "data_management/tst_data_management.h"
#include "backtest/tst_backtest_extended.h"
#include "backtest/tst_live_backtest.h"
#include "backtest/tst_backtest_engine_coverage.h"
#include "backtest/tst_momentum_three_stock_validation.h"
#include "backtest/tst_momentum_hundred_stock_validation.h"
#include "db/tst_dbhandler_disconnect.h"
#include "backtest/tst_workspace_session.h"
#include "backtest/tst_backtest_run_persistence.h"
#include "backtest/tst_backtest_statistics.h"
#include "backtest/tst_backtest_report_golden.h"
#include "backtest/tst_backtest_summary_formatter.h"
#include "backend/tst_storage_config.h"
#include "backend/tst_persistence_factory.h"
#include "backend/tst_model_tree_repository.h"
#include "backend/tst_system_backend.h"
#include "backend/tst_persistence.h"
#include "backend/tst_runtime.h"
#include "backend/tst_cli_proof.h"
#include "backend/tst_strategy_definition.h"
#include "backend/tst_strategy_catalog.h"
#include "pipeline/tst_pipeline_config_mutations.h"
#include "integration/tst_adapter_pure_backtest.h"
#include "integration/tst_semantic_e2e.h"
#include "parity/tst_pipeline_parity.h"
#include "blocks/tst_quant_momentum_blocks.h"
#include "ui/tst_runtime_policy_editor.h"
#include "ui/tst_backtest_workspace_coordinator.h"
#include "ui/tst_trading_readiness.h"

// When IBTRADING_TEST_CLASS is set (e.g. TestMomentumThreeStockValidation), only that suite runs.
// Use with a function filter so other QObject classes do not report "function not found":
//   IBTRADING_TEST_CLASS=TestMomentumThreeStockValidation ./release/ibtrading_tests momentum_top2_three_stock_histPriceValidationHtml
#define IBTRADING_RUN_TEST(Class, VarName) \
    do { \
        if (testClassFilter.isEmpty() || testClassFilter == QStringLiteral(#Class)) { \
            Class VarName; \
            status |= QTest::qExec(&VarName, argc, argv); \
        } \
    } while (0)

int main(int argc, char *argv[])
{
    // Default QTest per-function limit is 300s. The 100-stock Yahoo + long replay can exceed it.
    if (qEnvironmentVariable("IBTRADING_TEST_CLASS").trimmed()
            == QStringLiteral("TestMomentumHundredStockValidation")
        && qEnvironmentVariableIsEmpty("QTEST_FUNCTION_TIMEOUT")) {
        qputenv("QTEST_FUNCTION_TIMEOUT", QByteArray::number(2LL * 60 * 60 * 1000)); // 2 hours
    }

    QApplication app(argc, argv);
    Pipeline::registerPipelineMetaTypes();
    int status = 0;
    const QString testClassFilter = qEnvironmentVariable("IBTRADING_TEST_CLASS").trimmed();

    // Phase 1
    IBTRADING_RUN_TEST(TestContracts, tc);
    IBTRADING_RUN_TEST(TestMergePolicies, tc);
    IBTRADING_RUN_TEST(TestMarketDataRouter, tc);
    IBTRADING_RUN_TEST(TestBlockInterfaces, tc);
    IBTRADING_RUN_TEST(TestExpected, tc);
    IBTRADING_RUN_TEST(TestScope, tc);
    IBTRADING_RUN_TEST(TestSubscriptionRequestStore, tc);

    // Phase 2
    IBTRADING_RUN_TEST(TestReplay, tc);
    IBTRADING_RUN_TEST(TestAdapters, tc);
    IBTRADING_RUN_TEST(TestIntegration, tc);

    // Phase 3
    IBTRADING_RUN_TEST(TestBlockRegistry, tc);
    IBTRADING_RUN_TEST(TestPluginRuntime, tc);
    IBTRADING_RUN_TEST(TestPipelineRunner, tc);
    IBTRADING_RUN_TEST(TestSemanticMapping, tc);

    // Phase 4
    IBTRADING_RUN_TEST(TestSupervision, tc);

    // Phase 5
    IBTRADING_RUN_TEST(TestObservability, tc);

    // Phase 6 - Benchmarks
    IBTRADING_RUN_TEST(TestBenchmark, tc);

    // Integration - Default Pipelines
    IBTRADING_RUN_TEST(TestDefaultPipelines, tc);

    // Integration - Pipeline Strategy Adapter
    IBTRADING_RUN_TEST(TestPipelineStrategyAdapter, tc);

    // Integration - Live Execution Wiring
    IBTRADING_RUN_TEST(TestLiveExecutionWiring, tc);

    // Integration - Typed Routers
    IBTRADING_RUN_TEST(TestTypedRouters, tc);

    // Phase B - Legacy subscriber migration
    IBTRADING_RUN_TEST(TestPhaseBMigration, tc);

    // Phase D - CDispatcher removal verification
    IBTRADING_RUN_TEST(TestPhaseD_DispatcherRemoval, tc);

    // Phase E - Remaining router signals
    IBTRADING_RUN_TEST(TestPhaseE_RemainingRouters, tc);

    // Backtester — Phase 1 foundations
    IBTRADING_RUN_TEST(TestIClock, tc);
    IBTRADING_RUN_TEST(TestMarketPriceStore, tc);
    IBTRADING_RUN_TEST(TestSimulatedLedger, tc);
    IBTRADING_RUN_TEST(TestSimulatedExecutionAdapter, tc);
    IBTRADING_RUN_TEST(TestBacktestMetricsCollector, tc);
    IBTRADING_RUN_TEST(TestMarketDataReplayerExtensions, tc);
    IBTRADING_RUN_TEST(TestCsvHistoricalDataSource, tc);
    IBTRADING_RUN_TEST(TestJsonlHistoricalDataSource, tc);
    IBTRADING_RUN_TEST(TestFullBacktestSession, tc);

    // Backtester — Yahoo Finance data source + benchmark comparison
    IBTRADING_RUN_TEST(TestHistoricalDataManagerCache, tc);
    IBTRADING_RUN_TEST(TestHistoricalRangeNormalizer, tc);
    IBTRADING_RUN_TEST(TestYahooUniverseValidator, tc);
    IBTRADING_RUN_TEST(TestBacktestPreFlightCoordinator, tc);
    IBTRADING_RUN_TEST(TestDataManagement, tc);
    IBTRADING_RUN_TEST(TestMarketSessionUtils, tc);
    IBTRADING_RUN_TEST(TestInstrumentMetadataResolver, tc);
    IBTRADING_RUN_TEST(TestYahooFinanceDataSource, tc);
    IBTRADING_RUN_TEST(TestBenchmarkComparison, tc);
    IBTRADING_RUN_TEST(TestMACrossoverBacktest, tc);
    IBTRADING_RUN_TEST(TestYahooBacktestSessionMockE2E, tc);
    IBTRADING_RUN_TEST(TestYahooBacktestSessionMockFailure, tc);
    IBTRADING_RUN_TEST(TestYahooBacktestPipelineVariants, tc);
    IBTRADING_RUN_TEST(TestBacktestEngineCoverage, tc);
    IBTRADING_RUN_TEST(TestMomentumThreeStockValidation, tc);
    IBTRADING_RUN_TEST(TestMomentumHundredStockValidation, tc);
    IBTRADING_RUN_TEST(TestDbHandlerDisconnect, tc);
    IBTRADING_RUN_TEST(TestWorkspaceSession, tc);
    IBTRADING_RUN_TEST(TestBacktestRunPersistence, tc);
    IBTRADING_RUN_TEST(TestBacktestStatistics, tc);
    IBTRADING_RUN_TEST(TestBacktestReportGolden, tc);
    IBTRADING_RUN_TEST(TestBacktestSummaryFormatter, tc);

    // Extended LEGO backtests (CSV + default pipeline JSON). Run: IBTRADING_EXTENDED_BACKTEST=1 ./tests
    if (qEnvironmentVariable("IBTRADING_EXTENDED_BACKTEST") == "1") {
        IBTRADING_RUN_TEST(TestBacktestExtendedLego, tc);
    }

    // Live Yahoo backtests — require internet, write HTML+TXT reports (IBTRADING_LIVE_TESTS=1)
    if (qEnvironmentVariable("IBTRADING_LIVE_TESTS") == "1") {
        IBTRADING_RUN_TEST(TestLiveBacktest, tc);
    }

    IBTRADING_RUN_TEST(TestStorageConfig, tc);
    IBTRADING_RUN_TEST(TestPersistenceFactory, tc);

    // Backend - Model Tree Repository & Mapper
    IBTRADING_RUN_TEST(TestModelTreeRepository, tc);

    // Backend - System Backend
    IBTRADING_RUN_TEST(TestSystemBackend, tc);

    // Backend - Persistence & Migration
    IBTRADING_RUN_TEST(TestPersistence, tc);

    // Backend - Runtime & Backtester Integration
    IBTRADING_RUN_TEST(TestRuntime, tc);

    // CLI Proof of Concept (no GUI dependency)
    IBTRADING_RUN_TEST(TestCliProof, tc);

    // Strategy Definition Refactoring — Phase 1-9
    IBTRADING_RUN_TEST(TestStrategyDefinition, tc);

    // Strategy Catalog (v3: families + versions)
    IBTRADING_RUN_TEST(TestStrategyCatalog, tc);

    IBTRADING_RUN_TEST(TestPipelineConfigMutations, tc);

    // Phase 10 — Adapter pure-backtest mode + config parity
    IBTRADING_RUN_TEST(TestAdapterPureBacktest, tc);

    // Semantic pipeline — ModelDataList rebalance/risk/execution + async alpha (mock ports)
    IBTRADING_RUN_TEST(TestSemanticE2E, tc);

    // Pipeline Parity — live/backtest semantic parity verification
    IBTRADING_RUN_TEST(TestPipelineParity, tc);

    IBTRADING_RUN_TEST(TestQuantMomentumBlocks, tc);

    // UI — RuntimePolicyEditor widget tests
    IBTRADING_RUN_TEST(TestRuntimePolicyEditor, tc);
    IBTRADING_RUN_TEST(TestBacktestWorkspaceCoordinator, tc);
    IBTRADING_RUN_TEST(TestTradingReadiness, tc);

    return status;
}
