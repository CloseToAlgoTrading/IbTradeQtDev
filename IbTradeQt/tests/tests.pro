QT += core testlib sql network gui widgets

TARGET = ibtrading_tests
TEMPLATE = app
CONFIG += c++17 testcase console
CONFIG -= app_bundle

CONFIG(debug, debug|release) {
    DESTDIR     = $$PWD/debug
    OBJECTS_DIR = $$PWD/debug/obj
    MOC_DIR     = $$PWD/debug/moc
    RCC_DIR     = $$PWD/debug/rcc
} else {
    DESTDIR     = $$PWD/release
    OBJECTS_DIR = $$PWD/release/obj
    MOC_DIR     = $$PWD/release/moc
    RCC_DIR     = $$PWD/release/rcc
}

DEFINES += SRCDIR=\\\"$$PWD\\\"

INCLUDEPATH += \
    $$PWD/.. \
    $$PWD/../Backend \
    $$PWD/../Backtest \
    $$PWD/../Pipeline \
    $$PWD/../Ports \
    $$PWD/../Common \
    $$PWD/../ThirdParty \
    $$PWD/../IBComm \
    $$PWD/../Adapters \
    $$PWD/../Testing \
    $$PWD/../Replay \
    $$PWD/../Blocks \
    $$PWD/../Plugin \
    $$PWD/../Supervision \
    $$PWD/../Logging \
    $$PWD/../Metrics \
    $$PWD/../Strategies/Generic \
    $$PWD/../Strategies/StateMachine \
    $$PWD/../DB \
    $$PWD/../IBComm \
    $$PWD/../CObjects \
    $$PWD/../StrategyManagementUI \
    $$PWD/../Brokers/IB/Shared \
    $$PWD/../ReqManager \
    $$MOC_DIR

unix {
    INCLUDEPATH += $$PWD/../Libs
    LIBS += -L$$PWD/../Libs/ -lbid
    DEPENDPATH += $$PWD/../Libs
    PRE_TARGETDEPS += $$PWD/../Libs/libbid.a
}

win32 {
    LIBS += -L$$PWD/../Libs/win/ -llibbid
    INCLUDEPATH += $$PWD/../Libs/win
    DEPENDPATH += $$PWD/../Libs/win
    PRE_TARGETDEPS += $$PWD/../Libs/win/libbid.lib
}

SOURCES += \
    main.cpp \
    ../Brokers/IB/src/Decimal.cpp \
    ../CObjects/caccountsummary.cpp \
    ../CObjects/cposition.cpp \
    ../CObjects/CHistoricalData.cpp \
    ../CObjects/crealtimebar.cpp \
    ../CObjects/cexecutionreport.cpp \
    ../CObjects/ccommissionreport.cpp \
    ../Common/NHelper.cpp \
    ../Common/cprocessingbase_v2.cpp \
    ../IBComm/cbrokerdataprovider.cpp \
    ../CObjects/cdeltaobject.cpp \
    ../CObjects/copenorder.cpp \
    ../CObjects/corderstatus.cpp \
    ../CObjects/coptiontickcomputation.cpp \
    ../CObjects/ctickprice.cpp \
    ../CObjects/cticksize.cpp \
    ../CObjects/ctickgeneric.cpp \
    ../CObjects/ctickstring.cpp \
    ../CObjects/cmktdepth.cpp \
    ../CObjects/cmktdepthl2.cpp \
    ../CObjects/ctickbytickalllast.cpp \
    ../CObjects/chistoricalticks.cpp \
    ../ReqManager/globalreqmanager.cpp \
    ../ReqManager/ReqManager.cpp \
    ../Brokers/IB/src/SoftDollarTier.cpp \
    ../Common/globalsettings.cpp \
    ../Strategies/Generic/cbasemodel.cpp \
    ../Strategies/Generic/cbasicroot.cpp \
    ../Strategies/Generic/cbasicaccount.cpp \
    ../Strategies/Generic/cbasicportfolio.cpp \
    ../Strategies/Generic/cbasicstrategy_V2.cpp \
    ../Strategies/Generic/cbasicselectionmodel.cpp \
    ../Strategies/Generic/cbasicalphamodel.cpp \
    ../Strategies/Generic/cbaserebalancemodel.cpp \
    ../Strategies/Generic/cbasicriskmodel.cpp \
    ../Strategies/Generic/cbasicexecutionmodel.cpp \
    ../Strategies/Generic/cgenericmodelApi.cpp \
    ../Strategies/Generic/cstrategyfactory.cpp \
    ../Strategies/Generic/UnifiedModelData.cpp \
    ../DB/dbhandler.cpp \
    ../DB/dbmanager.cpp \
    ../DB/DBConnector.cpp \
    ../Backend/ModelTreeRepository.cpp \
    ../Backend/ModelTreeMapper.cpp \
    ../Backend/SystemBackendImpl.cpp \
    ../Backtest/SimulatedLedger.cpp \
    ../Backtest/SimulatedExecutionAdapter.cpp \
    ../Backtest/BacktestMetricsCollector.cpp \
    ../Backtest/BacktestSession.cpp \
    ../Backtest/YahooFinanceDataSource.cpp \
    ../StrategyManagementUI/StrategyCatalogModel.cpp \
    ../StrategyManagementUI/StrategyCatalogPanel.cpp \
    ../StrategyManagementUI/StrategyDetailPanel.cpp \
    ../StrategyManagementUI/StrategyManagementPanel.cpp

HEADERS += \
    ../Pipeline/Contracts.h \
    ../Pipeline/Scope.h \
    ../Pipeline/IAlphaBlock.h \
    ../Pipeline/ISelectionBlock.h \
    ../Pipeline/IRebalanceBlock.h \
    ../Pipeline/IRiskBlock.h \
    ../Pipeline/IExecutionBlock.h \
    ../Pipeline/ISignalMergePolicy.h \
    ../IBComm/MarketDataRouter.h \
    ../Ports/IOrderExecutionPort.h \
    ../Ports/IPositionRepositoryPort.h \
    ../Common/Expected.h \
    ../ThirdParty/expected.hpp \
    ../Adapters/MockExecutionAdapter.h \
    ../Adapters/MockPositionRepository.h \
    ../Testing/MockMarketDataRouter.h \
    ../Testing/IntegrationTestHarness.h \
    ../Replay/MarketDataRecorder.h \
    ../Replay/MarketDataReplayer.h \
    ../Pipeline/BlockRegistry.h \
    ../Pipeline/StrategyPipelineRunner.h \
    ../Pipeline/BlockGraphSerializer.h \
    ../Blocks/MomentumAlphaBlock.h \
    ../Blocks/MaxPositionRiskBlock.h \
    ../Blocks/MarketOrderExecutionBlock.h \
    ../Blocks/MeanReversionAlphaBlock.h \
    ../Pipeline/PipelineFactory.h \
    ../Plugin/BlockPlugin.h \
    ../Plugin/PluginLoader.h \
    ../Supervision/BoundedQueue.h \
    ../Supervision/StrategyRuntime.h \
    ../Supervision/Supervisor.h \
    ../Logging/StructuredLogger.h \
    ../Metrics/MetricsCollector.h \
    ../Strategies/Generic/ModelType.h \
    phase1/tst_contracts.h \
    phase1/tst_merge_policies.h \
    phase1/tst_market_data_router.h \
    phase1/tst_block_interfaces.h \
    phase1/tst_expected.h \
    phase1/tst_scope.h \
    phase2/tst_replay.h \
    phase2/tst_adapters.h \
    phase2/tst_integration.h \
    phase3/tst_block_registry.h \
    phase3/tst_pipeline_runner.h \
    phase4/tst_supervision.h \
    phase5/tst_observability.h \
    phase6/tst_benchmark.h \
    integration/tst_default_pipelines.h \
    integration/tst_pipeline_strategy_adapter.h \
    integration/tst_live_execution_wiring.h \
    integration/tst_typed_routers.h \
    integration/tst_phase_b_migration.h \
    integration/tst_phase_d_dispatcher_removal.h \
    integration/tst_phase_e_remaining_routers.h \
    ../Adapters/IBOrderExecutionAdapter.h \
    ../Adapters/IBPositionRepositoryAdapter.h \
    ../Adapters/SqlitePositionRepository.h \
    ../Blocks/StaticListSelectionBlock.h \
    ../Blocks/LimitOrderExecutionBlock.h \
    ../Blocks/MovingAverageCrossoverAlphaBlock.h \
    ../IBComm/IBrokerAPI.h \
    ../IBComm/PositionRouter.h \
    ../IBComm/HistoricalDataRouter.h \
    ../IBComm/OrderRouter.h \
    ../IBComm/AccountRouter.h \
    ../IBComm/TimeRouter.h \
    ../IBComm/MarketDepthRouter.h \
    ../CObjects/caccountsummary.h \
    ../CObjects/cposition.h \
    ../CObjects/CHistoricalData.h \
    ../CObjects/crealtimebar.h \
    ../CObjects/cexecutionreport.h \
    ../CObjects/ccommissionreport.h \
    ../Common/IClock.h \
    ../Backtest/BacktestConfig.h \
    ../Backtest/BacktestResult.h \
    ../Backtest/DataQuality.h \
    ../Backtest/FilledOrder.h \
    ../Backtest/LedgerSnapshot.h \
    ../Backtest/MarketPriceStore.h \
    ../Backtest/SimulatedLedger.h \
    ../Backtest/SimulatedExecutionAdapter.h \
    ../Backtest/BacktestMetricsCollector.h \
    ../Backtest/IHistoricalDataSource.h \
    ../Backtest/JsonlHistoricalDataSource.h \
    ../Backtest/CsvHistoricalDataSource.h \
    ../Backtest/YahooFinanceDataSource.h \
    ../Backtest/BenchmarkComparison.h \
    ../Backtest/BacktestReportWriter.h \
    ../Backtest/BacktestSession.h \
    backtest/tst_backtest.h \
    backtest/tst_yahoo_backtest.h \
    backtest/tst_live_backtest.h \
    backend/tst_model_tree_repository.h \
    backend/tst_system_backend.h \
    backend/tst_persistence.h \
    backend/tst_runtime.h \
    backend/tst_cli_proof.h \
    backend/tst_strategy_definition.h \
    backend/tst_strategy_catalog.h \
    integration/tst_adapter_pure_backtest.h \
    ../Backend/ISystemBackend.h \
    ../Backend/SystemBackendImpl.h \
    ../Backend/ModelNodeRecord.h \
    ../Backend/ModelTreeRepository.h \
    ../Backend/ModelTreeMapper.h \
    ../Strategies/Generic/cbasemodel.h \
    ../Strategies/Generic/cbasicroot.h \
    ../Strategies/Generic/cbasicaccount.h \
    ../Strategies/Generic/cbasicportfolio.h \
    ../Strategies/Generic/cbasicstrategy_V2.h \
    ../Strategies/Generic/cbasicselectionmodel.h \
    ../Strategies/Generic/cbasicalphamodel.h \
    ../Strategies/Generic/cbaserebalancemodel.h \
    ../Strategies/Generic/cbasicriskmodel.h \
    ../Strategies/Generic/cbasicexecutionmodel.h \
    ../Strategies/Generic/cstrategyfactory.h \
    ../Strategies/Generic/cpipelinestrategyadapter.h \
    ../Strategies/Generic/UnifiedModelData.h \
    ../Strategies/Generic/IMandatoryFields.h \
    ../Strategies/Generic/mandatoryFieldKeys.h \
    ../Strategies/Generic/mandatoryFieldRegistration.h \
    ../Strategies/Generic/modelConstants.h \
    ../Strategies/StateMachine/cmodelstate.h \
    ../Strategies/StateMachine/cmodelstateimpl.h \
    ../Strategies/StateMachine/ModelStateUtils.h \
    ../Common/cprocessingbase_v2.h \
    ../DB/dbhandler.h \
    ../DB/dbmanager.h \
    ../DB/dbdatatypes.h \
    ../DB/DBConnector.h \
    ../CObjects/cdeltaobject.h \
    ../CObjects/copenorder.h \
    ../CObjects/corderstatus.h \
    ../CObjects/coptiontickcomputation.h \
    ../CObjects/ctickprice.h \
    ../CObjects/cticksize.h \
    ../CObjects/ctickgeneric.h \
    ../CObjects/ctickstring.h \
    ../CObjects/cmktdepth.h \
    ../CObjects/cmktdepthl2.h \
    ../CObjects/ctickbytickalllast.h \
    ../CObjects/chistoricalticks.h \
    ../IBComm/cbrokerdataprovider.h \
    ../Common/globalsettings.h \
    ../ReqManager/globalreqmanager.h \
    ../ReqManager/ReqManager.h \
    ../StrategyManagementUI/StrategyCatalogModel.h \
    ../StrategyManagementUI/StrategyCatalogPanel.h \
    ../StrategyManagementUI/StrategyDetailPanel.h \
    ../StrategyManagementUI/StrategyManagementPanel.h
