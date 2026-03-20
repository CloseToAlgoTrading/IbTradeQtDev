#ifndef UILAYOUTDEFAULTS_H
#define UILAYOUTDEFAULTS_H

class QTreeView;
class QTableView;
class QTableWidget;
class QSplitter;
class QTabWidget;
class QMainWindow;
class QDockWidget;
class CIBTradeSystemView;
class EventLogPanel;

// Single place for built-in layout defaults (first run + "Restore default layout").
namespace UiLayoutDefaults {

void applySystemTreeColumnDefaults(QTreeView* treeView);
void applyBacktestStrategyTreeDefaults(QTreeView* treeView);
void applyStrategyCatalogTreeDefaults(QTreeView* treeView);
void applyEventLogTableDefaults(QTableView* tableView);
void applyStrategyVersionTableDefaults(QTableWidget* tableWidget);

void applySplitterDefaults(QSplitter* mainLive, QSplitter* backtest, QSplitter* strategyMgmt);
void applyMainTabDefault(QTabWidget* tabs);
void applyMainWindowDockDefaults(QMainWindow* mainWindow, EventLogPanel* eventLogDock,
                                 QDockWidget* diagramDock = nullptr);

void applyFullDefaults(CIBTradeSystemView* view);

} // namespace UiLayoutDefaults

#endif
