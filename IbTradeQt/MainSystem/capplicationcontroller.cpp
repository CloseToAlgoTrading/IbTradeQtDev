#include "capplicationcontroller.h"
#include "MyLogger.h"
#include <QtWidgets/QApplication>
#include <QProcess>
#include <QLibraryInfo>
#include <QStringList>
#include <QStandardPaths>
#include<QIcon>

CApplicationController::CApplicationController(): w(nullptr)
  , prst(nullptr)
{

}

int CApplicationController::run(int argc, char *argv[])
{
    QFont font("Courier New", 8);
    font.setStyleHint(QFont::Monospace);
    QApplication::setFont(font);

    QApplication a(argc, argv);

    QString ss = QCoreApplication::applicationDirPath();
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

    w = new IBTradeSystem;
    prst = new CPresenter(nullptr);




    prst->addView(w);
    prst->MapSignals();
    w->show();

    auto icon = QIcon(":/IBTradeSystem/x_resources/app.png");
    a.setWindowIcon(icon);

//    QFile file("Combinear.qss");
//    file.open(QFile::ReadOnly | QFile::Text);
//    QTextStream stream(&file);
//    QString styleSheet = stream.readAll();

//    a.setStyleSheet(styleSheet);

//    file.close();

    int result = a.exec();

    delete w;
    delete prst;

    return result;
}
