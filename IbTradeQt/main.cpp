
#include <QtWidgets/QApplication>
#include <QProcess>
#include <QLibraryInfo>
#include <QStringList>
#include <QStandardPaths>

#include "cpresenter.h"
#include "ibtradesystem.h"
#include "MyLogger.h"

#include<QIcon>

#define QT_LOGGING_DEBUG 1
int main(int argc, char *argv[])
{
    QFont font("Courier New", 8);
    font.setStyleHint(QFont::Monospace);
    QApplication::setFont(font);

   // QLocale::setDefault(QLocale(QLocale::English, QLocale::RussianFederation));

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

	IBTradeSystem w;
	CPresenter prst(nullptr);




	prst.addView(&w);
	prst.MapSignals();
	w.show();

    auto icon = QIcon(":/IBTradeSystem/x_resources/app.png");
    a.setWindowIcon(icon);

//    QFile file("Combinear.qss");
//    file.open(QFile::ReadOnly | QFile::Text);
//    QTextStream stream(&file);
//    QString styleSheet = stream.readAll();

//    a.setStyleSheet(styleSheet);

//    file.close();

	return a.exec();
}
