QT += core sql gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = ibtrade-cli
DESTDIR = $$PWD/../release

INCLUDEPATH += \
    $$PWD/.. \
    $$PWD/../Backend \
    $$PWD/../Strategies/Generic \
    $$PWD/../Strategies/StateMachine \
    $$PWD/../Common \
    $$PWD/../DB \
    $$PWD/../IBComm \
    $$PWD/../CObjects \
    $$PWD/../Brokers/IB/Shared \
    $$PWD/../ReqManager \
    $$PWD/../Pipeline \
    $$PWD/../Supervision \
    $$PWD/../Ports \
    $$PWD/../Adapters \
    $$PWD/../Blocks \
    $$PWD/../ThirdParty \
    $$PWD/../Libs

SOURCES += \
    main.cpp \
    ../Backend/ModelTreeRepository.cpp \
    ../Backend/ModelTreeMapper.cpp \
    ../Backend/SystemBackendImpl.cpp \
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
    ../Common/cprocessingbase_v2.cpp \
    ../Common/globalsettings.cpp \
    ../IBComm/cbrokerdataprovider.cpp \
    ../Common/NHelper.cpp \
    ../Common/StorageConfig.cpp \
    ../DB/dbhandler.cpp \
    ../DB/dbmanager.cpp \
    ../DB/DBConnector.cpp \
    ../ReqManager/globalreqmanager.cpp \
    ../ReqManager/ReqManager.cpp \
    ../Brokers/IB/src/SoftDollarTier.cpp \
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
    ../CObjects/caccountsummary.cpp \
    ../CObjects/cposition.cpp \
    ../CObjects/CHistoricalData.cpp \
    ../CObjects/crealtimebar.cpp \
    ../CObjects/cexecutionreport.cpp \
    ../CObjects/ccommissionreport.cpp \
    ../Brokers/IB/src/Decimal.cpp \
    ../Pipeline/PipelineLog.cpp

HEADERS += \
    ../Backend/ISystemBackend.h \
    ../Backend/SystemBackendImpl.h \
    ../Backend/ModelNodeRecord.h \
    ../Backend/ModelTreeRepository.h \
    ../Backend/ModelTreeMapper.h \
    ../Strategies/Generic/cbasemodel.h \
    ../Strategies/Generic/cbasicroot.h \
    ../Strategies/Generic/cbasicaccount.h \
    ../Strategies/Generic/cbasicportfolio.h \
    ../Strategies/Generic/cbasicselectionmodel.h \
    ../Strategies/Generic/cbasicalphamodel.h \
    ../Strategies/Generic/cbaserebalancemodel.h \
    ../Strategies/Generic/cbasicriskmodel.h \
    ../Strategies/Generic/cbasicexecutionmodel.h \
    ../Strategies/Generic/cgenericmodelApi.h \
    ../Strategies/Generic/cstrategyfactory.h \
    ../Strategies/Generic/cpipelinestrategyadapter.h \
    ../Common/cprocessingbase_v2.h \
    ../IBComm/cbrokerdataprovider.h \
    ../IBComm/IBrokerAPI.h \
    ../DB/dbhandler.h \
    ../DB/dbmanager.h \
    ../DB/DBConnector.h \
    ../CObjects/cdeltaobject.h \
    ../Supervision/StrategyRuntime.h \
    ../Supervision/Supervisor.h \
    ../Pipeline/StrategyPipelineRunner.h \
    ../Pipeline/IAlphaBlock.h \
    ../Pipeline/ISelectionBlock.h \
    ../Pipeline/IRebalanceBlock.h \
    ../Pipeline/IRiskBlock.h \
    ../Pipeline/IExecutionBlock.h \
    ../Pipeline/ISignalMergePolicy.h \
    ../Pipeline/Contracts.h \
    ../Pipeline/BlockRegistry.h \
    ../Pipeline/PipelineLog.h \
    ../IBComm/MarketDataRouter.h \
    ../IBComm/PositionRouter.h \
    ../IBComm/HistoricalDataRouter.h \
    ../IBComm/OrderRouter.h \
    ../IBComm/AccountRouter.h \
    ../IBComm/TimeRouter.h \
    ../IBComm/MarketDepthRouter.h \
    ../Adapters/IBPositionRepositoryAdapter.h \
    ../Blocks/MomentumAlphaBlock.h \
    ../Blocks/MeanReversionAlphaBlock.h \
    ../Blocks/MovingAverageCrossoverAlphaBlock.h \
    ../Blocks/MaxPositionRiskBlock.h \
    ../Blocks/MarketOrderExecutionBlock.h \
    ../Blocks/LimitOrderExecutionBlock.h \
    ../Blocks/StaticListSelectionBlock.h

LIBS += -L$$PWD/../Libs/ -lbid
