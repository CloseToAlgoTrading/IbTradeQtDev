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

namespace BacktestUI { class BacktestStrategySelector; }

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

    // Switch the main tab to "Backtest" (index 1)
    void switchToBacktestTab();

    void mapSignals();


private:
    void setupConsoleLayout();

    Ui::IBTradeSystemClass ui;
    QLabel * m_pTimeLabel;
    QLabel * m_pConnectLabel;
    CIconHandler m_ih;

    GlobalStatusBar*  m_globalStatusBar  = nullptr;
    EventLogPanel*    m_eventLogPanel    = nullptr;
    ContextWorkspace* m_contextWorkspace = nullptr;
    QSplitter*        m_mainSplitter     = nullptr;   // inside "Live Trading" tab
    QTabWidget*       m_mainTabWidget    = nullptr;
    BacktestUI::BacktestStrategySelector* m_backtestSelector = nullptr;

private slots:
	void slotOnTimeReceived(long time);
	void slotOnLogMsgReceived(QString msg);
	void slotRecvConnectButtonState(bool isConnect);
    void slotClearLog();
    void slotShowLog();
    void slotshowSettings();

    void slotUpdateTreeView(const QModelIndex& index);
public:
    void slotUpdateTreeViewAll();

};

#endif // IBTRADESYSTEMVIEW_H
