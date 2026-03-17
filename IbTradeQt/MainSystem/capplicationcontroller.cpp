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
#include "Pipeline/BlockRegistry.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/MeanReversionAlphaBlock.h"
#include "Blocks/MovingAverageCrossoverAlphaBlock.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MarketOrderExecutionBlock.h"
#include "Blocks/LimitOrderExecutionBlock.h"
#include "Blocks/StaticListSelectionBlock.h"

static void registerBuiltinBlocks()
{
    auto& reg = Pipeline::BlockRegistry::instance();
    if (reg.blockCount() > 0) return;

    reg.registerBlock({"momentum-alpha", "Momentum Alpha", "Alpha",
                       "Momentum-based signal: long when return > threshold",
                       Pipeline::Scope::Strategy, {{"period", 20}, {"threshold", 0.02}},
                       []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }});

    reg.registerBlock({"mean-reversion-alpha", "Mean Reversion Alpha", "Alpha",
                       "Mean reversion: long/short when price deviates from moving average",
                       Pipeline::Scope::Strategy, {{"period", 20}, {"stdDevThreshold", 2.0}},
                       []() -> QObject* { return new Blocks::MeanReversionAlphaBlock(); }});

    reg.registerBlock({"ma-crossover-alpha", "MA Crossover Alpha", "Alpha",
                       "Moving average crossover: long when fast MA > slow MA",
                       Pipeline::Scope::Strategy, {{"fastPeriod", 10}, {"slowPeriod", 30}},
                       []() -> QObject* { return new Blocks::MovingAverageCrossoverAlphaBlock(); }});

    reg.registerBlock({"max-position-risk", "Max Position Risk", "Risk",
                       "Limits position size and total exposure",
                       Pipeline::Scope::Strategy, {{"maxPositionSize", 500.0}, {"maxTotalExposure", 3000.0}},
                       []() -> QObject* { return new Blocks::MaxPositionRiskBlock(); }});

    reg.registerBlock({"market-order-execution", "Market Order Execution", "Execution",
                       "Executes market orders",
                       Pipeline::Scope::Strategy, {{"minQuantity", 1.0}},
                       []() -> QObject* { return new Blocks::MarketOrderExecutionBlock(); }});

    reg.registerBlock({"limit-order-execution", "Limit Order Execution", "Execution",
                       "Executes limit orders with configurable offset",
                       Pipeline::Scope::Strategy, {{"minQuantity", 1.0}, {"limitOffset", 0.01}},
                       []() -> QObject* { return new Blocks::LimitOrderExecutionBlock(); }});

    reg.registerBlock({"simple-rebalance", "Simple Rebalance", "Rebalance",
                       "Fixed-quantity rebalancer",
                       Pipeline::Scope::Strategy, {{"defaultQuantity", 100.0}},
                       []() -> QObject* { return new Blocks::SimpleRebalanceBlock(); }});

    reg.registerBlock({"static-list-selection", "Static List Selection", "Selection",
                       "Selects from a fixed list of symbols",
                       Pipeline::Scope::Strategy, {{"symbols", "AAPL,MSFT"}},
                       []() -> QObject* { return new Blocks::StaticListSelectionBlock(); }});
}

CApplicationController::CApplicationController(QObject *parent):
    QObject(parent)
   , pMainPresenter(new CPresenter(parent))
   , pMainView(new CIBTradeSystemView)
   , m_pDataRoot(new CBasicRoot())
{
    registerBuiltinBlocks();

    // Create typed routers BEFORE loadTreeFromFile so they propagate via setBrokerDataProvider
    m_pPositionRouter = new IBComm::PositionRouter(this);
    m_pHistoricalDataRouter = new IBComm::HistoricalDataRouter(this);
    m_pOrderRouter = new IBComm::OrderRouter(this);
    m_pAccountRouter = new IBComm::AccountRouter(this);
    m_pTimeRouter = new IBComm::TimeRouter(this);
    m_pMarketDepthRouter = new IBComm::MarketDepthRouter(this);

    IBrokerAPI* brokerApi = pMainPresenter->getDataProvider()->getClien().data();
    auto* implClient = dynamic_cast<IBComClientImpl*>(brokerApi);

    if (implClient) {
        implClient->setPositionRouter(m_pPositionRouter);
        implClient->setHistoricalDataRouter(m_pHistoricalDataRouter);
        implClient->setOrderRouter(m_pOrderRouter);
        implClient->setAccountRouter(m_pAccountRouter);
        implClient->setTimeRouter(m_pTimeRouter);
        implClient->setMarketDepthRouter(m_pMarketDepthRouter);
    }

    auto dp = pMainPresenter->getDataProvider();
    dp->setOrderRouter(m_pOrderRouter);
    dp->setAccountRouter(m_pAccountRouter);
    dp->setPositionRouter(m_pPositionRouter);
    dp->setHistoricalDataRouter(m_pHistoricalDataRouter);
    dp->setTimeRouter(m_pTimeRouter);
    dp->setMarketDepthRouter(m_pMarketDepthRouter);

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

    // Load operations console stylesheet
    QFile qssFile(":/style/operations-console.qss");
    if (!qssFile.exists())
        qssFile.setFileName(QStringLiteral("MainSystem/style/operations-console.qss"));
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(qssFile.readAll());
        qssFile.close();
    }

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
