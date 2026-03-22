#include "cpresenter.h"
#include "ISystemBackend.h"
#include "ReqManager.h"
#include "Brokers/BrokerConnectionFactory.h"
#include "IBComClientImpl.h"
#include "cmainmodel.h"
#include "CPortfolioConfigModel.h"
#include "PipelineItemDelegate.h"
#include "PipelineDiagramWidget.h"
#include "cpipelinestrategyadapter.h"
#include "cstrategyfactory.h"
#include "cbasicaccount.h"
#include "cbasicportfolio.h"
#include "cbasicroot.h"
#include "Pipeline/PipelineConstants.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PortfolioModelDefines.h"
#include "SystemTreeModel.h"
#include "SystemTreeDelegate.h"
#include "GlobalStatusBar.h"
#include "EventLogPanel.h"
#include "ContextWorkspace.h"
#include "AlertService.h"
#include <QDockWidget>
#include <QMenu>
#include <QPoint>
#include <QUuid>
#include "BacktestWorkspaceCoordinator.h"
#include "StrategyManagementCoordinator.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"
#include "UiLayoutDefaults.h"
#include "SettingsTreeDelegate.h"
#include <QJsonDocument>
#include <QAbstractItemView>
#include <QTreeView>


CPresenter::CPresenter(QObject *parent)
	: QObject(parent)
    , m_pLog(LOGGER)
    , m_pDataProvider(QSharedPointer<CBrokerDataProvider>::create())
    , pIbtsView(nullptr)
    , pGuiModel(nullptr)
    , threadIBClient(new QThread)
    , workerIBClient(new IBWorker::Worker(parent, *m_pDataProvider.data()))
    , threadAlfaTime(new QThread)
    , workerAlfaTime(new AlphaModGetTime(parent, *m_pDataProvider.data()))
    , pAboutDlgPresenter(new AboutDlgPresener(parent))
{
    QSharedPointer<IBrokerAPI> pClient = Brokers::createBrokerApi(QStringLiteral("ib"));
    m_pDataProvider->setClien(pClient);

    m_pMarketDataRouter = new IBComm::MarketDataRouter(this);
    if (auto* ibImpl = dynamic_cast<IBComClientImpl*>(pClient.data()))
        ibImpl->setMarketDataRouter(m_pMarketDataRouter);

    workerIBClient->moveToThread(threadIBClient);
    QObject::connect(threadIBClient, SIGNAL(started()), workerIBClient, SLOT(process()));
    threadIBClient->start();

    QThread::currentThread()->setObjectName("mainThread");
    threadIBClient->setObjectName("myThread");
}

CPresenter::~CPresenter()
{
    workerIBClient->setCommand(IBWorker::EXIT);
}


void CPresenter::MapSignals()
{
    QObject::connect(pIbtsView->getUi().actionConnect, &QAction::triggered, this, &CPresenter::onClickMyButton);

    QObject::connect(workerAlfaTime, SIGNAL(signalTimeReceived(long)), pIbtsView, SLOT(slotOnTimeReceived(long)));
    QObject::connect(this, SIGNAL(signalClickConnect(bool)), pIbtsView, SLOT(slotRecvConnectButtonState(bool)));
    QObject::connect(&LOGGER, SIGNAL(signalAddLogMsg(QString)), pIbtsView, SLOT(slotOnLogMsgReceived(QString)));
    QObject::connect(&LOGGER, SIGNAL(signalQtStructuredLog(int,QString,QString,QString)),
                     pIbtsView, SLOT(slotOnQtStructuredLog(int,QString,QString,QString)));

    QTreeView * pTreeView = this->pIbtsView->getPortfolioConfigTreeView();
    CPortfolioConfigModel *pPConfigModel = this->getPGuiModel()->pPortfolioConfigModel();

    auto* sysModel = m_pSystemTreeModel;
    auto* backend = m_backend;

    // ── Context menu (tree right-click) ──────────────────────────────────
    QObject::connect(pTreeView, &QTreeView::customContextMenuRequested,
                     this->pIbtsView, [pTreeView, sysModel, pPConfigModel, backend, this](const QPoint& pos) {
        QModelIndex index = pTreeView->indexAt(pos);

        QMenu menu;

        QAction* addAccount = menu.addAction("Add New Account");

        QAction* addPortfolio     = nullptr;
        QAction* addNewStrategy   = nullptr;
        QAction* useExistingStrat = nullptr;
        QAction* removeNode       = nullptr;
        QAction* openBacktest     = nullptr;

        CGenericModelApi* clickedModel = nullptr;
        ModelType clickedType = ModelType::ROOT;

        if (index.isValid() && sysModel) {
            clickedModel = sysModel->modelAt(index);
            if (clickedModel)
                clickedType = clickedModel->modelType();

            if (clickedType == ModelType::ACCOUNT) {
                addPortfolio = menu.addAction("Add New Portfolio");
            }
            if (clickedType == ModelType::PORTFOLIO) {
                addNewStrategy = menu.addAction("Add New Strategy");
                useExistingStrat = menu.addAction("Use Existing Strategy...");
            }

            bool isStrategy = (clickedType == ModelType::STRATEGY ||
                               clickedType == ModelType::STRATEGY_BASIC_TEST ||
                               clickedType == ModelType::STRATEGY_MA ||
                               clickedType == ModelType::STRATEGY_MOMENTUM ||
                               clickedType == ModelType::STRATEGY_PIPELINE);

            if (isStrategy) {
                openBacktest = menu.addAction("Open in Backtest Workspace");
            }

            if (clickedModel) {
                menu.addSeparator();
                removeNode = menu.addAction("Remove Selected Node");
            }
        }

        QAction* chosen = menu.exec(pTreeView->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        auto rebuildTree = [&]() {
            sysModel->rebuildFromRoot();
            pPConfigModel->setupModelData();
            pTreeView->expandAll();
        };

        if (chosen == addAccount && backend) {
            backend->createAccount("Account");
            rebuildTree();
        }
        else if (chosen == addPortfolio && clickedModel && backend) {
            QString accountId = clickedModel->getId().toString(QUuid::WithoutBraces);
            backend->createPortfolio(accountId, "Portfolio");
            rebuildTree();
        }
        else if (chosen == addNewStrategy && clickedModel && backend) {
            QString portfolioId = clickedModel->getId().toString(QUuid::WithoutBraces);
            backend->createStrategy(portfolioId, ModelType::STRATEGY_PIPELINE);
            rebuildTree();
        }
        else if (chosen == useExistingStrat && clickedModel && backend) {
            QJsonArray catalog = backend->listStrategyCatalog(false);
            QStringList choices;
            QMap<int, QPair<QString, QString>> indexMap;
            int ci = 0;
            for (const auto& e : catalog) {
                QJsonObject obj = e.toObject();
                QString sid = obj["strategyId"].toString();
                QJsonArray versions = backend->listStrategyVersions(sid);
                for (const auto& v : versions) {
                    QJsonObject vo = v.toObject();
                    if (!vo["isPublished"].toBool()) continue;
                    QString label = QStringLiteral("%1 v%2")
                        .arg(obj["name"].toString())
                        .arg(vo["versionNumber"].toInt());
                    choices << label;
                    indexMap[ci++] = {sid, vo["versionId"].toString()};
                }
            }
            if (choices.isEmpty()) {
                QMessageBox::information(pIbtsView, QStringLiteral("No Published Versions"),
                    QStringLiteral("No published strategy versions found. Publish a version from the Strategy Management tab first."));
            } else {
                bool ok = false;
                QString picked = QInputDialog::getItem(pIbtsView,
                    QStringLiteral("Use Existing Strategy"),
                    QStringLiteral("Select a published strategy version:"),
                    choices, 0, false, &ok);
                if (ok) {
                    int idx = choices.indexOf(picked);
                    if (idx >= 0 && indexMap.contains(idx)) {
                        auto [sid, vid] = indexMap[idx];
                        QString portfolioId = clickedModel->getId().toString(QUuid::WithoutBraces);
                        QString nodeId = backend->createLiveNodeForExistingCatalog(
                            portfolioId, ModelType::STRATEGY_PIPELINE, sid, vid);
                        rebuildTree();
                    }
                }
            }
        }
        else if (chosen == openBacktest && clickedModel && backend) {
            QString strategyId = clickedModel->getId().toString(QUuid::WithoutBraces);
            QJsonObject pipelineCfg = backend->pipelineConfig(strategyId);
            CGenericModelApi* parentModel = sysModel->parentModelAt(index);
            QString portfolioPath = parentModel ? parentModel->getName() : "";
            emit pPConfigModel->openInBacktestWorkspace(
                strategyId,
                clickedModel->getName(),
                portfolioPath,
                pipelineCfg);
        }
        else if (chosen == removeNode && clickedModel && backend) {
            QString uuid = clickedModel->getId().toString(QUuid::WithoutBraces);
            backend->removeNode(uuid);
            rebuildTree();
        }
    });

    // ── Diagram dock (tabified with Events — same bottom dock tab bar) ───
    m_pDiagramWidget = new PipelineDiagramWidget();
    m_pDiagramDock = new QDockWidget(QStringLiteral("Diagram View"), this->pIbtsView);
    m_pDiagramDock->setObjectName(QStringLiteral("DiagramViewDock"));
    m_pDiagramDock->setWidget(m_pDiagramWidget);
    m_pDiagramDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_pDiagramDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    this->pIbtsView->addDockWidget(Qt::BottomDockWidgetArea, m_pDiagramDock);
    if (EventLogPanel* eventDock = this->pIbtsView->eventLogPanel()) {
        this->pIbtsView->tabifyDockWidget(eventDock, m_pDiagramDock);
        eventDock->raise();
    }
    m_pDiagramDock->show();
    this->pIbtsView->setDiagramDock(m_pDiagramDock);

    // ── Backtest Workspace (delegated to BacktestWorkspaceCoordinator) ───
    m_backtestCoord = new BacktestWorkspaceCoordinator(this);
    m_backtestCoord->setView(pIbtsView);
    m_backtestCoord->setBackend(m_backend);

    auto* btDock = new BacktestUI::BacktestWorkspaceDock(this->pIbtsView);
    btDock->hide();
    m_backtestCoord->setDock(btDock);

    auto* backtestRightPane = this->pIbtsView->findChild<QWidget*>(
        QStringLiteral("BacktestRightPane"));
    if (backtestRightPane && btDock->widget()) {
        QLayout* paneLayout = backtestRightPane->layout();
        while (QLayoutItem* item = paneLayout->takeAt(0)) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        QWidget* btContent = btDock->widget();
        btContent->setParent(backtestRightPane);
        paneLayout->addWidget(btContent);
    }

    QObject::connect(pPConfigModel, &CPortfolioConfigModel::openInBacktestWorkspace,
                     m_backtestCoord, [this](const QString& id, const QString& name,
                                             const QString& path, const QJsonObject& cfg) {
        m_backtestCoord->openStrategy(id, name, path, cfg);
    }, Qt::QueuedConnection);

    m_backtestCoord->wireSignals();

    // ── Strategy Management (delegated to StrategyManagementCoordinator) ─
    m_stratMgmtCoord = new StrategyManagementCoordinator(this);
    m_stratMgmtCoord->setView(pIbtsView);
    m_stratMgmtCoord->setBackend(m_backend);
    m_stratMgmtCoord->setPanel(pIbtsView->strategyManagementPanel());
    m_stratMgmtCoord->wireSignals();

    connect(m_stratMgmtCoord, &StrategyManagementCoordinator::openInBacktest,
            m_backtestCoord, &BacktestWorkspaceCoordinator::openCatalogVersion);

    connect(m_backtestCoord, &BacktestWorkspaceCoordinator::catalogRefreshNeeded,
            m_stratMgmtCoord, &StrategyManagementCoordinator::refreshCatalog);

    connect(m_stratMgmtCoord, &StrategyManagementCoordinator::refreshLiveTree,
            this, [this, pPConfigModel]() {
        if (pPConfigModel) {
            pPConfigModel->setupModelData();
            pIbtsView->slotUpdateTreeViewAll();
        }
    });

    // ── Tab switch → refresh ─────────────────────────────────────────────
    if (auto* tabs = this->pIbtsView->mainTabWidget()) {
        connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
            if (index == 1)
                m_backtestCoord->refreshStrategies();
            else if (index == 2)
                m_stratMgmtCoord->refreshCatalog();
        });
    }

    // ── Backend divergence signal ────────────────────────────────────────
    if (m_backend) {
        connect(m_backend, &ISystemBackend::nodeConfigDiverged,
                this, [this](const QString& nodeId) {
            if (!m_backend || !m_backend->isBrokerConnected()) return;
            CGenericModelApi* root = m_backend->dataRoot();
            if (!root) return;

            QString nodeName = nodeId;
            auto findName = [&]() {
                for (auto& acct : root->getModels())
                    for (auto& port : acct->getModels())
                        for (auto& strat : port->getModels())
                            if (strat->getId().toString(QUuid::WithoutBraces) == nodeId) {
                                nodeName = strat->getName();
                                return;
                            }
            };
            findName();

            QMessageBox::information(
                pIbtsView,
                QStringLiteral("Strategy Config Diverged"),
                QStringLiteral("Strategy \"%1\" has diverged from its pinned version.\n"
                               "Consider saving a new version via the Strategy Management tab.")
                    .arg(nodeName));
        });
    }

    // ── Selection changed ────────────────────────────────────────────────
    if (pTreeView->selectionModel()) {
        QObject::connect(pTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
                         this, &CPresenter::onTreeSelectionChanged);
    }

    QObject::connect(pPConfigModel, &CPortfolioConfigModel::pipelineConfigChanged,
                     m_pDiagramWidget, &PipelineDiagramWidget::updateFromConfig);

    QObject::connect(pPConfigModel, SIGNAL(signalUpdateData(QModelIndex)), this->pIbtsView, SLOT(slotUpdateTreeView(QModelIndex)));
    QObject::connect(pPConfigModel, &CPortfolioConfigModel::signalUpdateDataAll, this->pIbtsView, &CIBTradeSystemView::slotUpdateTreeViewAll, Qt::QueuedConnection);

    this->pIbtsView->mapSignals();

    if (pGuiModel && pGuiModel->alertService() && pIbtsView->globalStatusBar()) {
        QObject::connect(pGuiModel->alertService(), &AlertService::countChanged,
                         pIbtsView->globalStatusBar(), &GlobalStatusBar::setAlertCount);
    }

    if (pIbtsView->globalStatusBar()) {
        QObject::connect(pIbtsView->globalStatusBar(), &GlobalStatusBar::reconnectClicked,
                         this, &CPresenter::onClickMyButton);
    }
}

void CPresenter::addView(CIBTradeSystemView * mw)
{
	this->pIbtsView = mw;
}


void CPresenter::onClickMyButton()
{
    static bool buttonState = false;
    if ((false == buttonState) && (!m_pDataProvider->getClien()->isConnectedAPI()))
    {
        if (m_backend) {
            QStringList divergedNames;
            CGenericModelApi* root = m_backend->dataRoot();
            if (root) {
                for (auto& acct : root->getModels())
                    for (auto& port : acct->getModels())
                        for (auto& strat : port->getModels()) {
                            if (strat->modelType() != ModelType::STRATEGY_PIPELINE) continue;
                            QString nid = strat->getId().toString(QUuid::WithoutBraces);
                            if (m_backend->isNodeDivergedFromVersion(nid))
                                divergedNames << strat->getName();
                        }
            }

            if (!divergedNames.isEmpty()) {
                QString msg = QStringLiteral(
                    "The following strategies have been modified since their last saved version:\n\n");
                for (const QString& n : divergedNames)
                    msg += QStringLiteral("  - ") + n + QStringLiteral("\n");
                msg += QStringLiteral(
                    "\nWould you like to save new versions before connecting?\n"
                    "Click Yes to create new versions, No to connect with unsaved changes, "
                    "or Cancel to abort.");

                auto result = QMessageBox::question(
                    pIbtsView, QStringLiteral("Strategy Config Changed"),
                    msg,
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                    QMessageBox::Yes);

                if (result == QMessageBox::Cancel) return;

                if (result == QMessageBox::Yes) {
                    for (auto& acct : root->getModels())
                        for (auto& port : acct->getModels())
                            for (auto& strat : port->getModels()) {
                                if (strat->modelType() != ModelType::STRATEGY_PIPELINE) continue;
                                QString nid = strat->getId().toString(QUuid::WithoutBraces);
                                if (!m_backend->isNodeDivergedFromVersion(nid)) continue;

                                QJsonObject defJson = m_backend->strategyDefinitionForNode(nid);
                                QString catalogStratId = defJson.value("strategyDefId").toString();
                                if (catalogStratId.isEmpty()) continue;

                                QJsonObject nodeConfig = m_backend->pipelineConfig(nid);
                                QString newVerId = m_backend->createStrategyVersion(
                                    catalogStratId, nodeConfig,
                                    QStringLiteral("Auto-saved before broker connect"));
                                if (!newVerId.isEmpty())
                                    m_backend->bindLiveNodeToVersion(nid, catalogStratId, newVerId);
                            }
                }
            }

            m_backend->connectBroker();
        }

        workerIBClient->setCommand(IBWorker::CONNECT);
        workerAlfaTime->StartGetTimeUpdate(1000);

        emit signalClickConnect(true);
        buttonState = true;
    }
    else
    {
        workerAlfaTime->StopTimeUpdate();
        workerIBClient->setCommand(IBWorker::DISCONNECT);

        if (m_backend) m_backend->disconnectBroker();
        emit signalClickConnect(false);

        buttonState = false;
    }
}


void CPresenter::onClickPairTraderButton()
{
}

void CPresenter::onClickAutoDeltaButton()
{
}

void CPresenter::onClickDBStoreButton()
{
}

QSharedPointer<CBrokerDataProvider> CPresenter::getDataProvider() const
{
    return m_pDataProvider;
}

bool CPresenter::storageReconfigurationAllowed() const
{
    if (m_pDataProvider && m_pDataProvider->isConnectedToTheServer())
        return false;
    if (m_backtestCoord && m_backtestCoord->isBacktestRunning())
        return false;
    return true;
}

void CPresenter::setBackend(ISystemBackend* backend)
{
    m_backend = backend;
}

CMainModel *CPresenter::getPGuiModel() const
{
    return pGuiModel;
}

void CPresenter::setPGuiModel(CMainModel *newPGuiModel)
{
   pGuiModel = newPGuiModel;
   this->pIbtsView->getSettingsTreeView()->setModel(this->pGuiModel->pSettingsModel());
   this->pIbtsView->getSettingsTreeView()->setItemDelegate(new SettingsTreeDelegate(this));
   this->pIbtsView->getSettingsTreeView()->expandAll();

   this->getPGuiModel()->pPortfolioConfigModel()->setBrokerDataProvider(this->m_pDataProvider);
   this->getPGuiModel()->pPortfolioConfigModel()->setupModelData();

   m_pSystemTreeModel = new SystemTreeModel(this);
   m_pSystemTreeDelegate = new SystemTreeDelegate(this);
   m_pSystemTreeModel->setBackend(m_backend);
   m_pSystemTreeModel->setRoot(this->getPGuiModel()->dataRoot());

   QTreeView* treeView = this->pIbtsView->getPortfolioConfigTreeView();
   treeView->setModel(m_pSystemTreeModel);
   treeView->setItemDelegate(m_pSystemTreeDelegate);
   treeView->setHeaderHidden(false);
   treeView->setAlternatingRowColors(true);
   treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
   treeView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
   UiLayoutDefaults::applySystemTreeColumnDefaults(treeView);

   if (m_stratMgmtCoord)
       m_stratMgmtCoord->refreshCatalog();
}


CIBTradeSystemView *CPresenter::getPIbtsView() const
{
    return pIbtsView;
}

// ---------------------------------------------------------------------------
// Tree selection routing (remains in CPresenter as the thin router)
// ---------------------------------------------------------------------------

void CPresenter::onTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    if (!current.isValid()) return;

    if (!m_pSystemTreeModel) {
        if (m_pDiagramWidget) m_pDiagramWidget->clear();
        return;
    }

    if (m_pSystemTreeModel->isVirtualCategory(current)) {
        CGenericModelApi* parentStrategy = m_pSystemTreeModel->parentStrategyOf(current);
        if (parentStrategy && pIbtsView->contextWorkspace()) {
            CGenericModelApi* parentModel = parentStrategy->getParentModel();
            pIbtsView->contextWorkspace()->showStrategyWorkspace(parentStrategy, parentModel);
            pIbtsView->contextWorkspace()->restoreStrategyProperties();
        }
        return;
    }

    if (m_pSystemTreeModel->isVirtualBlock(current)) {
        CGenericModelApi* parentStrategy = m_pSystemTreeModel->parentStrategyOf(current);
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(parentStrategy);

        if (adapter && m_pDiagramWidget)
            m_pDiagramWidget->setPipelineConfig(adapter->pipelineConfig());

        if (adapter && pIbtsView->contextWorkspace()) {
            QString blockId  = m_pSystemTreeModel->virtualBlockId(current);
            QString category = m_pSystemTreeModel->virtualCategory(current);

            const QString jsonKey = QString(Pipeline::categoryKey(category));
            const bool isArray    = Pipeline::categoryIsArray(category);

            int arrayIndex = 0;
            if (isArray) {
                QJsonArray arr = adapter->pipelineConfig().value(jsonKey).toArray();
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    QString id = obj.value(Pipeline::Key::BlockId).toString();
                    if (id.isEmpty()) id = obj.value("type").toString();
                    if (id == blockId) { arrayIndex = i; break; }
                }
            }

            CGenericModelApi* parentModel = parentStrategy->getParentModel();
            pIbtsView->contextWorkspace()->showStrategyWorkspace(parentStrategy, parentModel);
            pIbtsView->contextWorkspace()->showBlockInProperties(category, jsonKey, isArray, arrayIndex);
        }
        return;
    }

    CGenericModelApi* model = m_pSystemTreeModel->modelAt(current);
    if (!model) {
        if (m_pDiagramWidget) m_pDiagramWidget->clear();
        if (pIbtsView->contextWorkspace()) pIbtsView->contextWorkspace()->showEmpty();
        return;
    }

    ModelType mt = model->modelType();

    if (m_pDiagramWidget) {
        if (mt == ModelType::ACCOUNT) {
            QStringList portfolioNames;
            for (const auto& p : model->getModels())
                portfolioNames.append(p->getName());
            m_pDiagramWidget->setAccountView(model->getName(), portfolioNames);
        }
        else if (mt == ModelType::PORTFOLIO) {
            QVector<StrategyDiagramInfo> strategies;
            for (const auto& s : model->getModels()) {
                StrategyDiagramInfo info;
                info.name = s->getName();
                auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(s.data());
                if (adapter) info.pipelineConfig = adapter->pipelineConfig();
                strategies.append(info);
            }
            m_pDiagramWidget->setPortfolioView(model->getName(), strategies);
        }
        else {
            auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(model);
            if (adapter)
                m_pDiagramWidget->setPipelineConfig(adapter->pipelineConfig());
            else
                m_pDiagramWidget->clear();
        }
    }

    if (pIbtsView->contextWorkspace()) {
        bool isStrategy = (mt == ModelType::STRATEGY ||
                           mt == ModelType::STRATEGY_BASIC_TEST ||
                           mt == ModelType::STRATEGY_MA ||
                           mt == ModelType::STRATEGY_MOMENTUM ||
                           mt == ModelType::STRATEGY_PIPELINE);
        if (mt == ModelType::ACCOUNT) {
            pIbtsView->contextWorkspace()->showAccountWorkspace(model);
        } else if (mt == ModelType::PORTFOLIO) {
            CGenericModelApi* parentModel = m_pSystemTreeModel->parentModelAt(current);
            pIbtsView->contextWorkspace()->showPortfolioWorkspace(model, parentModel);
        } else if (isStrategy) {
            CGenericModelApi* parentModel = m_pSystemTreeModel->parentModelAt(current);
            pIbtsView->contextWorkspace()->showStrategyWorkspace(model, parentModel);
        } else {
            pIbtsView->contextWorkspace()->showEmpty();
        }
    }
}
