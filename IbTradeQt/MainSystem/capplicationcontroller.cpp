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

#include "cpipelinestrategyadapter.h"
#include "IBComClientImpl.h"

CApplicationController::CApplicationController(QObject *parent):
    QObject(parent)
   , pMainPresenter(new CPresenter(parent))
   , pMainView(new CIBTradeSystemView)
   , m_pDataRoot(new CBasicRoot())
{
    // Create typed routers BEFORE loadTreeFromFile so they propagate via setBrokerDataProvider
    m_pPositionRouter = new IBComm::PositionRouter(this);
    m_pHistoricalDataRouter = new IBComm::HistoricalDataRouter(this);
    m_pOrderRouter = new IBComm::OrderRouter(this);
    m_pAccountRouter = new IBComm::AccountRouter(this);
    m_pTimeRouter = new IBComm::TimeRouter(this);

    IBrokerAPI* brokerApi = pMainPresenter->getDataProvider()->getClien().data();
    auto* implClient = dynamic_cast<IBComClientImpl*>(brokerApi);

    if (implClient) {
        implClient->setPositionRouter(m_pPositionRouter);
        implClient->setHistoricalDataRouter(m_pHistoricalDataRouter);
        implClient->setOrderRouter(m_pOrderRouter);
        implClient->setAccountRouter(m_pAccountRouter);
        implClient->setTimeRouter(m_pTimeRouter);
    }

    auto dp = pMainPresenter->getDataProvider();
    dp->setOrderRouter(m_pOrderRouter);
    dp->setAccountRouter(m_pAccountRouter);
    dp->setPositionRouter(m_pPositionRouter);
    dp->setHistoricalDataRouter(m_pHistoricalDataRouter);
    dp->setTimeRouter(m_pTimeRouter);

    loadTreeFromFile("model_tree_config.json", dp);

    this->pMainPresenter->addView(this->pMainView);

    pMainModel =new CMainModel(pMainPresenter, m_pDataRoot, nullptr);

    this->pMainPresenter->setPGuiModel(this->pMainModel);

    // Connect AlphaModGetTime to TimeRouter
    if (m_pTimeRouter) {
        QObject::connect(m_pTimeRouter, &IBComm::TimeRouter::currentTimeReceived,
                         pMainPresenter->getWorkerAlfaTime(), &AlphaModGetTime::slotCurrentTimeReceived,
                         Qt::QueuedConnection);
    }

    this->pMainPresenter->MapSignals();

    QObject::connect(pMainView->getUi().actionSave, &QAction::triggered, this, &CApplicationController::slotStoreModelTree);

    m_pSupervisor = new Supervision::Supervisor(this);
    m_pSupervisor->startMonitoring(10000);

    CPipelineStrategyAdapter::setGlobalRouter(pMainPresenter->marketDataRouter());
    CPipelineStrategyAdapter::setGlobalSupervisor(m_pSupervisor);

    m_pExecutionAdapter = new IBOrderExecutionAdapter(brokerApi);
    CPipelineStrategyAdapter::setGlobalExecutionPort(m_pExecutionAdapter);

    m_pPositionRepo = new SqlitePositionRepository("myLocalDb.sqlite", true);

    m_pLivePositionRepo = new IBPositionRepositoryAdapter(this);
    m_pLivePositionRepo->connectToRouter(m_pPositionRouter);

    connect(m_pOrderRouter, &IBComm::OrderRouter::orderStatusChanged,
            this, [this](const IBComm::OrderStatusUpdate& update) {
        if (m_pExecutionAdapter)
            m_pExecutionAdapter->updateOrderStatus(update.orderId, update.status);
    });
    connect(m_pOrderRouter, &IBComm::OrderRouter::executionReceived,
            this, [this](const IBComm::ExecutionReport& report) {
        if (m_pExecutionAdapter)
            m_pExecutionAdapter->updateOrderStatus(report.orderId, "Filled");
    });

    CPipelineStrategyAdapter::setGlobalPositionRepo(m_pLivePositionRepo);
    CPipelineStrategyAdapter::setGlobalPersistentPositionRepo(m_pPositionRepo);

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
    if (m_pSupervisor) {
        m_pSupervisor->stopAll();
    }
    delete m_pExecutionAdapter;
    delete m_pPositionRepo;
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
