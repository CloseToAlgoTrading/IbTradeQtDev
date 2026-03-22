#-------------------------------------------------
#
# Project created by QtCreator 2018-10-14T12:26:36
#
#-------------------------------------------------

QT       += core gui charts sql printsupport network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ibtrading
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.



DEFINES += QT_DEPRECATED_WARNINGS



# If there is no version tag in git this one will be used
VERSION = 0.1.1
# Adding C preprocessor #DEFINE so we can use it in C++ code
# also here we want full version on every system so using GIT_VERSION
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"


#DEFINES += GIT_CURRENT_SHORT_REV=\\"$$GIT_CURRENT_SHORT_REV)\\"

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

UI_DIR = $$PWD/GeneratedIncludes

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

SOURCES += \
    Brokers/IB/addon/AccountSummaryTags.cpp \
    Brokers/IB/addon/AvailableAlgoParams.cpp \
    Brokers/IB/addon/ContractSamples.cpp \
    Brokers/IB/addon/OrderSamples.cpp \
    Brokers/IB/addon/ScannerSubscriptionSamples.cpp \
    Brokers/IB/src/Decimal.cpp \
    Brokers/IB/src/Utils.cpp \
    CObjects/caccountsummary.cpp \
    CObjects/ccommissionreport.cpp \
    CObjects/cdeltaobject.cpp \
    CObjects/cexecutionreport.cpp \
    Common/cprocessingbase_v2.cpp \
    Brokers/BrokerConnectionFactory.cpp \
    Brokers/PaperBrokerStub.cpp \
    Common/StorageConfig.cpp \
    DB/dbhandler.cpp \
    DB/dbmanager.cpp \
    MainSystem/capplicationcontroller.cpp \
    MainSystem/UiLayoutStore.cpp \
    MainSystem/UiLayoutDefaults.cpp \
    MainSystem/UiLayoutDefaultsTables.cpp \
    MainSystem/UiLayoutDefaultsCatalog.cpp \
    MainSystem/ciconhandler.cpp \
    MainSystem/cmainmodel.cpp \
    MainSystem/csettinsmodeldata.cpp \
    MainSystem/SettingsTreeDelegate.cpp \
    MainSystem/ctreeviewcustommodel.cpp \
    MainSystem/ibtradesystemview.cpp \
    MainSystem/portfolioconfigmodel.cpp \
    MainSystem/PipelineDiagramWidget.cpp \
    Strategies/Generic/UnifiedModelData.cpp \
    Strategies/Generic/cbasemodel.cpp \
    Strategies/Generic/cbaserebalancemodel.cpp \
    Strategies/Generic/cbasicaccount.cpp \
    Strategies/Generic/cbasicalphamodel.cpp \
    Strategies/Generic/cbasicexecutionmodel.cpp \
    Strategies/Generic/cbasicportfolio.cpp \
    Strategies/Generic/cbasicriskmodel.cpp \
    Strategies/Generic/cbasicroot.cpp \
    Strategies/Generic/cbasicselectionmodel.cpp \
    Strategies/Generic/cbasicstrategy_V2.cpp \
    Strategies/Generic/cgenericmodelApi.cpp \
    Strategies/Generic/csma.cpp \
    Strategies/Generic/cstrategyfactory.cpp \
    Strategies/Generic/cpipelinestrategyadapter.cpp \
    Strategies/PairTrader/PairTraderLogging.cpp \
    Strategies/AutoDeltAlignment/src/AutoDeltaLogging.cpp \
    main.cpp \
    AboutDialog/aboutdialog.cpp \
    AboutDialog/AboutDlgPresener.cpp \
    AlphaModelGetTime/AlphaModGetTime.cpp \
    CObjects/CHistoricalData.cpp \
    CObjects/cmktdepth.cpp \
    CObjects/cmktdepthl2.cpp \
    CObjects/copenorder.cpp \
    CObjects/coptiontickcomputation.cpp \
    CObjects/corderstatus.cpp \
    CObjects/cposition.cpp \
    CObjects/crealtimebar.cpp \
    CObjects/ctickgeneric.cpp \
    CObjects/ctickprice.cpp \
    CObjects/cticksize.cpp \
    CObjects/ctickstring.cpp \
    Common/NHelper.cpp \
    CustomWidgets/ccandlestickqchart.cpp \
    CustomWidgets/clineqchart.cpp \
    DB/DBConnector.cpp \
    DBStore/DBStoreGUI.cpp \
    DBStore/DBStoreLogging.cpp \
    DBStore/DBStorePresenter.cpp \
    DBStore/DBStoreProcessing.cpp \
    IBComm/cbrokerdataprovider.cpp \
    IBComm/ProcessingRouterSink.cpp \
    IBComm/IBComClientIpml.cpp \
    IBComm/IBworker.cpp \
    Logger/MyLogger.cpp \
    Logging/UiLogCategories.cpp \
    MainSystem/cpresenter.cpp \
    MainSystem/QtUnsavedChangesPrompt.cpp \
    MainSystem/BacktestWorkspaceCoordinator.cpp \
    MainSystem/StrategyManagementCoordinator.cpp \
    ReqManager/globalreqmanager.cpp \
    ReqManager/ReqManager.cpp \
    Brokers/IB/src/ContractCondition.cpp \
    Brokers/IB/src/DefaultEWrapper.cpp \
    Brokers/IB/src/EClient.cpp \
    Brokers/IB/src/EClientSocket.cpp \
    Brokers/IB/src/EDecoder.cpp \
    Brokers/IB/src/EMessage.cpp \
    Brokers/IB/src/EMutex.cpp \
    Brokers/IB/src/EReader.cpp \
    Brokers/IB/src/EReaderOSSignal.cpp \
    Brokers/IB/src/ESocket.cpp \
    Brokers/IB/src/executioncondition.cpp \
    Brokers/IB/src/MarginCondition.cpp \
    Brokers/IB/src/OperatorCondition.cpp \
    Brokers/IB/src/OrderCondition.cpp \
    Brokers/IB/src/PercentChangeCondition.cpp \
    Brokers/IB/src/PriceCondition.cpp \
    Brokers/IB/src/SoftDollarTier.cpp \
    Brokers/IB/src/TimeCondition.cpp \
    Brokers/IB/src/VolumeCondition.cpp \
    Brokers/IB/src/EOrderDecoder.cpp \
    baseimpl.cpp \
    Common/globalsettings.cpp \
    CObjects/ctickbytickalllast.cpp \
    CObjects/chistoricalticks.cpp \
    DBStore/dbstoremodel.cpp \
    #MainSystem/cstandartitemsettings.cpp \
    MainSystem/treeitem.cpp \
    Backtest/SimulatedLedger.cpp \
    Backtest/SimulatedExecutionAdapter.cpp \
    Backtest/BacktestMetricsCollector.cpp \
    Backtest/BacktestSession.cpp \
    Backtest/BacktestWorkspaceSession.cpp \
    Backtest/BacktestRunPersistence.cpp \
    Backtest/YahooFinanceDataSource.cpp \
    Backtest/MarketSessionUtils.cpp \
    Backtest/HistoricalDataManager.cpp \
    Backtest/BacktestController.cpp \
    BacktestUI/BacktestWorkspaceDock.cpp \
    BacktestUI/BacktestStrategySelector.cpp \
    BacktestUI/BacktestRunConfigPanel.cpp \
    BacktestUI/BacktestRunHistoryPanel.cpp \
    BacktestUI/EquityChartWidget.cpp \
    BacktestUI/BacktestCandlestickWidget.cpp \
    BacktestUI/TradeLogWidget.cpp \
    MainSystem/GlobalStatusBar.cpp \
    MainSystem/EventLogPanel.cpp \
    MainSystem/ContextWorkspace.cpp \
    MainSystem/SystemTreeModel.cpp \
    MainSystem/SystemTreeDelegate.cpp \
    MainSystem/WorkspaceWidgets/WorkspaceBase.cpp \
    MainSystem/WorkspaceWidgets/WorkspaceHeader.cpp \
    MainSystem/WorkspaceWidgets/MetricsStrip.cpp \
    MainSystem/WorkspaceWidgets/StrategyWorkspace.cpp \
    MainSystem/WorkspaceWidgets/AccountWorkspace.cpp \
    MainSystem/WorkspaceWidgets/PortfolioWorkspace.cpp \
    MainSystem/WorkspaceWidgets/BlockInspectorPanel.cpp \
    MainSystem/WorkspaceWidgets/RuntimePolicyEditor.cpp \
    Pipeline/PipelineTreeUtils.cpp \
    Pipeline/PipelineLog.cpp \
    Pipeline/StrategyPipelineRunner.cpp \
    Pipeline/PipelineFactory.cpp \
    Pipeline/BlockGraphSerializer.cpp \
    Pipeline/UniverseResolver.cpp \
    Pipeline/StrategyRuntimePolicy.cpp \
    Pipeline/SignalMergePolicies.cpp \
    Pipeline/PipelineExecutionHost.cpp \
    Blocks/MomentumAlphaBlock.cpp \
    Blocks/MeanReversionAlphaBlock.cpp \
    Blocks/MovingAverageCrossoverAlphaBlock.cpp \
    Blocks/MaxPositionRiskBlock.cpp \
    Blocks/StaticListSelectionBlock.cpp \
    Blocks/LimitOrderExecutionBlock.cpp \
    Blocks/MarketOrderExecutionBlock.cpp \
    Supervision/StrategyRuntime.cpp \
    Supervision/Supervisor.cpp \
    Plugin/PluginLoader.cpp \
    MainSystem/AlertService.cpp \
    Backend/ModelTreeRepository.cpp \
    Backend/ModelTreeRepositoryPostgres.cpp \
    Backend/PersistenceFactory.cpp \
    Backend/ModelTreeMapper.cpp \
    Backend/SystemBackendImpl.cpp \
    SharedUI/StrategyTreeDelegate.cpp \
    SharedUI/AbstractPipelineTreeModel.cpp \
    SharedUI/StrategyTreePanel.cpp \
    SharedUI/WorkspacePresenterBase.cpp \
    SharedUI/StrategyWorkspacePresenter.cpp \
    SharedUI/BacktestPresenter.cpp \
    SharedUI/PipelineDiagramModel.cpp \
    SharedUI/BlockInspectorPresenter.cpp \
    SharedUI/StrategyDetailPresenter.cpp \
    BacktestUI/BacktestTreeModel.cpp \
    StrategyManagementUI/StrategyCatalogModel.cpp \
    StrategyManagementUI/CatalogTreeModel.cpp \
    StrategyManagementUI/StrategyCatalogPanel.cpp \
    StrategyManagementUI/StrategyDetailPanel.cpp \
    StrategyManagementUI/StrategyManagementPanel.cpp


HEADERS += \
    Common/Expected.h \
    Common/IClock.h \
    ThirdParty/expected.hpp \
    Pipeline/Contracts.h \
    Pipeline/Scope.h \
    Pipeline/IAlphaBlock.h \
    Pipeline/ISelectionBlock.h \
    Pipeline/IRebalanceBlock.h \
    Pipeline/IRiskBlock.h \
    Pipeline/IExecutionBlock.h \
    Pipeline/ISignalMergePolicy.h \
    Pipeline/SignalMergePolicies.h \
    Pipeline/PipelineExecutionHost.h \
    Pipeline/StrategyRuntimePolicy.h \
    Pipeline/UniverseResolver.h \
    IBComm/MarketDataRouter.h \
    Ports/IOrderExecutionPort.h \
    Ports/IPositionRepositoryPort.h \
    Adapters/IBOrderExecutionAdapter.h \
    Adapters/SqlitePositionRepository.h \
    Adapters/MockExecutionAdapter.h \
    Adapters/MockPositionRepository.h \
    Adapters/IBPositionRepositoryAdapter.h \
    IBComm/PositionRouter.h \
    IBComm/HistoricalDataRouter.h \
    IBComm/OrderRouter.h \
    IBComm/AccountRouter.h \
    IBComm/TimeRouter.h \
    IBComm/MarketDepthRouter.h \
    Testing/MockMarketDataRouter.h \
    Testing/IntegrationTestHarness.h \
    Replay/MarketDataRecorder.h \
    Replay/MarketDataReplayer.h \
    Pipeline/PipelineTreeUtils.h \
    Pipeline/BlockRegistry.h \
    Pipeline/StrategyPipelineRunner.h \
    Pipeline/BlockGraphSerializer.h \
    Blocks/MomentumAlphaBlock.h \
    Blocks/MaxPositionRiskBlock.h \
    Blocks/MarketOrderExecutionBlock.h \
    Blocks/MeanReversionAlphaBlock.h \
    Blocks/StaticListSelectionBlock.h \
    Blocks/LimitOrderExecutionBlock.h \
    Blocks/MovingAverageCrossoverAlphaBlock.h \
    Pipeline/PipelineFactory.h \
    Plugin/BlockPlugin.h \
    Plugin/PluginLoader.h \
    Adapters/AlphaModelAdapter.h \
    Adapters/RiskModelAdapter.h \
    Adapters/ExecutionModelAdapter.h \
    Supervision/BoundedQueue.h \
    Supervision/StrategyRuntime.h \
    Supervision/Supervisor.h \
    Logging/StructuredLogger.h \
    Metrics/MetricsCollector.h \
    Brokers/IB/Shared/Utils.h \
    Brokers/IB/addon/AccountSummaryTags.h \
    Brokers/IB/addon/AvailableAlgoParams.h \
    Brokers/IB/addon/ContractSamples.h \
    Brokers/IB/addon/FAMethodSamples.h \
    Brokers/IB/addon/OrderSamples.h \
    Brokers/IB/addon/ScannerSubscriptionSamples.h \
    CObjects/caccountsummary.h \
    CObjects/ccommissionreport.h \
    CObjects/cdeltaobject.h \
    CObjects/cexecutionreport.h \
    Common/cprocessingbase_v2.h \
    DB/dbdatatypes.h \
    DB/dbhandler.h \
    DB/dbmanager.h \
    DB/dbquery.h \
    MainSystem/CPortfolioConfigModel.h \
    MainSystem/PipelineItemDelegate.h \
    MainSystem/PipelineDiagramWidget.h \
    MainSystem/PortfolioModelDefines.h \
    MainSystem/TreeItemDataTypesDef.h \
    MainSystem/capplicationcontroller.h \
    MainSystem/UiLayoutStore.h \
    MainSystem/UiLayoutDefaults.h \
    MainSystem/ThemePalette.h \
    MainSystem/ciconhandler.h \
    MainSystem/cmainmodel.h \
    MainSystem/csettinsmodeldata.h \
    MainSystem/SettingsTreeDelegate.h \
    MainSystem/ctreeviewcustommodel.h \
    MainSystem/ctreeviewdatamodel.h \
    MainSystem/ibtradesystemview.h \
    Strategies/Generic/ModelType.h \
    Strategies/Generic/IMandatoryFields.h \
    Strategies/Generic/mandatoryFieldKeys.h \
    Strategies/Generic/mandatoryFieldRegistration.h \
    Strategies/Generic/UnifiedModelData.h \
    Strategies/Generic/cbasemodel.h \
    Strategies/Generic/cbaserebalancemodel.h \
    Strategies/Generic/cbasicaccount.h \
    Strategies/Generic/cbasicalphamodel.h \
    Strategies/Generic/cbasicexecutionmodel.h \
    Strategies/Generic/cbasicportfolio.h \
    Strategies/Generic/cbasicriskmodel.h \
    Strategies/Generic/cbasicroot.h \
    Strategies/Generic/cbasicselectionmodel.h \
    Strategies/Generic/cbasicstrategy_V2.h \
    Strategies/Generic/cgenericmodelApi.h \
    Strategies/Generic/csma.h \
    Strategies/Generic/cstrategyfactory.h \
    Strategies/Generic/cpipelinestrategyadapter.h \
    Backtest/BacktestConfig.h \
    Backtest/BacktestResult.h \
    Backtest/DataQuality.h \
    Backtest/FilledOrder.h \
    Backtest/LedgerSnapshot.h \
    Backtest/MarketPriceStore.h \
    Backtest/SimulatedLedger.h \
    Backtest/SimulatedExecutionAdapter.h \
    Backtest/BacktestMetricsCollector.h \
    Backtest/IHistoricalDataSource.h \
    Backtest/JsonlHistoricalDataSource.h \
    Backtest/CsvHistoricalDataSource.h \
    Backtest/YahooFinanceDataSource.h \
    Backtest/MarketSessionUtils.h \
    Backtest/BenchmarkComparison.h \
    Backtest/BacktestReportWriter.h \
    Backtest/BacktestSession.h \
    Backtest/BacktestWorkspaceSession.h \
    Backtest/BacktestRunPersistence.h \
    Backtest/BacktestDataTypes.h \
    Backtest/HistoricalDataManager.h \
    Backtest/BacktestController.h \
    BacktestUI/BacktestWorkspaceDock.h \
    BacktestUI/BacktestStrategySelector.h \
    BacktestUI/BacktestRunConfigPanel.h \
    BacktestUI/BacktestRunHistoryPanel.h \
    BacktestUI/EquityChartWidget.h \
    BacktestUI/BacktestCandlestickWidget.h \
    BacktestUI/TradeLogWidget.h \
    MainSystem/GlobalStatusBar.h \
    MainSystem/EventLogPanel.h \
    MainSystem/ContextWorkspace.h \
    MainSystem/SystemTreeModel.h \
    MainSystem/SystemTreeDelegate.h \
    MainSystem/WorkspaceWidgets/LayoutConstants.h \
    MainSystem/WorkspaceWidgets/WorkspaceBase.h \
    MainSystem/WorkspaceWidgets/WorkspaceHeader.h \
    MainSystem/WorkspaceWidgets/MetricsStrip.h \
    MainSystem/WorkspaceWidgets/StrategyWorkspace.h \
    MainSystem/WorkspaceWidgets/AccountWorkspace.h \
    MainSystem/WorkspaceWidgets/PortfolioWorkspace.h \
    MainSystem/WorkspaceWidgets/BlockInspectorPanel.h \
    MainSystem/WorkspaceWidgets/RuntimePolicyEditor.h \
    MainSystem/AlertService.h \
    Backend/ModelNodeRecord.h \
    Backend/IModelTreeRepository.h \
    Backend/ModelTreeRepository.h \
    Backend/ModelTreeRepositoryPostgres.h \
    Backend/PersistenceFactory.h \
    Backend/ModelTreeMapper.h \
    Backend/ISystemBackend.h \
    Backend/SystemBackendImpl.h \
    SharedUI/StrategyTreeDelegate.h \
    SharedUI/AbstractPipelineTreeModel.h \
    SharedUI/StrategyTreePanel.h \
    SharedUI/ViewModels.h \
    SharedUI/IWorkspaceView.h \
    SharedUI/WorkspacePresenterBase.h \
    SharedUI/StrategyWorkspacePresenter.h \
    SharedUI/BacktestPresenter.h \
    SharedUI/PipelineDiagramModel.h \
    SharedUI/BlockInspectorPresenter.h \
    SharedUI/StrategyDetailPresenter.h \
    BacktestUI/BacktestTreeModel.h \
    StrategyManagementUI/StrategyCatalogModel.h \
    StrategyManagementUI/CatalogTreeModel.h \
    StrategyManagementUI/StrategyCatalogPanel.h \
    StrategyManagementUI/StrategyDetailPanel.h \
    StrategyManagementUI/StrategyManagementPanel.h \
    Strategies/Generic/modelConstants.h \
    Strategies/StateMachine/cmodelstate.h \
    Strategies/StateMachine/cmodelstateimpl.h \
    Strategies/StateMachine/ModelStateUtils.h \
    baseimpl.h \
    AboutDialog/aboutdialog.h \
    AboutDialog/AboutDlgPresener.h \
    AlphaModelGetTime/AlphaModGetTime.h \
    CObjects/CHistoricalData.h \
    CObjects/cmktdepth.h \
    CObjects/cmktdepthl2.h \
    CObjects/copenorder.h \
    CObjects/coptiontickcomputation.h \
    CObjects/corderstatus.h \
    CObjects/cposition.h \
    CObjects/crealtimebar.h \
    CObjects/ctickgeneric.h \
    CObjects/ctickprice.h \
    CObjects/cticksize.h \
    CObjects/ctickstring.h \
    Common/GlobalDef.h \
    Common/StorageConfig.h \
    Common/NHelper.h \
    Common/Singleton.h \
    CustomWidgets/ccandlestickqchart.h \
    CustomWidgets/clineqchart.h \
    DB/DBConnector.h \
    DBStore/DBStoreGUI.h \
    DBStore/DBStorePresenter.h \
    DBStore/DBStoreProcessing.h \
    GeneratedIncludes/ui_aboutdialog.h \
    GeneratedIncludes/ui_autodeltaaligform.h \
    GeneratedIncludes/ui_dbstroreform.h \
    GeneratedIncludes/ui_ibtradesystem.h \
    GeneratedIncludes/ui_pairtrading.h \
    IBComm/cbrokerdataprovider.h \
    IBComm/ProcessingRouterSink.h \
    IBComm/IBComClientImpl.h \
    IBComm/IBrokerAPI.h \
    IBComm/IBworker.h \
    Logger/MyLogger.h \
    Logging/UiLogCategories.h \
    Pipeline/PipelineLog.h \
    Brokers/BrokerConnectionFactory.h \
    Brokers/PaperBrokerStub.h \
    MainSystem/cpresenter.h \
    MainSystem/IUnsavedChangesPrompt.h \
    MainSystem/BacktestWorkspaceCoordinator.h \
    MainSystem/StrategyManagementCoordinator.h \
    ReqManager/globalreqmanager.h \
    ReqManager/ReqManager.h \
    Brokers/IB/Shared/standardincludes.h \
    Brokers/IB/Shared/bar.h \
    Brokers/IB/Shared/CommissionReport.h \
    Brokers/IB/Shared/CommonDefs.h \
    Brokers/IB/Shared/Contract.h \
    Brokers/IB/Shared/ContractCondition.h \
    Brokers/IB/Shared/DefaultEWrapper.h \
    Brokers/IB/Shared/DepthMktDataDescription.h \
    Brokers/IB/Shared/EClient.h \
    Brokers/IB/Shared/EClientMsgSink.h \
    Brokers/IB/Shared/EClientSocket.h \
    Brokers/IB/Shared/EDecoder.h \
    Brokers/IB/Shared/EMessage.h \
    Brokers/IB/Shared/EMutex.h \
    Brokers/IB/Shared/EPosixClientSocketPlatform.h \
    Brokers/IB/Shared/EReader.h \
    Brokers/IB/Shared/EReaderOSSignal.h \
    Brokers/IB/Shared/EReaderSignal.h \
    Brokers/IB/Shared/ESocket.h \
    Brokers/IB/Shared/ETransport.h \
    Brokers/IB/Shared/EWrapper.h \
    Brokers/IB/Shared/EWrapper_prototypes.h \
    Brokers/IB/Shared/Execution.h \
    Brokers/IB/Shared/executioncondition.h \
    Brokers/IB/Shared/FamilyCode.h \
    Brokers/IB/Shared/HistogramEntry.h \
    Brokers/IB/Shared/HistoricalTick.h \
    Brokers/IB/Shared/HistoricalTickBidAsk.h \
    Brokers/IB/Shared/HistoricalTickLast.h \
    Brokers/IB/Shared/IExternalizable.h \
    Brokers/IB/Shared/MarginCondition.h \
    Brokers/IB/Shared/NewsProvider.h \
    Brokers/IB/Shared/OperatorCondition.h \
    Brokers/IB/Shared/Order.h \
    Brokers/IB/Shared/OrderCondition.h \
    Brokers/IB/Shared/OrderState.h \
    Brokers/IB/Shared/PercentChangeCondition.h \
    Brokers/IB/Shared/PriceCondition.h \
    Brokers/IB/Shared/PriceIncrement.h \
    Brokers/IB/Shared/ScannerSubscription.h \
    Brokers/IB/Shared/SoftDollarTier.h \
    #Brokers/IB/Shared/StdAfx.h \
    Brokers/IB/Shared/TagValue.h \
    Brokers/IB/Shared/TickAttrib.h \
    Brokers/IB/Shared/TickAttribBidAsk.h \
    Brokers/IB/Shared/TickAttribLast.h \
    Brokers/IB/Shared/TimeCondition.h \
    Brokers/IB/Shared/TwsSocketClientErrors.h \
    Brokers/IB/Shared/VolumeCondition.h \
    Brokers/IB/Shared/Decimal.h \
    Brokers/IB/Shared/EClientException.h \
    Brokers/IB/Shared/EOrderDecoder.h \
    Brokers/IB/Shared/HistoricalSession.h \
    Brokers/IB/Shared/platformspecific.h \
    Brokers/IB/Shared/resource.h \
    Brokers/IB/Shared/WshEventData.h \
    Common/globalsettings.h \
    CObjects/ctickbytickalllast.h \
    CObjects/chistoricalticks.h \
    DBStore/dbstoremodel.h \
    #MainSystem/cstandartitemsettings.h \
    MainSystem/treeitem.h


FORMS += \
    AboutDialog/aboutdialog.ui \
    MainSystem/ibtradesystemview.ui \
    DBStore/dbstroreform.ui

INCLUDEPATH += \
    $$PWD/Backtest \
    $$PWD/Pipeline \
    $$PWD/Ports \
    $$PWD/ThirdParty \
    $$PWD/Adapters \
    $$PWD/Testing \
    $$PWD/Replay \
    $$PWD/Blocks \
    $$PWD/Plugin \
    $$PWD/Supervision \
    $$PWD/Logging \
    $$PWD/Metrics \
    $$PWD/Brokers/IB/Shared \
    $$PWD/Brokers/IB/addon \
    $$PWD/ReqManager \
    $$PWD/QCustomPlot \
    $$PWD/PairTrader \
    $$PWD/MainSystem \
    $$PWD/MainSystem/WorkspaceWidgets \
    $$PWD/Logger \
    $$PWD/IBComm \
    $$PWD/DB \
    $$PWD/CustomWidgets \
    $$PWD/Common \
    $$PWD/CObjects \
    $$PWD/AlphaModelGetTime \
    $$PWD/AboutDialog \
    $$PWD/Strategies/Generic \
    $$PWD/Backend \
    $$PWD/SharedUI \
    $$PWD/StrategyManagementUI \
    $$PWD/Strategies/StateMachine \
    $$PWD/GeneratedIncludes \
    $$PWD/DBStore



#win32:INCLUDEPATH += "c:/Program Files/PostgreSQL/11/include/"
#unix:INCLUDEPATH += /usr/include/postgresql

#win32:LIBS += "c:/Program Files/PostgreSQL/11/libpq.lib"
#unix:LIBS += -L/usr/lib -lpq

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

*msvc* { # visual studio spec filter
      QMAKE_CXXFLAGS += /MP /MDd
  }

#QMAKE_CXXFLAGS += -pthread -fPIC

SUBDIRS += \
    ibtrading.pro

RESOURCES += \
    ibtradesystem.qrc

DISTFILES += \
    doc/mainClass.wsd


unix {

    INCLUDEPATH += $$PWD/Libs
    INCLUDEPATH += /usr/include/postgresql

    LIBS += -L$$PWD/Libs/ -lbid

    DEPENDPATH += $$PWD/Libs
    PRE_TARGETDEPS += $$PWD/Libs/libbid.a
}

win32 {
    LIBS += -L$$PWD/Libs/win/ -llibbid

    INCLUDEPATH += $$PWD/Libs/win
    INCLUDEPATH += "c:/Program Files/PostgreSQL/11/include/"

    DEPENDPATH += $$PWD/Libs/win
    PRE_TARGETDEPS += $$PWD/Libs/win/libbid.lib
}
#unix: LIBS += -L$$PWD/Libs/ -lbid
#unix:INCLUDEPATH += $$PWD/Libs
#unix:DEPENDPATH += $$PWD/Libs
#unix: PRE_TARGETDEPS += $$PWD/Libs/libbid.a


#win32: LIBS += -L$$PWD/Libs/win/ -llibbid
#win32: INCLUDEPATH += $$PWD/Libs/win
#win32: DEPENDPATH += $$PWD/Libs/win
#win32:!win32-g++: PRE_TARGETDEPS += $$PWD/Libs/win/libbid.lib
#else:win32-g++: PRE_TARGETDEPS += $$PWD/Libs/win/liblibbid.a
