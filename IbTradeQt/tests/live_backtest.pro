QT += core testlib sql network
QT -= gui

TARGET   = ibtrading_live_backtest
TEMPLATE = app
CONFIG  += c++17 console
CONFIG  -= app_bundle

CONFIG(debug, debug|release) {
    DESTDIR     = $$PWD/debug
    OBJECTS_DIR = $$PWD/debug/obj_live
    MOC_DIR     = $$PWD/debug/moc_live
    RCC_DIR     = $$PWD/debug/rcc_live
} else {
    DESTDIR     = $$PWD/release
    OBJECTS_DIR = $$PWD/release/obj_live
    MOC_DIR     = $$PWD/release/moc_live
    RCC_DIR     = $$PWD/release/rcc_live
}

DEFINES += SRCDIR=\\\"$$PWD\\\"

INCLUDEPATH += \
    $$PWD/.. \
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
    $$PWD/../CObjects \
    $$PWD/../Brokers/IB/Shared \
    $$PWD/../ReqManager \
    $$PWD/debug/moc_live

unix {
    INCLUDEPATH += $$PWD/../Libs
    LIBS += -L$$PWD/../Libs/ -lbid
    DEPENDPATH += $$PWD/../Libs
    PRE_TARGETDEPS += $$PWD/../Libs/libbid.a
}

SOURCES += \
    live_backtest_main.cpp \
    ../Brokers/IB/src/Decimal.cpp \
    ../CObjects/caccountsummary.cpp \
    ../CObjects/cposition.cpp \
    ../CObjects/CHistoricalData.cpp \
    ../CObjects/crealtimebar.cpp \
    ../CObjects/cexecutionreport.cpp \
    ../CObjects/ccommissionreport.cpp \
    ../Common/NHelper.cpp \
    ../Backtest/SimulatedLedger.cpp \
    ../Backtest/SimulatedExecutionAdapter.cpp \
    ../Backtest/BacktestMetricsCollector.cpp \
    ../Backtest/BacktestSession.cpp \
    ../Backtest/YahooFinanceDataSource.cpp

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
    ../Common/IClock.h \
    ../ThirdParty/expected.hpp \
    ../Replay/MarketDataReplayer.h \
    ../Pipeline/BlockRegistry.h \
    ../Pipeline/StrategyPipelineRunner.h \
    ../Pipeline/BlockGraphSerializer.h \
    ../Pipeline/PipelineFactory.h \
    ../Blocks/MomentumAlphaBlock.h \
    ../Blocks/MaxPositionRiskBlock.h \
    ../Blocks/MarketOrderExecutionBlock.h \
    ../Blocks/MeanReversionAlphaBlock.h \
    ../Blocks/MovingAverageCrossoverAlphaBlock.h \
    ../Blocks/StaticListSelectionBlock.h \
    ../Blocks/LimitOrderExecutionBlock.h \
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
    ../Adapters/IBOrderExecutionAdapter.h \
    ../Adapters/IBPositionRepositoryAdapter.h \
    ../Adapters/SqlitePositionRepository.h \
    backtest/tst_live_backtest.h
