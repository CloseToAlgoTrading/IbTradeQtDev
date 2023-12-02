#include "capplicationcontroller.h"
#include "MyLogger.h"
#include <QtWidgets/QApplication>
#include <QProcess>
#include <QLibraryInfo>
#include <QStringList>
#include <QStandardPaths>
#include<QIcon>

CApplicationController::CApplicationController(QObject *parent):
    QObject(parent)
   , pMainPresenter(new CPresenter(parent))
   , pMainView(new CIBTradeSystemView)
   , m_pDataRoot(new CBasicRoot())
{
    loadTreeFromFile("model_tree_config.json", pMainPresenter->getDataProvider());

    this->pMainPresenter->addView(this->pMainView);

    pMainModel =new CMainModel(pMainPresenter, m_pDataRoot, nullptr);

    this->pMainPresenter->setPGuiModel(this->pMainModel);

    this->pMainPresenter->MapSignals();

    QObject::connect(pMainView->getUi().actionSave, &QAction::triggered, this, &CApplicationController::slotStoreModelTree);
}

CApplicationController::~CApplicationController()
{
    delete this->pMainView;
    delete this->pMainPresenter;
    delete this->pMainModel;
    delete this->m_pDataRoot;
}

void CApplicationController::setUpApplication(QApplication &app)
{
    QFont font("Courier New", 8);
    font.setStyleHint(QFont::Monospace);
    QApplication::setFont(font);

//    QString ss = QCoreApplication::applicationDirPath();
//    qInstallMessageHandler(MyLogger::myMessageOutput);

//    //Log output template
//    qSetMessagePattern("%{time [dd.MM.yy hh:mm:ss]}[%{type}][%{function}]: %{message}");

//    //Filter rules (default set in QtProject/qtlogging.ini)
//    //-----
//    // processing.Base, customCandleQChart.GUI,customQChart.GUI, dataProvider.General, ibComClient.Callback, ibComClientImpl.Callback
//    // pairTrader.PM, pairTrader.GUI
//    //-----
//    //Comment the following line if you want to use qtlogging.ini settings
//    QLoggingCategory::setFilterRules(QStringLiteral("\
//        pairTrader.*      = true  \n\
//        customQChart.*    = false \n\
//        ibComClient.*     = true  \n\
//        dataProviderLog.* = true  \n\
//        processing.*      = true  \n\
//        DBStore.*         = true  \n\
//        ibComClientImpl.* = true  \n"));

//    //MyLogger::setDebugLevelMask(MyLogger::LL_ALL);
//    MyLogger::setDebugLevelMask(MyLogger::LL_INFO|MyLogger::LL_DEBUG);

//    this->pMainPresenter->addView(this->pMainView);
//    this->pMainPresenter->MapSignals();


    auto icon = QIcon(":/IBTradeSystem/x_resources/app.png");
    app.setWindowIcon(icon);

//    QFile file("Combinear.qss");
//    file.open(QFile::ReadOnly | QFile::Text);
//    QTextStream stream(&file);
//    QString styleSheet = stream.readAll();

//    a.setStyleSheet(styleSheet);

//    file.close();
    this->pMainView->show();
}

void CApplicationController::setPMainModel(CMainModel *newPMainModel)
{
    pMainModel = newPMainModel;
}

void CApplicationController::loadTreeFromFile(const QString &fileName, QSharedPointer<CBrokerDataProvider> dataProvider)
{
    QFile file(fileName);
    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly)) {
            // Handle error
        }

        QByteArray jsonData = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        QJsonObject rootJson = doc.object();
        this->m_pDataRoot->setBrokerDataProvider(dataProvider);
        this->m_pDataRoot->fromJson(rootJson);
    }
}

void CApplicationController::slotStoreModelTree()
{
    QFile file("model_tree_config.json");
    if (!file.open(QIODevice::WriteOnly)) {
        // Handle error
    }

    QJsonObject rootJson = m_pDataRoot->toJson();
    QJsonDocument doc(rootJson);
    file.write(doc.toJson());

}
