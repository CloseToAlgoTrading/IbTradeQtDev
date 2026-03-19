#include "ibtradesystemview.h"
#include "GlobalStatusBar.h"
#include "EventLogPanel.h"
#include "ContextWorkspace.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include <time.h>
#include <QStandardItemModel>
#include "GlobalDef.h"
#include <QTime>
#include <QSharedPointer>
#include "ciconhandler.h"
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>


CIBTradeSystemView::CIBTradeSystemView(QWidget *parent)
	: QMainWindow(parent)
    , m_pTimeLabel(new QLabel(this))
    , m_pConnectLabel(new QLabel(this))
    , m_ih()
{
	ui.setupUi(this);



    /*** Begin Create Context Menu **************/
    ui.test_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Account"),   "Add New Account");   // [0]
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Portfolio"),  "Add New Portfolio"); // [1]
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Strategy"),   "Add Strategy");      // [2]
    QAction *act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);                                                               // [3] separator
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Strategy"), "Add Selection Model"); // [4]
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Strategy"), "Add Aplha Model");     // [5]
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Strategy"), "Add Rebalance Model"); // [6]
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Strategy"), "Add Risk Model");      // [7]
    ui.test_treeView->addAction(m_ih.loadIconFromResourceTheme("Strategy"), "Add Execution Model"); // [8]
    act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);                                                               // [9] separator
    ui.test_treeView->addAction("Remove Selected Node");                                            // [10]
    act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);                                                               // [11] separator
    ui.test_treeView->addAction("Open in Backtest Workspace");                                      // [12]

    /*** End Create Context Menu **************/

    this->setWindowTitle(QString("IB Trade v. ") + QString(APP_VERSION));

	ui.statusBar->addWidget(m_pTimeLabel);
    ui.statusBar->addPermanentWidget(m_pConnectLabel);
    ui.statusBar->setSizeGripEnabled(true);

    m_pConnectLabel->setPixmap(m_ih.loadIconFromResourceTheme("NotConnected").pixmap(16));
    m_pConnectLabel->setToolTip("Disconnected");

    ui.actionLoad->setIcon(m_ih.loadIconFromResourceTheme("LoadConfiguration"));
    ui.actionSave->setIcon(m_ih.loadIconFromResourceTheme("SaveConfiguration"));
    ui.actionSetting->setIcon(m_ih.loadIconFromResourceTheme("Settings"));
    ui.actionClear_Log->setIcon(m_ih.loadIconFromResourceTheme("LogClear"));
    ui.actionShow_Log->setIcon(m_ih.loadIconFromResourceTheme("Log"));
    ui.actionConnect->setIcon(m_ih.loadIconFromResourceTheme("Disconnect"));

    setupConsoleLayout();
}

CIBTradeSystemView::~CIBTradeSystemView()
{

}

void CIBTradeSystemView::setupConsoleLayout()
{
    m_globalStatusBar  = new GlobalStatusBar(this);
    m_contextWorkspace = new ContextWorkspace(this);
    m_eventLogPanel    = new EventLogPanel(this);

    // Detach the tree from its old parent layout so we can reparent it
    // into the new horizontal splitter. The .ui hierarchy is:
    //   centralWidget -> verticalLayout_2 -> splitter_2 -> frameControlPanel
    //     -> splitter -> frame_4 -> test_treeView
    ui.test_treeView->setParent(nullptr);

    // ── "Live Trading" tab content ──────────────────────────────────────────
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(ui.test_treeView);
    m_mainSplitter->addWidget(m_contextWorkspace);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);
    ui.test_treeView->setMinimumWidth(300);

    // ── "Backtest" tab content ──────────────────────────────────────────────
    // Left: strategy selector panel
    m_backtestSelector = new BacktestUI::BacktestStrategySelector(this);

    // Right: placeholder widget — CPresenter will reparent the BacktestWorkspaceDock
    // inner widget here via addBacktestWorkspace().  We expose it as a plain
    // QWidget so it can host whatever CPresenter injects.
    auto* backtestRight = new QWidget(this);
    backtestRight->setObjectName(QStringLiteral("BacktestRightPane"));
    auto* backtestRightLayout = new QVBoxLayout(backtestRight);
    backtestRightLayout->setContentsMargins(0, 0, 0, 0);
    // "Nothing selected yet" placeholder — replaced when CPresenter injects the dock widget
    auto* placeholder = new QLabel(
        QStringLiteral("← Select a strategy to start a backtest"), backtestRight);
    placeholder->setObjectName(QStringLiteral("BacktestPlaceholderLabel"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(QStringLiteral("color:#555; font-size:13px;"));
    backtestRightLayout->addWidget(placeholder);

    auto* backtestSplitter = new QSplitter(Qt::Horizontal, this);
    backtestSplitter->addWidget(m_backtestSelector);
    backtestSplitter->addWidget(backtestRight);
    backtestSplitter->setStretchFactor(0, 0);  // selector: fixed-ish
    backtestSplitter->setStretchFactor(1, 1);  // workspace: expands
    backtestSplitter->setHandleWidth(4);

    // ── Main tab widget ─────────────────────────────────────────────────────
    m_mainTabWidget = new QTabWidget(this);
    m_mainTabWidget->setTabPosition(QTabWidget::North);
    m_mainTabWidget->setDocumentMode(false);
    m_mainTabWidget->setObjectName(QStringLiteral("MainTabWidget"));
    m_strategyMgmtPanel = new StrategyMgmt::StrategyManagementPanel(this);

    m_mainTabWidget->addTab(m_mainSplitter,      QStringLiteral("Live Trading"));
    m_mainTabWidget->addTab(backtestSplitter,     QStringLiteral("Backtest"));
    m_mainTabWidget->addTab(m_strategyMgmtPanel,  QStringLiteral("Strategy Management"));

    // Remove and hide the old .ui splitter hierarchy.
    ui.verticalLayout_2->removeWidget(ui.splitter_2);
    ui.splitter_2->hide();

    auto* centralLayout = ui.verticalLayout_2;
    centralLayout->addWidget(m_globalStatusBar);
    centralLayout->addWidget(m_mainTabWidget, 1);

    // Replace the old logging dock with EventLogPanel
    removeDockWidget(ui.dockWidget_Logging);
    ui.dockWidget_Logging->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_eventLogPanel);
}

void CIBTradeSystemView::switchToBacktestTab()
{
    if (m_mainTabWidget)
        m_mainTabWidget->setCurrentIndex(1);
}

void CIBTradeSystemView::switchToStrategyManagementTab()
{
    if (m_mainTabWidget)
        m_mainTabWidget->setCurrentIndex(2);
}

Ui::IBTradeSystemClass CIBTradeSystemView::getUi()
{
    return ui;
}


QTreeView *CIBTradeSystemView::getSettingsTreeView()
{
    return ui.settingsTreeView;
}

QTreeView *CIBTradeSystemView::getPortfolioConfigTreeView()
{
    return ui.test_treeView;
}

void CIBTradeSystemView::mapSignals()
{
    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &CIBTradeSystemView::slotClearLog);
    QObject::connect(ui.actionShow_Log, &QAction::triggered, this, &CIBTradeSystemView::slotShowLog);
    QObject::connect(ui.actionSetting, &QAction::triggered, this, &CIBTradeSystemView::slotshowSettings);

    QObject::connect(ui.test_treeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });
    QObject::connect(ui.test_treeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });

    QObject::connect(ui.settingsTreeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.settingsTreeView->resizeColumnToContents(index.column()); });
    QObject::connect(ui.settingsTreeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.settingsTreeView->resizeColumnToContents(index.column()); });
}

void CIBTradeSystemView::slotOnTimeReceived(long time)
{
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch((time_t)(time));
    QString dateTimeString = dateTime.toString("dd-MM-yyyy hh:mm:ss");
    m_pTimeLabel->setText(dateTimeString);

    if (m_globalStatusBar)
        m_globalStatusBar->setTime(dateTimeString);
}


void CIBTradeSystemView::slotOnLogMsgReceived(QString msg)
{
    ui.textEdit->append(msg);

    if (m_eventLogPanel) {
        m_eventLogPanel->appendEvent(
            EventLogPanel::makeSystemEvent(LogLevel::Info, msg));
    }
}

void CIBTradeSystemView::slotRecvConnectButtonState(bool isConnect)
{
	if (isConnect)
	{
        ui.actionConnect->setText("Disconnect");
        ui.actionConnect->setIcon(m_ih.loadIconFromResourceTheme("Connect"));

        m_pConnectLabel->setPixmap(m_ih.loadIconFromResourceTheme("Connected").pixmap(16));
        m_pConnectLabel->setToolTip("Connected");

        if (m_globalStatusBar)
            m_globalStatusBar->setConnectionState(QStringLiteral("IB"), true);
	}
	else
	{
        ui.actionConnect->setText("Connect");
        ui.actionConnect->setIcon(m_ih.loadIconFromResourceTheme("Disconnect"));

        m_pConnectLabel->setPixmap(m_ih.loadIconFromResourceTheme("NotConnected").pixmap(16));
        m_pConnectLabel->setToolTip("Disconnected");

        if (m_globalStatusBar)
            m_globalStatusBar->setConnectionState(QStringLiteral("IB"), false);
    }
}

void CIBTradeSystemView::slotClearLog()
{
    if (m_eventLogPanel)
        m_eventLogPanel->clearAll();
}

void CIBTradeSystemView::slotShowLog()
{
    if (m_eventLogPanel)
        m_eventLogPanel->show();
}

void CIBTradeSystemView::slotshowSettings()
{
    ui.dockWidget_Settings->show();
}

void CIBTradeSystemView::slotUpdateTreeView(const QModelIndex &index)
{
    ui.test_treeView->update(index);
    ui.test_treeView->expand(index);
}

void CIBTradeSystemView::slotUpdateTreeViewAll()
{
    ui.test_treeView->update();
}


