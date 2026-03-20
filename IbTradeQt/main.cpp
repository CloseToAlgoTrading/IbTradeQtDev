#include "capplicationcontroller.h"

#include <clocale>

#define QT_LOGGING_DEBUG 1
int main(int argc, char *argv[])
{

    //QString ss = QCoreApplication::applicationDirPath();
    qInstallMessageHandler(MyLogger::myMessageOutput);

    //Log output template
    qSetMessagePattern("%{time [dd.MM.yy hh:mm:ss]}[%{type}][%{function}]: %{message}");

    // Filter rules: keys are logging category *name* strings (2nd arg to Q_LOGGING_CATEGORY), not C++ vars.
    // Comment out to use qtlogging.ini instead.
    QLoggingCategory::setFilterRules(QStringLiteral(
        "pairTrader.*       = true\n"
        "AutoDeltaAlig.*    = true\n"
        "customQChart.*       = false\n"
        "customCandleQChart.* = false\n"
        "ibComClientImpl.*    = true\n"
        "dataProvider.*       = true\n"
        "processing.*         = true\n"
        "DBStore.*            = true\n"
        "backtest.*           = true\n"
        "backend.*            = true\n"
        "db.*                 = true\n"
        "pipeline.*           = true\n"
        "strategyMgmt.*       = true\n"
        "shared.*             = true\n"
        "app.*                = true\n"
        "ui.*                 = true\n"
        "reqManager.*         = true\n"
        "baseModel.*          = true\n"
        "basicStrategy.*      = true\n"
        "portfolio.*          = true\n"));

    //MyLogger::setDebugLevelMask(MyLogger::LL_ALL);
    MyLogger::setDebugLevelMask(MyLogger::LL_INFO|MyLogger::LL_DEBUG);


    QApplication a(argc, argv);
    setlocale(LC_NUMERIC,"C");
    CApplicationController app;
    app.setUpApplication(a);
    return a.exec();
}
