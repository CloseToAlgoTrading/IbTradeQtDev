#include "capplicationcontroller.h"

#define QT_LOGGING_DEBUG 1
int main(int argc, char *argv[])
{

    //QString ss = QCoreApplication::applicationDirPath();
    qInstallMessageHandler(MyLogger::myMessageOutput);

    //Log output template
    qSetMessagePattern("%{time [dd.MM.yy hh:mm:ss]}[%{type}][%{function}]: %{message}");

    //Filter rules (default set in QtProject/qtlogging.ini)
    //-----
    // processing.Base, customCandleQChart.GUI,customQChart.GUI, dataProvider.General, ibComClient.Callback, ibComClientImpl.Callback
    // pairTrader.PM, pairTrader.GUI
    //-----
    //Comment the following line if you want to use qtlogging.ini settings
    QLoggingCategory::setFilterRules(QStringLiteral("\
        pairTrader.*      = true  \n\
        customQChart.*    = false \n\
        ibComClient.*     = true  \n\
        dataProviderLog.* = true  \n\
        processing.*      = true  \n\
        DBStore.*         = true  \n\
        ibComClientImpl.* = true  \n"));

    //MyLogger::setDebugLevelMask(MyLogger::LL_ALL);
    MyLogger::setDebugLevelMask(MyLogger::LL_INFO|MyLogger::LL_DEBUG);


    QApplication a(argc, argv);
    CApplicationController app;
    app.setUpApplication(a);
//    return app.run(argc, argv);
    return a.exec();
}
