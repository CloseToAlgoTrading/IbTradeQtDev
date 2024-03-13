#include "capplicationcontroller.h"
#include "MyLogger.h"
#include <QtWidgets/QApplication>
#include <QProcess>
#include <QLibraryInfo>
#include <QStringList>
#include <QStandardPaths>
#include <QIcon>
#include <QFile>
#include <QJsonDocument>

/******* xxx *********/
#include "dbmanager.h"
#include "dbdatatypes.h"
#include <QDateTime>
/******* xxx *********/

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

    /*** Test Code ***/
    // DBManager m_dbManager;
    // QDateTime currentDateTime = QDateTime::currentDateTime();

    // DbTrade newTrade;
    // newTrade.strategyId = 1;  // Example data
    // newTrade.symbol = "XXX";
    // newTrade.quantity = -200;
    // newTrade.price = 300.0;
    // newTrade.pnl = 20.0;
    // newTrade.fee = 0.5;
    // newTrade.date = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");
    // newTrade.tradeType = "SELL";

    // m_dbManager.signalAddNewTrade(newTrade);

    /******* xxx *********/

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

    auto icon = QIcon(":/IBTradeSystem/x_resources/app.png");
    app.setWindowIcon(icon);

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
