#include "cpresenter.h"
#include "ISystemBackend.h"
#include "ReqManager.h"
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
#include "Pipeline/BlockRegistry.h"
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
#include "Backtest/BacktestController.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"
#include "DB/dbquery.h"
#include <QtSql/QSqlDatabase>
#include <QJsonDocument>


CPresenter::CPresenter(QObject *parent)
	: QObject(parent)
    , m_pLog(LOGGER)
    //, m_DataProvider()
    , m_pDataProvider(QSharedPointer<CBrokerDataProvider>::create())
    , pIbtsView(nullptr)
    , pGuiModel(nullptr)
    , threadIBClient(new QThread)
    , workerIBClient(new IBWorker::Worker(parent, *m_pDataProvider.data()))
    , threadAlfaTime(new QThread)
    , workerAlfaTime(new AlphaModGetTime(parent, *m_pDataProvider.data()))
    , pAboutDlgPresenter(new AboutDlgPresener(parent))
    // , pPairTradingPresenter(new PairTradingPresenter(parent, *m_pDataProvider.data()))
    // , pAutoDeltaAligPresenter(new AutoDeltaAligPresenter(parent, *m_pDataProvider.data()))
    // , pDBStorePresenter(new DBStorePresenter(parent, *m_pDataProvider.data()))
{
	
    //

    QSharedPointer<IBComClientImpl> pClient = QSharedPointer<IBComClientImpl>::create();

   //Define Data Provider
    m_pDataProvider->setClien(pClient);

    // Pipeline integration: create MarketDataRouter and wire to IBComClientImpl
    m_pMarketDataRouter = new IBComm::MarketDataRouter(this);
    pClient->setMarketDataRouter(m_pMarketDataRouter);

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
	//MAP GUI Signals/Slots
	//Click connect button
    QObject::connect(pIbtsView->getUi().actionConnect, &QAction::triggered, this, &CPresenter::onClickMyButton);

    // //click Pair Trader button
 //    QObject::connect(pIbtsView->getUi().actionPair_Trader, &QAction::triggered, this, &CPresenter::onClickPairTraderButton);

 //    //click Auto Delta button
 //    QObject::connect(pIbtsView->getUi().actionAuto_Delta, &QAction::triggered, this, &CPresenter::onClickAutoDeltaButton);

 //    //click DBStore button
 //    QObject::connect(pIbtsView->getUi().actionDBStore, &QAction::triggered, this, &CPresenter::onClickDBStoreButton);



	//Click received Time button
	QObject::connect(workerAlfaTime, SIGNAL(signalTimeReceived(long)), pIbtsView, SLOT(slotOnTimeReceived(long)));
	//Received info about connection button state
	QObject::connect(this, SIGNAL(signalClickConnect(bool)), pIbtsView, SLOT(slotRecvConnectButtonState(bool)));

	//Logger connection to main gui
	QObject::connect(&LOGGER, SIGNAL(signalAddLogMsg(QString)), pIbtsView, SLOT(slotOnLogMsgReceived(QString)));

    // QObject::connect(workerAlfaTime, &AlphaModGetTime::signalPlanResetSubscribtion,
    //                  pDBStorePresenter.data(), &DBStorePresenter::signalResetSubscribtion, Qt::QueuedConnection);

    // QObject::connect(workerAlfaTime, &AlphaModGetTime::signalPlanResetSubscribtion,
    //                  pAutoDeltaAligPresenter->getPM().data(), &CProcessingBase::signalRestartSubscription, Qt::QueuedConnection);


    QTreeView * pTreeView = this->pIbtsView->getPortfolioConfigTreeView();
    CPortfolioConfigModel *pPConfigModel = this->getPGuiModel()->pPortfolioConfigModel();

    // Context menu driven by SystemTreeModel, mutations routed through ISystemBackend
    auto* sysModel = m_pSystemTreeModel;
    auto* backend = m_backend;

    QObject::connect(pTreeView, &QTreeView::customContextMenuRequested,
                     this->pIbtsView, [pTreeView, sysModel, pPConfigModel, backend, this](const QPoint& pos) {
        QModelIndex index = pTreeView->indexAt(pos);

        QMenu menu;

        QAction* addAccount = menu.addAction("Add New Account");

        QAction* addPortfolio    = nullptr;
        QAction* addStrategy     = nullptr;
        QAction* useExistingStrat = nullptr;
        QAction* removeNode      = nullptr;
        QAction* openBacktest = nullptr;
        QAction* addSelectionBlock  = nullptr;
        QAction* addAlphaBlock      = nullptr;
        QAction* addRebalanceBlock  = nullptr;
        QAction* addRiskBlock       = nullptr;
        QAction* addExecutionBlock  = nullptr;
        QAction* removeBlock        = nullptr;

        CGenericModelApi* clickedModel = nullptr;
        ModelType clickedType = ModelType::ROOT;
        bool isVirtual = false;

        if (index.isValid() && sysModel) {
            isVirtual = sysModel->isVirtualBlock(index);
            clickedModel = sysModel->modelAt(index);
            if (clickedModel)
                clickedType = clickedModel->modelType();

            if (clickedType == ModelType::ACCOUNT) {
                addPortfolio = menu.addAction("Add New Portfolio");
            }
            if (clickedType == ModelType::PORTFOLIO) {
                addStrategy       = menu.addAction("Add New Strategy");
                useExistingStrat  = menu.addAction("Use Existing Strategy...");
            }

            bool isStrategy = (clickedType == ModelType::STRATEGY ||
                               clickedType == ModelType::STRATEGY_BASIC_TEST ||
                               clickedType == ModelType::STRATEGY_MA ||
                               clickedType == ModelType::STRATEGY_MOMENTUM ||
                               clickedType == ModelType::STRATEGY_PIPELINE);

            if (isStrategy) {
                openBacktest = menu.addAction("Open in Backtest Workspace");
            }

            bool isPipelineStrategy = (clickedType == ModelType::STRATEGY_PIPELINE);
            if (isPipelineStrategy) {
                menu.addSeparator();
                addSelectionBlock  = menu.addAction("Add Selection Model");
                addAlphaBlock      = menu.addAction("Add Alpha Model");
                addRebalanceBlock  = menu.addAction("Add Rebalance Model");
                addRiskBlock       = menu.addAction("Add Risk Model");
                addExecutionBlock  = menu.addAction("Add Execution Model");
            }

            if (isVirtual) {
                menu.addSeparator();
                removeBlock = menu.addAction("Remove Block");
            } else if (clickedModel) {
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
        else if (chosen == addStrategy && clickedModel && backend) {
            QString portfolioId = clickedModel->getId().toString(QUuid::WithoutBraces);
            backend->createStrategy(portfolioId, ModelType::STRATEGY_PIPELINE);
            rebuildTree();
        }
        else if (chosen == useExistingStrat && clickedModel && backend) {
            // Show a picker listing published versions from the catalog
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
        else if ((chosen == addSelectionBlock || chosen == addAlphaBlock ||
                  chosen == addRebalanceBlock || chosen == addRiskBlock ||
                  chosen == addExecutionBlock) && clickedModel && backend) {
            QString category;

            if (chosen == addSelectionBlock)      { category = Pipeline::Category::Selection; }
            else if (chosen == addAlphaBlock)     { category = Pipeline::Category::Alpha;     }
            else if (chosen == addRebalanceBlock) { category = Pipeline::Category::Rebalance; }
            else if (chosen == addRiskBlock)      { category = Pipeline::Category::Risk;      }
            else if (chosen == addExecutionBlock) { category = Pipeline::Category::Execution; }

            auto blocks = Pipeline::BlockRegistry::instance().blocksByCategory(category);
            if (!blocks.isEmpty()) {
                QStringList displayNames, blockIds;
                for (const auto& desc : blocks) {
                    QString label = desc.name;
                    if (!desc.description.isEmpty())
                        label += " -- " + desc.description;
                    displayNames.append(label);
                    blockIds.append(desc.id);
                }

                bool ok = false;
                QString chosenBlock = QInputDialog::getItem(
                    pTreeView, QString("Select %1 Block").arg(category),
                    QString("Available %1 blocks:").arg(category.toLower()),
                    displayNames, 0, false, &ok);

                if (ok && !chosenBlock.isEmpty()) {
                    int idx = displayNames.indexOf(chosenBlock);
                    if (idx >= 0) {
                        QString blockId = blockIds.at(idx);
                        auto descResult = Pipeline::BlockRegistry::instance().descriptor(blockId);
                        QJsonObject defaultCfg = descResult ? descResult.value().defaultConfig : QJsonObject();

                        QString strategyId = clickedModel->getId().toString(QUuid::WithoutBraces);
                        backend->addBlock(strategyId, category, blockId, defaultCfg);

                        sysModel->rebuildFromRoot();
                        pTreeView->expandAll();

                        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(clickedModel);
                        if (adapter)
                            emit pPConfigModel->pipelineConfigChanged(adapter->pipelineConfig());
                    }
                }
            }
        }
        else if (chosen == removeBlock && isVirtual && backend) {
            CGenericModelApi* parentStrategy = sysModel->parentStrategyOf(index);
            if (parentStrategy) {
                QString strategyId = parentStrategy->getId().toString(QUuid::WithoutBraces);
                QString category = sysModel->virtualCategory(index);
                QString blockId  = sysModel->virtualBlockId(index);

                auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(parentStrategy);
                if (adapter) {
                    const QString jsonKey = QString(Pipeline::categoryKey(category));
                    const bool isArray    = Pipeline::categoryIsArray(category);

                    if (isArray) {
                        QJsonArray arr = adapter->pipelineConfig().value(jsonKey).toArray();
                        int blockIndex = -1;
                        for (int i = 0; i < arr.size(); ++i) {
                            QJsonObject obj = arr[i].toObject();
                            if (obj.value(Pipeline::Key::BlockId).toString() == blockId) {
                                blockIndex = i;
                                break;
                            }
                        }
                        if (blockIndex >= 0)
                            backend->removeBlock(strategyId, category, blockIndex);
                    } else {
                        backend->removeBlock(strategyId, category, 0);
                    }

                    rebuildTree();
                }
            }
        }
        else if (chosen == removeNode && clickedModel && backend) {
            QString uuid = clickedModel->getId().toString(QUuid::WithoutBraces);
            backend->removeNode(uuid);
            rebuildTree();
        }
    });

    m_pDiagramWidget = new PipelineDiagramWidget();
    m_pDiagramDock = new QDockWidget("Diagram View", this->pIbtsView);
    m_pDiagramDock->setWidget(m_pDiagramWidget);
    m_pDiagramDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    this->pIbtsView->addDockWidget(Qt::BottomDockWidgetArea, m_pDiagramDock);

    // ── Backtest Workspace (embedded in Backtest tab, not as a floating dock) ──
    //
    // The BacktestWorkspaceDock is created and its inner widget is extracted so it
    // can be placed inside the "Backtest" tab's right pane.  The dock wrapper itself
    // is kept alive as the parent object for signal wiring purposes but is never
    // shown as a floating window.
    m_pBacktestDock = new BacktestUI::BacktestWorkspaceDock(this->pIbtsView);
    m_pBacktestDock->hide();   // dock frame hidden — content embedded in tab

    // Inject the dock's inner widget into the pre-built right pane placeholder.
    auto* backtestRightPane = this->pIbtsView->findChild<QWidget*>(
        QStringLiteral("BacktestRightPane"));
    if (backtestRightPane && m_pBacktestDock->widget()) {
        QLayout* paneLayout = backtestRightPane->layout();
        // Remove the placeholder label
        while (QLayoutItem* item = paneLayout->takeAt(0)) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        // Re-parent the dock's content widget directly into the pane
        QWidget* btContent = m_pBacktestDock->widget();
        btContent->setParent(backtestRightPane);
        paneLayout->addWidget(btContent);
    }

    // Open in Backtest Workspace: context menu → presenter → dock
    QObject::connect(pPConfigModel, &CPortfolioConfigModel::openInBacktestWorkspace,
                     this, &CPresenter::onOpenInBacktestWorkspace,
                     Qt::QueuedConnection);

    // Dock → presenter wiring
    QObject::connect(m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::runRequested,
                     this, [this](const Backtest::BacktestRunConfig& config) {
        if (!m_pBacktestController) return;
        m_pBacktestDock->setRunning(true);
        m_pBacktestController->start(config);
    });

    QObject::connect(m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::loadRunRequested,
                     this, &CPresenter::onLoadRun);

    // ── BacktestStrategySelector wiring ──────────────────────────────────────
    auto* selector = this->pIbtsView->backtestStrategySelector();
    if (selector) {
        // Double-click / Open button in selector → open in backtest panel
        connect(selector, &BacktestUI::BacktestStrategySelector::strategySelected,
                this, [this](const QString& strategyId,
                              const QString& displayName,
                              const QString& portfolioPath,
                              const QJsonObject& pipelineConfig) {
            onOpenInBacktestWorkspace(strategyId, displayName, portfolioPath, pipelineConfig);
        });

        connect(selector, &BacktestUI::BacktestStrategySelector::catalogVersionSelected,
                this, &CPresenter::openCatalogVersionInBacktest);

        connect(selector, &BacktestUI::BacktestStrategySelector::refreshRequested,
                this, &CPresenter::refreshBacktestStrategies);
    }

    // Populate on first show (when Backtest tab is activated)
    if (auto* tabs = this->pIbtsView->mainTabWidget()) {
        connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
            if (index == 1)  // Backtest tab
                refreshBacktestStrategies();
            else if (index == 2)  // Strategy Management tab
                refreshStrategyCatalog();
        });
    }

    // ── Strategy Management panel wiring ─────────────────────────────────
    if (auto* smPanel = this->pIbtsView->strategyManagementPanel()) {
        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::strategySelected,
                this, [this](const QString& strategyId) {
            if (!m_backend) return;
            QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
            QJsonArray versions = m_backend->listStrategyVersions(strategyId);
            pIbtsView->strategyManagementPanel()->showStrategyDetail(entry, versions);
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::newStrategyRequested,
                this, [this]() {
            if (!m_backend) return;
            QString name = QInputDialog::getText(pIbtsView, QStringLiteral("New Strategy"),
                                                  QStringLiteral("Strategy name:"));
            if (name.isEmpty()) return;
            m_backend->createStrategyCatalogEntry(name, static_cast<int>(ModelType::STRATEGY_PIPELINE));
            refreshStrategyCatalog();
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::metadataChanged,
                this, [this](const QString& sid, const QString& name,
                             const QString& desc, const QString& tags,
                             const QString& state) {
            if (!m_backend) return;
            m_backend->updateStrategyCatalogMeta(sid, name, desc, tags, state);
            refreshStrategyCatalog();
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::newVersionRequested,
                this, [this](const QString& strategyId) {
            if (!m_backend) return;
            auto latestVers = m_backend->listStrategyVersions(strategyId);
            QJsonObject latestCfg;
            if (!latestVers.isEmpty())
                latestCfg = QJsonDocument::fromJson(
                    latestVers.last().toObject()["configJson"].toString().toUtf8()).object();
            QString notes = QInputDialog::getText(pIbtsView,
                QStringLiteral("New Version"),
                QStringLiteral("Notes for this version:"));
            m_backend->createStrategyVersion(strategyId, latestCfg, notes);
            refreshStrategyCatalog();
            // Refresh detail for the same strategy
            QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
            QJsonArray versions = m_backend->listStrategyVersions(strategyId);
            pIbtsView->strategyManagementPanel()->showStrategyDetail(entry, versions);
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::publishRequested,
                this, [this](const QString& strategyId, const QString& versionId) {
            if (!m_backend) return;
            m_backend->publishVersion(versionId);
            QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
            QJsonArray versions = m_backend->listStrategyVersions(strategyId);
            pIbtsView->strategyManagementPanel()->showStrategyDetail(entry, versions);
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::archiveRequested,
                this, [this](const QString& strategyId) {
            if (!m_backend) return;
            m_backend->archiveStrategyCatalogEntry(strategyId);
            refreshStrategyCatalog();
            pIbtsView->strategyManagementPanel()->detailPanel()->clear();
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::useInLiveRequested,
                this, [this](const QString& catalogStrategyId, const QString& catalogVersionId) {
            if (!m_backend) return;
            CGenericModelApi* root = m_backend->dataRoot();
            if (!root) return;

            QStringList portfolioLabels;
            QStringList portfolioIds;
            for (auto& acct : root->getModels())
                for (auto& port : acct->getModels()) {
                    QString pid = port->getId().toString(QUuid::WithoutBraces);
                    portfolioLabels << acct->getName() + QStringLiteral(" / ") + port->getName();
                    portfolioIds << pid;
                }

            if (portfolioIds.isEmpty()) {
                QMessageBox::warning(pIbtsView, QStringLiteral("No Portfolios"),
                    QStringLiteral("Create an account and portfolio first."));
                return;
            }

            bool ok = false;
            QString chosen = QInputDialog::getItem(
                pIbtsView, QStringLiteral("Select Portfolio"),
                QStringLiteral("Deploy strategy to portfolio:"),
                portfolioLabels, 0, false, &ok);
            if (!ok) return;

            int idx = portfolioLabels.indexOf(chosen);
            if (idx < 0) return;
            QString portfolioId = portfolioIds.at(idx);

            QString nodeId = m_backend->createLiveNodeForExistingCatalog(
                portfolioId, ModelType::STRATEGY_PIPELINE,
                catalogStrategyId, catalogVersionId);
            if (nodeId.isEmpty()) return;

            // Refresh the live tree
            if (pPConfigModel) {
                pPConfigModel->setData(root);
                pIbtsView->slotUpdateTreeViewAll();
            }
        });

        connect(smPanel, &StrategyMgmt::StrategyManagementPanel::openInBacktestRequested,
                this, &CPresenter::openCatalogVersionInBacktest);
    }

    // Backend catalog signals → refresh
    if (m_backend) {
        connect(m_backend, &ISystemBackend::strategyCatalogChanged,
                this, [this](const QString&) { refreshStrategyCatalog(); });
        connect(m_backend, &ISystemBackend::strategyVersionCreated,
                this, [this](const QString&, const QString&) { refreshStrategyCatalog(); });
        connect(m_backend, &ISystemBackend::nodeConfigDiverged,
                this, [this](const QString& nodeId) {
            if (!m_backend || !m_backend->isBrokerConnected()) return;
            CGenericModelApi* root = m_backend->dataRoot();
            if (!root) return;

            QString nodeName = nodeId;
            auto findName = [&]() -> bool {
                for (auto& acct : root->getModels())
                    for (auto& port : acct->getModels())
                        for (auto& strat : port->getModels())
                            if (strat->getId().toString(QUuid::WithoutBraces) == nodeId) {
                                nodeName = strat->getName();
                                return true;
                            }
                return false;
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

    // Reconnect selection changed to account for the new model that was set in setPGuiModel()
    if (pTreeView->selectionModel()) {
        QObject::connect(pTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
                         this, &CPresenter::onTreeSelectionChanged);
    }

    QObject::connect(pPConfigModel, &CPortfolioConfigModel::pipelineConfigChanged,
                     m_pDiagramWidget, &PipelineDiagramWidget::updateFromConfig);

    QObject::connect(pPConfigModel, SIGNAL(signalUpdateData(QModelIndex)), this->pIbtsView, SLOT(slotUpdateTreeView(QModelIndex)));
    QObject::connect(pPConfigModel, &CPortfolioConfigModel::signalUpdateDataAll, this->pIbtsView, &CIBTradeSystemView::slotUpdateTreeViewAll, Qt::QueuedConnection);


    this->pIbtsView->mapSignals();

    // Wire AlertService to GlobalStatusBar (D2: owned, not singleton)
    if (pGuiModel && pGuiModel->alertService() && pIbtsView->globalStatusBar()) {
        QObject::connect(pGuiModel->alertService(), &AlertService::countChanged,
                         pIbtsView->globalStatusBar(), &GlobalStatusBar::setAlertCount);
    }

    // Wire global actions from GlobalStatusBar
    if (pIbtsView->globalStatusBar()) {
        QObject::connect(pIbtsView->globalStatusBar(), &GlobalStatusBar::reconnectClicked,
                         this, &CPresenter::onClickMyButton);
    }



	/////////---------------
	//ReqManager temp;

	//temp.debugPrintList();
	//temp.addReqIds(1, RT_REQ_REL_DATA);
	//temp.debugPrintList();
	//temp.addReqIds(1, RT_REQ_REL_DATA);
	//temp.debugPrintList();
	//temp.addReqIds(1, RT_REQ_CUR_TIME);
	//temp.debugPrintList();
	//temp.addReqIds(2, RT_REQ_CUR_TIME);
	//temp.addReqIds(3, RT_REQ_CUR_TIME);
	//temp.debugPrintList();
	//temp.removeReqIds(2, RT_REQ_CUR_TIME);
	//temp.removeReqIds(2, RT_REQ_CUR_TIME);
	//temp.debugPrintList();
	//temp.isPresent(1, RT_REQ_REL_DATA);
	//temp.isPresent(1, RT_TICK_PRICE);
	//temp.isPresent(1, RT_MKT_DEPTH);
    // pPairTradingPresenter->init();
    // pAutoDeltaAligPresenter->init();
    // pDBStorePresenter->init();

}

void CPresenter::addView(CIBTradeSystemView * mw)
{
	this->pIbtsView = mw;

}



/*! 
*  Connect button processing
*/
void CPresenter::onClickMyButton()
{
    static bool buttonState = false;
    if ((false == buttonState) && (!m_pDataProvider->getClien()->isConnectedAPI()))
    {
        // Before connecting, check all live strategy nodes for config divergence
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
    // if (!pPairTradingPresenter.isNull())
    // {
    // 	pPairTradingPresenter->showDlg();
    // }
}

void CPresenter::onClickAutoDeltaButton()
{
    // if (!pAutoDeltaAligPresenter.isNull())
    // {
    //     pAutoDeltaAligPresenter->showDlg();
    // }
}

void CPresenter::onClickDBStoreButton()
{
    // if (!pDBStorePresenter.isNull())
    // {
    //     pDBStorePresenter->showDlg();
    // }
}

QSharedPointer<CBrokerDataProvider> CPresenter::getDataProvider() const
{
    return m_pDataProvider;
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
   this->pIbtsView->getSettingsTreeView()->expandAll();

   this->getPGuiModel()->pPortfolioConfigModel()->setBrokerDataProvider(this->m_pDataProvider);
   this->getPGuiModel()->pPortfolioConfigModel()->setupModelData();

   // Set up SystemTreeModel (operations console tree replacing old config tree)
   m_pSystemTreeModel = new SystemTreeModel(this);
   m_pSystemTreeDelegate = new SystemTreeDelegate(this);
   m_pSystemTreeModel->setBackend(m_backend);
   m_pSystemTreeModel->setRoot(this->getPGuiModel()->dataRoot());

   QTreeView* treeView = this->pIbtsView->getPortfolioConfigTreeView();
   treeView->setModel(m_pSystemTreeModel);
   treeView->setItemDelegate(m_pSystemTreeDelegate);
   treeView->setHeaderHidden(false);
   treeView->setAlternatingRowColors(true);
   treeView->setColumnWidth(SystemTreeModel::ColName, 200);
   treeView->setColumnWidth(SystemTreeModel::ColEnabled, 50);
   treeView->setColumnWidth(SystemTreeModel::ColStatus, 110);
   treeView->setColumnWidth(SystemTreeModel::ColPnL, 90);
   treeView->header()->setStretchLastSection(false);
   treeView->header()->setSectionResizeMode(SystemTreeModel::ColName, QHeaderView::Stretch);
   treeView->expandAll();

   refreshStrategyCatalog();
}


CIBTradeSystemView *CPresenter::getPIbtsView() const
{
    return pIbtsView;
}

// ---------------------------------------------------------------------------
// Backtest workspace slots
// ---------------------------------------------------------------------------

void CPresenter::onOpenInBacktestWorkspace(const QString& strategyId,
                                            const QString& displayName,
                                            const QString& portfolioPath,
                                            const QJsonObject& pipelineConfig)
{
    if (!m_pBacktestDock) return;

    QString strategyDefId;
    QString catalogVersionId;
    int     strategyVersion = 1;
    if (m_backend) {
        QJsonObject defJson = m_backend->strategyDefinitionForNode(strategyId);
        if (!defJson.isEmpty()) {
            strategyDefId    = defJson.value("strategyDefId").toString();
            strategyVersion  = defJson.value("version").toInt(1);
            catalogVersionId = defJson.value("versionId").toString();
        }
    }

    // Re-create the controller for this strategy (one controller per active run).
    // Any previous run is abandoned (no cooperative cancel in v1 — the old thread
    // will finish on its own and its signals will be ignored after reconnect).
    delete m_pBacktestController;
    m_pBacktestController = new Backtest::BacktestController(
        QStringLiteral("myLocalDb.sqlite"), nullptr, this);

    connect(m_pBacktestController, &Backtest::BacktestController::progressChanged,
            m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::setProgress);
    connect(m_pBacktestController, &Backtest::BacktestController::statusChanged,
            m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::setStatus);
    connect(m_pBacktestController, &Backtest::BacktestController::finished,
            this, &CPresenter::onBacktestFinished);
    connect(m_pBacktestController, &Backtest::BacktestController::failed,
            this, &CPresenter::onBacktestFailed);

    // Determine profile from the pipeline config JSON
    Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            pipelineConfig.value("backtestProfile").toObject());

    const QString pipelineConfigJson = QString::fromUtf8(
        QJsonDocument(pipelineConfig).toJson(QJsonDocument::Compact));

    m_pBacktestDock->selectStrategy(strategyId, displayName, portfolioPath,
                                     profile, pipelineConfigJson,
                                     strategyDefId, strategyVersion,
                                     catalogVersionId);

    // Switch to the Backtest tab and highlight the strategy in the selector.
    pIbtsView->switchToBacktestTab();
    if (auto* sel = pIbtsView->backtestStrategySelector())
        sel->highlightStrategy(strategyId);

    // Fetch run history from DB.
    // When a definition ID is known, query by definition (includes runs from all live
    // node UUIDs that were bound to the same definition — preserves history after
    // remove + re-add of a strategy). Fall back to legacy node-UUID query otherwise.
    if (m_pBacktestController) {
        const QString conn = m_pBacktestController->dbConnectionName();

        auto populateHistory = [&](QSqlQuery q) {
            QList<DbBacktestRunSummary> summaries;
            if (q.exec()) {
                while (q.next()) {
                    DbBacktestRunSummary s;
                    s.runId        = q.value("runId").toString();
                    s.strategyId   = q.value("strategyId").toString();
                    s.symbols      = q.value("symbols").toString();
                    s.startDate    = q.value("startDate").toString();
                    s.endDate      = q.value("endDate").toString();
                    s.status       = q.value("status").toString();
                    s.dataSourceId = q.value("dataSourceId").toString();
                    s.createdAt    = q.value("createdAt").toString();
                    s.totalReturn  = q.value("totalReturn").toDouble();
                    s.sharpeRatio  = q.value("sharpeRatio").toDouble();
                    // Scope fields (may be empty for old runs)
                    s.strategyDefId   = q.value("strategyDefId").toString();
                    s.scopeType       = q.value("scopeType").toString();
                    s.scopeRefId      = q.value("scopeRefId").toString();
                    s.strategyVersion = q.value("strategyVersion").isNull()
                                            ? 1 : q.value("strategyVersion").toInt();
                    summaries.append(s);
                }
            }
            m_pBacktestDock->setRunHistory(summaries);
        };

        if (!strategyDefId.isEmpty())
            populateHistory(query_fetchRunsForDefinition(strategyDefId, conn));
        else
            populateHistory(query_fetchRunsForStrategy(strategyId, conn));
    }
}

void CPresenter::refreshBacktestStrategies()
{
    auto* selector = pIbtsView ? pIbtsView->backtestStrategySelector() : nullptr;
    if (!selector || !m_backend) return;

    CGenericModelApi* root = m_backend->dataRoot();
    if (!root) {
        selector->populate({});
        return;
    }

    QList<BacktestUI::StrategyListItem> items;

    // Walk Account → Portfolio → Strategy tree
    for (auto& accountPtr : root->getModels()) {
        CGenericModelApi* account = accountPtr.data();
        const QString accountName = account->getName();

        for (auto& portfolioPtr : account->getModels()) {
            CGenericModelApi* portfolio = portfolioPtr.data();
            const QString portfolioName = portfolio->getName();

            for (auto& stratPtr : portfolio->getModels()) {
                CGenericModelApi* strat = stratPtr.data();
                if (strat->modelType() != ModelType::STRATEGY_PIPELINE) continue;

                const QString stratId = strat->getId().toString(QUuid::WithoutBraces);

                BacktestUI::StrategyListItem item;
                item.strategyId   = stratId;
                item.name         = strat->getName();
                item.accountName  = accountName;
                item.portfolioName = portfolioName;
                item.pipelineConfig = m_backend->pipelineConfig(stratId);

                // Look up the catalog definition for version badge
                QJsonObject defJson = m_backend->strategyDefinitionForNode(stratId);
                if (!defJson.isEmpty()) {
                    item.strategyDefId      = defJson.value("strategyDefId").toString();
                    item.version            = defJson.value("version").toInt(1);
                    item.catalogVersionId   = defJson.value("versionId").toString();
                }

                items.append(item);
            }
        }
    }

    selector->populate(items);

    // Also populate catalog strategies for the version picker
    QList<BacktestUI::CatalogVersionItem> catalogItems;
    QJsonArray catalog = m_backend->listStrategyCatalog(false);
    for (const QJsonValue& c : catalog) {
        QJsonObject sObj = c.toObject();
        QString stratId   = sObj.value("strategyId").toString();
        QString stratName = sObj.value("name").toString();

        QJsonArray versions = m_backend->listStrategyVersions(stratId);
        for (const QJsonValue& v : versions) {
            QJsonObject vObj = v.toObject();
            BacktestUI::CatalogVersionItem ci;
            ci.strategyId    = stratId;
            ci.strategyName  = stratName;
            ci.versionId     = vObj.value("versionId").toString();
            ci.versionNumber = vObj.value("versionNumber").toInt(1);
            ci.configJson    = vObj.value("configJson").toString();
            ci.isPublished   = vObj.value("isPublished").toBool();
            catalogItems.append(ci);
        }
    }
    selector->populateCatalog(catalogItems);
}

void CPresenter::openCatalogVersionInBacktest(const QString& catalogStrategyId,
                                               const QString& catalogVersionId)
{
    if (!m_backend || !m_pBacktestDock) return;

    QJsonArray versions = m_backend->listStrategyVersions(catalogStrategyId);
    QJsonObject verJson;
    for (const QJsonValue& v : versions) {
        QJsonObject obj = v.toObject();
        if (obj.value("versionId").toString() == catalogVersionId) {
            verJson = obj;
            break;
        }
    }
    if (verJson.isEmpty()) return;

    QJsonArray catalog = m_backend->listStrategyCatalog(true);
    QString displayName = QStringLiteral("Strategy");
    for (const QJsonValue& c : catalog) {
        QJsonObject obj = c.toObject();
        if (obj.value("strategyId").toString() == catalogStrategyId) {
            displayName = obj.value("name").toString(displayName);
            break;
        }
    }

    int versionNumber = verJson.value("versionNumber").toInt(1);
    QString configJson = verJson.value("configJson").toString();

    delete m_pBacktestController;
    m_pBacktestController = new Backtest::BacktestController(
        QStringLiteral("myLocalDb.sqlite"), nullptr, this);

    connect(m_pBacktestController, &Backtest::BacktestController::progressChanged,
            m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::setProgress);
    connect(m_pBacktestController, &Backtest::BacktestController::statusChanged,
            m_pBacktestDock, &BacktestUI::BacktestWorkspaceDock::setStatus);
    connect(m_pBacktestController, &Backtest::BacktestController::finished,
            this, &CPresenter::onBacktestFinished);
    connect(m_pBacktestController, &Backtest::BacktestController::failed,
            this, &CPresenter::onBacktestFailed);

    QJsonDocument configDoc = QJsonDocument::fromJson(configJson.toUtf8());
    QJsonObject configObj = configDoc.object();
    Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            configObj.value("backtestProfile").toObject());

    m_pBacktestDock->selectStrategy(
        QString(),
        displayName + QStringLiteral(" v") + QString::number(versionNumber),
        QString(),
        profile,
        configJson,
        catalogStrategyId,
        versionNumber,
        catalogVersionId);

    pIbtsView->switchToBacktestTab();
}

void CPresenter::refreshStrategyCatalog()
{
    auto* smPanel = pIbtsView ? pIbtsView->strategyManagementPanel() : nullptr;
    if (!smPanel || !m_backend) return;

    QJsonArray entries = m_backend->listStrategyCatalog(false);

    QMap<QString, int> versionCounts;
    for (const auto& e : entries) {
        QString sid = e.toObject().value("strategyId").toString();
        QJsonArray versions = m_backend->listStrategyVersions(sid);
        versionCounts[sid] = versions.size();
    }

    smPanel->populateCatalog(entries, versionCounts);
}

void CPresenter::onLoadRun(const QString& runId) {
    if (!m_pBacktestDock || !m_pBacktestController) return;

    const QString conn = m_pBacktestController->dbConnectionName();
    QSqlDatabase db = QSqlDatabase::database(conn);
    if (!db.isOpen()) return;

    Backtest::BacktestLoadedRun loaded;

    // Run record
    {
        auto q = query_fetchBacktestRun(runId, conn);
        if (q.exec() && q.next()) {
            loaded.record.runId               = q.value("runId").toString();
            loaded.record.strategyId          = q.value("strategyId").toString();
            loaded.record.strategyDisplayName = q.value("strategyDisplayName").toString();
            loaded.record.portfolioPath       = q.value("portfolioPath").toString();
            loaded.record.symbols             = q.value("symbols").toString();
            loaded.record.startDate           = q.value("startDate").toString();
            loaded.record.endDate             = q.value("endDate").toString();
            loaded.record.status              = q.value("status").toString();
        }
    }

    // Metrics → result fields
    {
        auto q = query_fetchBacktestMetrics(runId, conn);
        if (q.exec() && q.next()) {
            loaded.result.totalReturn      = q.value("totalReturn").toDouble();
            loaded.result.annualizedReturn = q.value("annualizedReturn").toDouble();
            loaded.result.sharpeRatio      = q.value("sharpeRatio").toDouble();
            loaded.result.maxDrawdown      = q.value("maxDrawdown").toDouble();
            loaded.result.winRate          = q.value("winRate").toDouble();
            loaded.result.totalTrades      = q.value("totalTrades").toInt();
            loaded.result.initialCapital   = q.value("initialCapital").toDouble();
            loaded.result.finalCapital     = q.value("finalCapital").toDouble();
            loaded.result.alphaVsBenchmark = q.value("alpha").toDouble();
        }
    }

    // Equity curve → LedgerSnapshot vector
    {
        auto q = query_fetchEquityCurve(runId, conn);
        if (q.exec()) {
            while (q.next()) {
                Backtest::LedgerSnapshot s;
                s.timestamp      = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
                s.portfolioValue = q.value("value").toDouble();
                loaded.result.equityCurve.append(s);

                Backtest::LedgerSnapshot bm;
                bm.timestamp      = s.timestamp;
                bm.portfolioValue = q.value("benchmarkValue").toDouble();
                loaded.result.benchmark.equityCurve.append(bm);
            }
        }
    }

    // Trade log → FilledOrder vector
    {
        auto q = query_fetchBacktestTrades(runId, conn);
        if (q.exec()) {
            int id = 0;
            while (q.next()) {
                Backtest::FilledOrder f;
                f.orderId   = ++id;
                f.symbol    = q.value("symbol").toString();
                f.quantity  = (q.value("side").toString() == QLatin1String("BUY"))
                    ? q.value("quantity").toDouble()
                    : -q.value("quantity").toDouble();
                f.fillPrice = q.value("fillPrice").toDouble();
                f.timestamp = QDateTime::fromString(q.value("timestamp").toString(), Qt::ISODate);
                loaded.result.tradeLog.append(f);
            }
        }
    }

    m_pBacktestDock->displayResult(loaded);
}

void CPresenter::onBacktestFinished(const Backtest::BacktestLoadedRun& run) {
    if (!m_pBacktestDock) return;
    m_pBacktestDock->setRunning(false);
    m_pBacktestDock->displayResult(run);

    // Refresh run history
    const QString conn = m_pBacktestController ? m_pBacktestController->dbConnectionName()
                                               : QStringLiteral("myLocalDb.sqlite");
    auto q = query_fetchRunsForStrategy(run.record.strategyId, conn);
    if (q.exec()) {
        QList<DbBacktestRunSummary> summaries;
        while (q.next()) {
            DbBacktestRunSummary s;
            s.runId        = q.value("runId").toString();
            s.strategyId   = q.value("strategyId").toString();
            s.symbols      = q.value("symbols").toString();
            s.startDate    = q.value("startDate").toString();
            s.endDate      = q.value("endDate").toString();
            s.status       = q.value("status").toString();
            s.dataSourceId = q.value("dataSourceId").toString();
            s.createdAt    = q.value("createdAt").toString();
            s.totalReturn  = q.value("totalReturn").toDouble();
            s.sharpeRatio  = q.value("sharpeRatio").toDouble();
            summaries.append(s);
        }
        m_pBacktestDock->setRunHistory(summaries);
    }

    // Post-run: compare pipeline config used in this run vs the pinned catalog version config.
    if (m_backend
        && !run.record.catalogStrategyId.isEmpty()
        && !run.record.catalogVersionId.isEmpty())
    {
        QJsonObject versionInfo = m_backend->strategyVersionInfo(run.record.catalogVersionId);
        QString versionConfigJson = versionInfo.value("configJson").toString();

        // Extract only the pipeline config from the full run config for comparison,
        // since the version stores pipeline config, not the full backtest run config.
        QJsonObject fullRunConfig = QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
        QString runPipelineJson = fullRunConfig.value("pipelineConfigJson").toString();

        QJsonDocument runPipelineDoc = QJsonDocument::fromJson(runPipelineJson.toUtf8());
        QJsonDocument verConfigDoc   = QJsonDocument::fromJson(versionConfigJson.toUtf8());

        if (runPipelineDoc != verConfigDoc
            && !runPipelineDoc.isEmpty() && !verConfigDoc.isEmpty())
        {
            auto answer = QMessageBox::question(
                pIbtsView,
                QStringLiteral("Save as New Version?"),
                QStringLiteral(
                    "The backtest ran with a pipeline configuration that differs from the pinned version.\n\n"
                    "Would you like to save the run configuration as a new strategy version?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (answer == QMessageBox::Yes) {
                QJsonObject pipelineConfig = runPipelineDoc.object();
                QString newVerId = m_backend->createStrategyVersion(
                    run.record.catalogStrategyId,
                    pipelineConfig,
                    QStringLiteral("Saved from backtest run ") + run.record.runId);
                if (!newVerId.isEmpty()) {
                    QMessageBox::information(
                        pIbtsView,
                        QStringLiteral("Version Created"),
                        QStringLiteral("New version created successfully."));
                    refreshStrategyCatalog();
                }
            }
        }
    }
}

void CPresenter::onBacktestFailed(const QString& reason) {
    if (!m_pBacktestDock) return;
    m_pBacktestDock->setRunning(false);
    m_pBacktestDock->setStatus(QStringLiteral("Failed: ") + reason);
}

// ---------------------------------------------------------------------------
// onTreeSelectionChanged (existing, unchanged)
// ---------------------------------------------------------------------------

void CPresenter::onTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    if (!current.isValid()) return;

    if (!m_pSystemTreeModel) {
        if (m_pDiagramWidget) m_pDiagramWidget->clear();
        return;
    }

    // Handle virtual block nodes first
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

            int arrayIndex = -1;
            if (isArray) {
                QJsonArray arr = adapter->pipelineConfig().value(jsonKey).toArray();
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    QString id = obj.value(Pipeline::Key::BlockId).toString();
                    if (id.isEmpty()) id = obj.value("type").toString();
                    if (id == blockId) { arrayIndex = i; break; }
                }
            }

            pIbtsView->contextWorkspace()->showBlockWorkspace(
                adapter, category, blockId, jsonKey, arrayIndex);
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

