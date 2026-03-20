#include <QApplication>
#include <QtTest>

#include "phase1/tst_contracts.h"
#include "phase1/tst_merge_policies.h"
#include "phase1/tst_market_data_router.h"
#include "phase1/tst_block_interfaces.h"
#include "phase1/tst_expected.h"
#include "phase1/tst_scope.h"
#include "phase2/tst_replay.h"
#include "phase2/tst_adapters.h"
#include "phase2/tst_integration.h"
#include "phase3/tst_block_registry.h"
#include "phase3/tst_pipeline_runner.h"
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
#include "backtest/tst_live_backtest.h"
#include "backend/tst_storage_config.h"
#include "backend/tst_persistence_factory.h"
#include "backend/tst_model_tree_repository.h"
#include "backend/tst_system_backend.h"
#include "backend/tst_persistence.h"
#include "backend/tst_runtime.h"
#include "backend/tst_cli_proof.h"
#include "backend/tst_strategy_definition.h"
#include "backend/tst_strategy_catalog.h"
#include "integration/tst_adapter_pure_backtest.h"
#include "parity/tst_pipeline_parity.h"
#include "ui/tst_runtime_policy_editor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    int status = 0;

    // Phase 1
    { TestContracts tc;        status |= QTest::qExec(&tc, argc, argv); }
    { TestMergePolicies tc;    status |= QTest::qExec(&tc, argc, argv); }
    { TestMarketDataRouter tc; status |= QTest::qExec(&tc, argc, argv); }
    { TestBlockInterfaces tc;  status |= QTest::qExec(&tc, argc, argv); }
    { TestExpected tc;         status |= QTest::qExec(&tc, argc, argv); }
    { TestScope tc;            status |= QTest::qExec(&tc, argc, argv); }

    // Phase 2
    { TestReplay tc;           status |= QTest::qExec(&tc, argc, argv); }
    { TestAdapters tc;         status |= QTest::qExec(&tc, argc, argv); }
    { TestIntegration tc;      status |= QTest::qExec(&tc, argc, argv); }

    // Phase 3
    { TestBlockRegistry tc;    status |= QTest::qExec(&tc, argc, argv); }
    { TestPipelineRunner tc;   status |= QTest::qExec(&tc, argc, argv); }

    // Phase 4
    { TestSupervision tc;      status |= QTest::qExec(&tc, argc, argv); }

    // Phase 5
    { TestObservability tc;    status |= QTest::qExec(&tc, argc, argv); }

    // Phase 6 - Benchmarks
    { TestBenchmark tc;        status |= QTest::qExec(&tc, argc, argv); }

    // Integration - Default Pipelines
    { TestDefaultPipelines tc; status |= QTest::qExec(&tc, argc, argv); }

    // Integration - Pipeline Strategy Adapter
    { TestPipelineStrategyAdapter tc; status |= QTest::qExec(&tc, argc, argv); }

    // Integration - Live Execution Wiring
    { TestLiveExecutionWiring tc; status |= QTest::qExec(&tc, argc, argv); }

    // Integration - Typed Routers
    { TestTypedRouters tc; status |= QTest::qExec(&tc, argc, argv); }

    // Phase B - Legacy subscriber migration
    { TestPhaseBMigration tc; status |= QTest::qExec(&tc, argc, argv); }

    // Phase D - CDispatcher removal verification
    { TestPhaseD_DispatcherRemoval tc; status |= QTest::qExec(&tc, argc, argv); }

    // Phase E - Remaining router signals
    { TestPhaseE_RemainingRouters tc; status |= QTest::qExec(&tc, argc, argv); }

    // Backtester — Phase 1 foundations
    { TestIClock tc;                         status |= QTest::qExec(&tc, argc, argv); }
    { TestMarketPriceStore tc;               status |= QTest::qExec(&tc, argc, argv); }
    { TestSimulatedLedger tc;                status |= QTest::qExec(&tc, argc, argv); }
    { TestSimulatedExecutionAdapter tc;      status |= QTest::qExec(&tc, argc, argv); }
    { TestBacktestMetricsCollector tc;       status |= QTest::qExec(&tc, argc, argv); }
    { TestMarketDataReplayerExtensions tc;   status |= QTest::qExec(&tc, argc, argv); }
    { TestCsvHistoricalDataSource tc;        status |= QTest::qExec(&tc, argc, argv); }
    { TestJsonlHistoricalDataSource tc;      status |= QTest::qExec(&tc, argc, argv); }
    { TestFullBacktestSession tc;            status |= QTest::qExec(&tc, argc, argv); }

    // Backtester — Yahoo Finance data source + benchmark comparison
    { TestYahooFinanceDataSource tc;         status |= QTest::qExec(&tc, argc, argv); }
    { TestBenchmarkComparison tc;            status |= QTest::qExec(&tc, argc, argv); }
    { TestMACrossoverBacktest tc;            status |= QTest::qExec(&tc, argc, argv); }

    // Live end-to-end backtests — require internet, write HTML+TXT reports
    { TestLiveBacktest tc;                   status |= QTest::qExec(&tc, argc, argv); }

    { TestStorageConfig tc;                  status |= QTest::qExec(&tc, argc, argv); }
    { TestPersistenceFactory tc;             status |= QTest::qExec(&tc, argc, argv); }

    // Backend - Model Tree Repository & Mapper
    { TestModelTreeRepository tc;            status |= QTest::qExec(&tc, argc, argv); }

    // Backend - System Backend
    { TestSystemBackend tc;                  status |= QTest::qExec(&tc, argc, argv); }

    // Backend - Persistence & Migration
    { TestPersistence tc;                    status |= QTest::qExec(&tc, argc, argv); }

    // Backend - Runtime & Backtester Integration
    { TestRuntime tc;                        status |= QTest::qExec(&tc, argc, argv); }

    // CLI Proof of Concept (no GUI dependency)
    { TestCliProof tc;                       status |= QTest::qExec(&tc, argc, argv); }

    // Strategy Definition Refactoring — Phase 1-9
    { TestStrategyDefinition tc;             status |= QTest::qExec(&tc, argc, argv); }

    // Strategy Catalog (v3: families + versions)
    { TestStrategyCatalog tc;                status |= QTest::qExec(&tc, argc, argv); }

    // Phase 10 — Adapter pure-backtest mode + config parity
    { TestAdapterPureBacktest tc;            status |= QTest::qExec(&tc, argc, argv); }

    // Pipeline Parity — live/backtest semantic parity verification
    { TestPipelineParity tc;                 status |= QTest::qExec(&tc, argc, argv); }

    // UI — RuntimePolicyEditor widget tests
    { TestRuntimePolicyEditor tc;            status |= QTest::qExec(&tc, argc, argv); }

    return status;
}
