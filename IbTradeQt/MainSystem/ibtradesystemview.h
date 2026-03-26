#ifndef IBTRADESYSTEMVIEW_H
#define IBTRADESYSTEMVIEW_H

#include <QtWidgets/QMainWindow>
#include <QtConcurrent/QtConcurrentRun>
#include "ciconhandler.h"
#include "ui_ibtradesystemview.h"
#include <QLabel>
#include <QIcon>

class GlobalStatusBar;
class EventLogPanel;
class ContextWorkspace;
class QSplitter;
class QTabWidget;
class QDockWidget;
class QFrame;
class QMainWindow;
class QResizeEvent;
class QShowEvent;
class QEvent;
class QKeyEvent;

class UiLayoutStore;

namespace BacktestUI { class BacktestStrategySelector; }
namespace StrategyMgmt { class StrategyManagementPanel; }
namespace DataManagementUI { class DataManagementPanel; }

class CIBTradeSystemView : public QMainWindow
{
	Q_OBJECT

public:
    CIBTradeSystemView(QWidget *parent = nullptr);
    ~CIBTradeSystemView();

    Ui::IBTradeSystemClass getUi();

    QTreeView * getSettingsTreeView();
    QTreeView * getPortfolioConfigTreeView();

    GlobalStatusBar*  globalStatusBar()  const { return m_globalStatusBar; }
    EventLogPanel*    eventLogPanel()    const { return m_eventLogPanel; }
    ContextWorkspace* contextWorkspace() const { return m_contextWorkspace; }
    QTabWidget*       mainTabWidget()    const { return m_mainTabWidget; }
    BacktestUI::BacktestStrategySelector* backtestStrategySelector() const
                                               { return m_backtestSelector; }
    StrategyMgmt::StrategyManagementPanel* strategyManagementPanel() const
                                               { return m_strategyMgmtPanel; }
    DataManagementUI::DataManagementPanel* dataManagementPanel() const
                                               { return m_dataManagementPanel; }

    QSplitter* mainSplitter() const { return m_mainSplitter; }
    QSplitter* backtestSplitter() const { return m_backtestSplitter; }
    QMainWindow* backtestDockHost() const { return m_backtestDockHost; }

    void setUiLayoutStore(UiLayoutStore* store);

    /// Bottom-area pipeline diagram dock (tabified with Events); set from CPresenter after creation.
    void setDiagramDock(QDockWidget* dock);
    QDockWidget* diagramDock() const { return m_diagramDock; }
    void setBacktestSummaryDock(QDockWidget* dock);
    QDockWidget* backtestSummaryDock() const { return m_backtestSummaryDock; }

    void switchToBacktestTab();
    void switchToStrategyManagementTab();
    void switchToDataManagementTab();

    void mapSignals();


protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupConsoleLayout();
    void setupSettingsSlideOverlay();
    void updateSettingsOverlayGeometry();
    void setSettingsOverlayVisible(bool visible);
    /// Tabified dock titles (Events / Diagram View): show full text, not "Eve…".
    void polishDockAreaTabBars();

    Ui::IBTradeSystemClass ui;
    QLabel * m_pTimeLabel;
    QLabel * m_pConnectLabel;
    CIconHandler m_ih;

    GlobalStatusBar*  m_globalStatusBar  = nullptr;
    EventLogPanel*    m_eventLogPanel    = nullptr;
    QDockWidget*      m_diagramDock     = nullptr;
    QDockWidget*      m_backtestSummaryDock = nullptr;
    ContextWorkspace* m_contextWorkspace = nullptr;
    QSplitter*        m_mainSplitter     = nullptr;   // inside "Live Trading" tab
    QSplitter*        m_backtestSplitter = nullptr;
    QMainWindow*      m_backtestDockHost = nullptr;
    QTabWidget*       m_mainTabWidget    = nullptr;
    BacktestUI::BacktestStrategySelector* m_backtestSelector = nullptr;
    StrategyMgmt::StrategyManagementPanel* m_strategyMgmtPanel = nullptr;
    DataManagementUI::DataManagementPanel* m_dataManagementPanel = nullptr;

    QWidget* m_settingsOverlay   = nullptr;
    QFrame*  m_settingsBackdrop  = nullptr;
    QFrame*  m_settingsSheet     = nullptr;
    bool     m_settingsOverlayVisible = false;

    UiLayoutStore* m_uiLayoutStore = nullptr;

private slots:
	void slotOnTimeReceived(long time);
	void slotOnLogMsgReceived(QString msg);
    void slotOnQtStructuredLog(int msgType, QString category, QString function, QString message);
	void slotRecvConnectButtonState(bool isConnect);
    void slotClearLog();
    void slotShowLog();

    void slotUpdateTreeView(const QModelIndex& index);
public:
    void slotUpdateTreeViewAll();

};

#endif // IBTRADESYSTEMVIEW_H
