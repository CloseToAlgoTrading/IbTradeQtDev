QT += core testlib sql
QT -= gui

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
    $$PWD/../IBComm \
    $$PWD/../CObjects \
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
    ../Common/NHelper.cpp

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
    ../CObjects/caccountsummary.h \
    ../CObjects/cposition.h \
    ../CObjects/CHistoricalData.h \
    ../CObjects/crealtimebar.h
