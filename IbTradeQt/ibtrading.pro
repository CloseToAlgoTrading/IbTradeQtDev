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

CONFIG += c++14

UI_DIR = $$PWD/GeneratedIncludes

SOURCES += \
    CObjects/cdeltaobject.cpp \
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
    MainSystem/ibtradesystem.cpp \
    PairTrader/pairtraderpm.cpp \
    PairTrader/PairTradingGui.cpp \
    PairTrader/PairTradingPresenter.cpp \
    QCustomPlot/CustomPlotZoom.cpp \
    QCustomPlot/qcustomplot.cpp \
    ReqManager/globalreqmanager.cpp \
    ReqManager/ReqManager.cpp \
    src/ContractCondition.cpp \
    src/DefaultEWrapper.cpp \
    src/EClient.cpp \
    src/EClientSocket.cpp \
    src/EDecoder.cpp \
    src/EMessage.cpp \
    src/EMutex.cpp \
    src/EReader.cpp \
    src/EReaderOSSignal.cpp \
    src/ESocket.cpp \
    src/executioncondition.cpp \
    src/MarginCondition.cpp \
    src/OperatorCondition.cpp \
    src/OrderCondition.cpp \
    src/PercentChangeCondition.cpp \
    src/PriceCondition.cpp \
    src/SoftDollarTier.cpp \
    src/StdAfx.cpp \
    src/TimeCondition.cpp \
    src/VolumeCondition.cpp \
    Strategies/AutoDeltAlignment/src/AutoDeltAlignmentGUI.cpp \
    Strategies/AutoDeltAlignment/src/AutoDeltAlignmentPresenter.cpp \
    Strategies/AutoDeltAlignment/src/AutoDeltAlignmentProcessing.cpp \
    baseimpl.cpp \
    Common/globalsettings.cpp \
    CObjects/ctickbytickalllast.cpp \
    CObjects/chistoricalticks.cpp \
    MainSystem/settingsmodel.cpp \
    DBStore/dbstoremodel.cpp \
    Common/contractsdefs.cpp \
    Common/OrderSamples.cpp \
    MainSystem/cstandartitemsettings.cpp


HEADERS += \
    CObjects/cdeltaobject.h \
    Strategies/AutoDeltAlignment/header/autodeltatypes.h \
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
    Common/standartincludes.h \
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
    MainSystem/ibtradesystem.h \
    PairTrader/pairtraderpm.h \
    PairTrader/PairTradingGui.h \
    PairTrader/PairTradingPresenter.h \
    QCustomPlot/CustomPlotZoom.h \
    QCustomPlot/GraphHelper.h \
    QCustomPlot/qcustomplot.h \
    ReqManager/globalreqmanager.h \
    ReqManager/ReqManager.h \
    Shared/bar.h \
    Shared/CommissionReport.h \
    Shared/CommonDefs.h \
    Shared/Contract.h \
    Shared/ContractCondition.h \
    Shared/DefaultEWrapper.h \
    Shared/DepthMktDataDescription.h \
    Shared/EClient.h \
    Shared/EClientMsgSink.h \
    Shared/EClientSocket.h \
    Shared/EDecoder.h \
    Shared/EMessage.h \
    Shared/EMutex.h \
    Shared/EPosixClientSocketPlatform.h \
    Shared/EReader.h \
    Shared/EReaderOSSignal.h \
    Shared/EReaderSignal.h \
    Shared/ESocket.h \
    Shared/ETransport.h \
    Shared/EWrapper.h \
    Shared/EWrapper_prototypes.h \
    Shared/Execution.h \
    Shared/executioncondition.h \
    Shared/FamilyCode.h \
    Shared/HistogramEntry.h \
    Shared/HistoricalTick.h \
    Shared/HistoricalTickBidAsk.h \
    Shared/HistoricalTickLast.h \
    Shared/IExternalizable.h \
    Shared/MarginCondition.h \
    Shared/NewsProvider.h \
    Shared/OperatorCondition.h \
    Shared/Order.h \
    Shared/OrderCondition.h \
    Shared/OrderState.h \
    Shared/PercentChangeCondition.h \
    Shared/PriceCondition.h \
    Shared/PriceIncrement.h \
    Shared/Resource.h \
    Shared/ScannerSubscription.h \
    Shared/shared_ptr.h \
    Shared/SoftDollarTier.h \
    Shared/StdAfx.h \
    Shared/TagValue.h \
    Shared/TickAttrib.h \
    Shared/TickAttribBidAsk.h \
    Shared/TickAttribLast.h \
    Shared/TimeCondition.h \
    Shared/TwsSocketClientErrors.h \
    Shared/VolumeCondition.h \
    Strategies/AutoDeltAlignment/header/AutoDeltAlignmentGUI.h \
    Strategies/AutoDeltAlignment/header/AutoDeltAlignmentPresenter.h \
    Strategies/AutoDeltAlignment/header/AutoDeltAlignmentProcessing.h \
    Common/globalsettings.h \
    CObjects/ctickbytickalllast.h \
    CObjects/chistoricalticks.h \
    MainSystem/settingsmodel.h \
    DBStore/dbstoremodel.h \
    Common/contractsdefs.h \
    Common/OrderSamples.h \
    MainSystem/cstandartitemsettings.h


FORMS += \
    AboutDialog/aboutdialog.ui \
    MainSystem/ibtradesystem.ui \
    PairTrader/pairtrading.ui \
    Strategies/AutoDeltAlignment/autodeltaaligform.ui \
    DBStore/dbstroreform.ui

INCLUDEPATH += \
    $$PWD/Shared \
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
    $$PWD/GeneratedIncludes \
    $$PWD/DBStore



win32:INCLUDEPATH += "c:/Program Files/PostgreSQL/11/include/"
unix:INCLUDEPATH += /usr/include/postgresql

#win32:LIBS += "c:/Program Files/PostgreSQL/11/libpq.lib"
#unix:LIBS += -L/usr/lib -lpq

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

*msvc* { # visual studio spec filter
      QMAKE_CXXFLAGS += -MP
  }

SUBDIRS += \
    ibtrading.pro

RESOURCES += \
    ibtradesystem.qrc

DISTFILES += \
    doc/mainClass.wsd
