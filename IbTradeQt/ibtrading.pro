#-------------------------------------------------
#
# Project created by QtCreator 2018-10-14T12:26:36
#
#-------------------------------------------------

QT       += core gui charts sql printsupport network

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

SOURCES += \
    CObjects/cdeltaobject.cpp \
    Common/cprocessingbase_v2.cpp \
    MainSystem/capplicationcontroller.cpp \
    MainSystem/ciconhandler.cpp \
    MainSystem/cmainmodel.cpp \
    MainSystem/csettinsmodeldata.cpp \
    MainSystem/ctreeviewcustommodel.cpp \
    MainSystem/ibtradesystemview.cpp \
    MainSystem/portfolioconfigmodel.cpp \
    Strategies/AutoDeltAlignment/src/CModelInputData.cpp \
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
    Strategies/Generic/cbasicstrategy.cpp \
    Strategies/Generic/cbasicstrategy_V2.cpp \
    Strategies/Generic/cgenericmodelApi.cpp \
    Strategies/Generic/cgenericmodelApi_V2.cpp \
    Strategies/Generic/cmomentum.cpp \
    Strategies/Generic/cmovingaveragecrossover.cpp \
    Strategies/Generic/csma.cpp \
    Strategies/Generic/cstrategyfactory.cpp \
    Strategies/Generic/cteststrategy.cpp \
    Strategies/PairTrader/PairTradingGui.cpp \
    Strategies/PairTrader/PairTradingPresenter.cpp \
    Strategies/PairTrader/pairtraderpm.cpp \
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
    Common/cprocessingbase.cpp \
    Common/NHelper.cpp \
    CustomWidgets/ccandlestickqchart.cpp \
    CustomWidgets/clineqchart.cpp \
    DB/DBConnector.cpp \
    DBStore/DBStoreGUI.cpp \
    DBStore/DBStorePresenter.cpp \
    DBStore/DBStoreProcessing.cpp \
    IBComm/cbrokerdataprovider.cpp \
    IBComm/Dispatcher.cpp \
    IBComm/IBComClientIpml.cpp \
    IBComm/IBworker.cpp \
    Logger/MyLogger.cpp \
    MainSystem/cpresenter.cpp \
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
    Strategies/AutoDeltAlignment/src/AutoDeltAlignmentGUI.cpp \
    Strategies/AutoDeltAlignment/src/AutoDeltAlignmentPresenter.cpp \
    Strategies/AutoDeltAlignment/src/AutoDeltAlignmentProcessing.cpp \
    baseimpl.cpp \
    Common/globalsettings.cpp \
    CObjects/ctickbytickalllast.cpp \
    CObjects/chistoricalticks.cpp \
    DBStore/dbstoremodel.cpp \
    Common/contractsdefs.cpp \
    Common/OrderSamples.cpp \
    #MainSystem/cstandartitemsettings.cpp \
    MainSystem/treeitem.cpp


HEADERS += \
    CObjects/cdeltaobject.h \
    Common/cprocessingbase_v2.h \
    MainSystem/CPortfolioConfigModel.h \
    MainSystem/PortfolioModelDefines.h \
    MainSystem/TreeItemDataTypesDef.h \
    MainSystem/capplicationcontroller.h \
    MainSystem/ciconhandler.h \
    MainSystem/cmainmodel.h \
    MainSystem/csettinsmodeldata.h \
    MainSystem/ctreeviewcustommodel.h \
    MainSystem/ctreeviewdatamodel.h \
    MainSystem/ibtradesystemview.h \
    Strategies/AutoDeltAlignment/header/CModelInputData.h \
    Strategies/AutoDeltAlignment/header/autodeltatypes.h \
    Strategies/Generic/ModelType.h \
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
    Strategies/Generic/cbasicstrategy.h \
    Strategies/Generic/cbasicstrategy_V2.h \
    Strategies/Generic/cgenericmodelApi.h \
    Strategies/Generic/cgenericmodelApi_V2.h \
    Strategies/Generic/cmomentum.h \
    Strategies/Generic/cmovingaveragecrossover.h \
    Strategies/Generic/csma.h \
    Strategies/Generic/cstrategyfactory.h \
    Strategies/Generic/cteststrategy.h \
    Strategies/PairTrader/PairTradingGui.h \
    Strategies/PairTrader/PairTradingPresenter.h \
    Strategies/PairTrader/pairtraderpm.h \
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
    Common/cprocessingbase.h \
    Common/GlobalDef.h \
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
    IBComm/Dispatcher.h \
    IBComm/IBComClientImpl.h \
    IBComm/IBrokerAPI.h \
    IBComm/IBworker.h \
    Logger/MyLogger.h \
    MainSystem/cpresenter.h \
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
    Strategies/AutoDeltAlignment/header/AutoDeltAlignmentGUI.h \
    Strategies/AutoDeltAlignment/header/AutoDeltAlignmentPresenter.h \
    Strategies/AutoDeltAlignment/header/AutoDeltAlignmentProcessing.h \
    Common/globalsettings.h \
    CObjects/ctickbytickalllast.h \
    CObjects/chistoricalticks.h \
    DBStore/dbstoremodel.h \
    Common/contractsdefs.h \
    Common/OrderSamples.h \
    #MainSystem/cstandartitemsettings.h \
    MainSystem/treeitem.h


FORMS += \
    AboutDialog/aboutdialog.ui \
    MainSystem/ibtradesystemview.ui \
    Strategies/PairTrader/pairtrading.ui \
    Strategies/AutoDeltAlignment/autodeltaaligform.ui \
    DBStore/dbstroreform.ui \
    Strategies/PairTrader/pairtrading.ui

INCLUDEPATH += \
    $$PWD/Brokers/IB/Shared \
    $$PWD/ReqManager \
    $$PWD/QCustomPlot \
    $$PWD/PairTrader \
    $$PWD/MainSystem \
    $$PWD/Logger \
    $$PWD/IBComm \
    $$PWD/DB \
    $$PWD/CustomWidgets \
    $$PWD/Common \
    $$PWD/CObjects \
    $$PWD/AlphaModelGetTime \
    $$PWD/AboutDialog \
    $$PWD/Strategies/AutoDeltAlignment/header\
    $$PWD/Strategies/PairTrader\
    $$PWD/Strategies/Generic\
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
