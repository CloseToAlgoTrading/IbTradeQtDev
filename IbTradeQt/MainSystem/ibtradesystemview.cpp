#include "ibtradesystemview.h"
#include "UiLayoutStore.h"
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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QResizeEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QSignalBlocker>
#include <QtGlobal>


CIBTradeSystemView::CIBTradeSystemView(QWidget *parent)
	: QMainWindow(parent)
    , m_pTimeLabel(new QLabel(this))
    , m_pConnectLabel(new QLabel(this))
    , m_ih()
{
    m_pTimeLabel->setObjectName(QStringLiteral("statusBarTimeLabel"));
    m_pConnectLabel->setObjectName(QStringLiteral("statusBarConnectionIndicator"));

	ui.setupUi(this);
    ui.actionSetting->setCheckable(true);



    /*** Begin Create Context Menu **************/
    ui.test_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    ui.test_treeView->addAction(m_ih.loadIconForChrome("Account"),   "Add New Account");   // [0]
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Portfolio"),  "Add New Portfolio"); // [1]
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Strategy"),   "Add Strategy");      // [2]
    QAction *act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);                                                               // [3] separator
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Strategy"), "Add Selection Model"); // [4]
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Strategy"), "Add Aplha Model");     // [5]
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Strategy"), "Add Rebalance Model"); // [6]
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Strategy"), "Add Risk Model");      // [7]
    ui.test_treeView->addAction(m_ih.loadIconForChrome("Strategy"), "Add Execution Model"); // [8]
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

    m_pConnectLabel->setPixmap(m_ih.loadIconForChrome("NotConnected").pixmap(16));
    m_pConnectLabel->setToolTip("Disconnected");

    ui.actionLoad->setIcon(m_ih.loadIconForChrome("LoadConfiguration"));
    ui.actionSave->setIcon(m_ih.loadIconForChrome("SaveConfiguration"));
    ui.actionSetting->setIcon(m_ih.loadIconForChrome("Settings"));
    ui.actionClear_Log->setIcon(m_ih.loadIconForChrome("LogClear"));
    ui.actionShow_Log->setIcon(m_ih.loadIconForChrome("Log"));
    ui.actionConnect->setIcon(m_ih.loadIconForChrome("Disconnect"));

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
    m_mainSplitter->setObjectName(QStringLiteral("MainLiveSplitter"));
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
    backtestRightLayout->addWidget(placeholder);

    m_backtestSplitter = new QSplitter(Qt::Horizontal, this);
    m_backtestSplitter->addWidget(m_backtestSelector);
    m_backtestSplitter->addWidget(backtestRight);
    m_backtestSplitter->setStretchFactor(0, 0);  // selector: fixed-ish
    m_backtestSplitter->setStretchFactor(1, 1);  // workspace: expands
    m_backtestSplitter->setHandleWidth(4);

    // ── Main tab widget ─────────────────────────────────────────────────────
    m_mainTabWidget = new QTabWidget(this);
    m_mainTabWidget->setTabPosition(QTabWidget::North);
    m_mainTabWidget->setDocumentMode(false);
    m_mainTabWidget->setObjectName(QStringLiteral("MainTabWidget"));
    m_strategyMgmtPanel = new StrategyMgmt::StrategyManagementPanel(this);

    m_mainTabWidget->addTab(m_mainSplitter,      QStringLiteral("Live Trading"));
    m_mainTabWidget->addTab(m_backtestSplitter,  QStringLiteral("Backtest"));
    m_mainTabWidget->addTab(m_strategyMgmtPanel,  QStringLiteral("Strategy Management"));

    // Remove and hide the old .ui splitter hierarchy.
    ui.verticalLayout_2->removeWidget(ui.splitter_2);
    ui.splitter_2->hide();

    auto* centralLayout = ui.verticalLayout_2;
    centralLayout->addWidget(m_globalStatusBar);
    centralLayout->addWidget(m_mainTabWidget, 1);

    // Events dock on the bottom; Settings uses a right slide-over panel (see setupSettingsSlideOverlay).
    removeDockWidget(ui.dockWidget_Settings);
    ui.dockWidget_Settings->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_eventLogPanel);

    // Allow the bottom Events dock to grow upward: QMainWindow won’t shrink the
    // central area below its minimumSizeHint; clear an inherited minimum so tab
    // content (esp. Backtest) doesn’t lock the splitter range.
    if (QWidget* cw = centralWidget()) {
        cw->setMinimumHeight(0);
        cw->setMinimumWidth(0);
    }

    setupSettingsSlideOverlay();
}

void CIBTradeSystemView::setupSettingsSlideOverlay()
{
    m_settingsOverlay = new QWidget(this);
    m_settingsOverlay->setObjectName(QStringLiteral("SettingsOverlay"));
    m_settingsOverlay->hide();
    m_settingsOverlay->setFocusPolicy(Qt::StrongFocus);

    m_settingsBackdrop = new QFrame(m_settingsOverlay);
    m_settingsBackdrop->setObjectName(QStringLiteral("SettingsOverlayBackdrop"));
    m_settingsBackdrop->setFrameShape(QFrame::NoFrame);
    m_settingsBackdrop->installEventFilter(this);

    m_settingsSheet = new QFrame(m_settingsOverlay);
    m_settingsSheet->setObjectName(QStringLiteral("SettingsSlidePanel"));
    m_settingsSheet->setFrameShape(QFrame::NoFrame);

    auto* title = new QLabel(QStringLiteral("Settings"), m_settingsSheet);
    title->setObjectName(QStringLiteral("SettingsSlideTitle"));

    auto* closeBtn = new QToolButton(m_settingsSheet);
    closeBtn->setObjectName(QStringLiteral("SettingsSlideCloseButton"));
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setAutoRaise(true);
    closeBtn->setToolTip(QStringLiteral("Close"));
    QObject::connect(closeBtn, &QAbstractButton::clicked, this, [this] {
        setSettingsOverlayVisible(false);
    });

    ui.settingsTreeView->setParent(m_settingsSheet);
    // Sheet chrome: padding, child margins, tree/header — operations-console.qss.
    // QLayout spacing/margins are not stylable in Qt; defaults are used (see THEMING.md).

    auto* headerWrap = new QWidget(m_settingsSheet);
    auto* headerLay  = new QHBoxLayout(headerWrap);
    headerLay->addWidget(title);
    headerLay->addStretch();
    headerLay->addWidget(closeBtn);

    auto* body = new QVBoxLayout(m_settingsSheet);
    body->addWidget(headerWrap);
    body->addWidget(ui.settingsTreeView, 1);
}

void CIBTradeSystemView::updateSettingsOverlayGeometry()
{
    if (!m_settingsOverlay || !centralWidget())
        return;

    const QPoint origin = centralWidget()->mapTo(this, QPoint(0, 0));
    m_settingsOverlay->setGeometry(origin.x(), origin.y(),
                                   centralWidget()->width(), centralWidget()->height());
    m_settingsBackdrop->setGeometry(0, 0, m_settingsOverlay->width(), m_settingsOverlay->height());

    if (m_settingsOverlayVisible && m_settingsOverlay->isVisible() && m_settingsSheet) {
        const int w = m_settingsSheet->width();
        m_settingsSheet->setGeometry(m_settingsOverlay->width() - w, 0,
                                      w, m_settingsOverlay->height());
    }
}

void CIBTradeSystemView::setSettingsOverlayVisible(bool visible)
{
    if (!m_settingsOverlay || !m_settingsBackdrop || !m_settingsSheet)
        return;

    if (visible == m_settingsOverlayVisible && m_settingsOverlay->isVisible() == visible)
        return;

    m_settingsOverlayVisible = visible;
    {
        const QSignalBlocker b(ui.actionSetting);
        ui.actionSetting->setChecked(visible);
    }

    updateSettingsOverlayGeometry();

    if (visible) {
        m_settingsOverlay->show();
        m_settingsOverlay->raise();
        m_settingsOverlay->setFocus();
        m_settingsBackdrop->show();
        m_settingsSheet->show();
        m_settingsSheet->raise();

        const int w = m_settingsSheet->width();
        const int h = m_settingsOverlay->height();
        const int fullW = m_settingsOverlay->width();
        const QRect start(fullW, 0, w, h);
        const QRect end(fullW - w, 0, w, h);
        m_settingsSheet->setGeometry(start);

        auto* anim = new QPropertyAnimation(m_settingsSheet, "geometry", this);
        anim->setDuration(200);
        anim->setStartValue(start);
        anim->setEndValue(end);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        const int w = m_settingsSheet->width();
        const int h = m_settingsOverlay->height();
        const int fullW = m_settingsOverlay->width();
        const QRect start = m_settingsSheet->geometry();
        const QRect end(fullW, 0, w, h);

        auto* anim = new QPropertyAnimation(m_settingsSheet, "geometry", this);
        anim->setDuration(160);
        anim->setStartValue(start);
        anim->setEndValue(end);
        anim->setEasingCurve(QEasingCurve::InCubic);
        QObject::connect(anim, &QPropertyAnimation::finished, this, [this] {
            if (m_settingsOverlay)
                m_settingsOverlay->hide();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void CIBTradeSystemView::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateSettingsOverlayGeometry();
    if (m_uiLayoutStore)
        m_uiLayoutStore->scheduleSave();
}

void CIBTradeSystemView::setUiLayoutStore(UiLayoutStore* store)
{
    m_uiLayoutStore = store;
}

bool CIBTradeSystemView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_settingsBackdrop && event->type() == QEvent::MouseButtonPress) {
        setSettingsOverlayVisible(false);
        return true;
    }
    return QObject::eventFilter(watched, event);
}

void CIBTradeSystemView::keyPressEvent(QKeyEvent* event)
{
    if (m_settingsOverlayVisible && event->key() == Qt::Key_Escape) {
        setSettingsOverlayVisible(false);
        return;
    }
    QMainWindow::keyPressEvent(event);
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
    QObject::connect(ui.actionSetting, &QAction::toggled,
                     this, &CIBTradeSystemView::setSettingsOverlayVisible);

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
        ui.actionConnect->setIcon(m_ih.loadIconForChrome("Connect"));

        m_pConnectLabel->setPixmap(m_ih.loadIconForChrome("Connected").pixmap(16));
        m_pConnectLabel->setToolTip("Connected");

        if (m_globalStatusBar)
            m_globalStatusBar->setConnectionState(QStringLiteral("IB"), true);
	}
	else
	{
        ui.actionConnect->setText("Connect");
        ui.actionConnect->setIcon(m_ih.loadIconForChrome("Disconnect"));

        m_pConnectLabel->setPixmap(m_ih.loadIconForChrome("NotConnected").pixmap(16));
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

void CIBTradeSystemView::slotUpdateTreeView(const QModelIndex &index)
{
    ui.test_treeView->update(index);
    ui.test_treeView->expand(index);
}

void CIBTradeSystemView::slotUpdateTreeViewAll()
{
    ui.test_treeView->update();
}


