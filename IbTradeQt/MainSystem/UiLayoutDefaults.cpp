#include "UiLayoutDefaults.h"
#include "ibtradesystemview.h"
#include "EventLogPanel.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyCatalogPanel.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "StrategyTreePanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"
#include "ThemePalette.h"

#include <QDockWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QMainWindow>

namespace UiLayoutDefaults {

void applySplitterDefaults(QSplitter* mainLive, QSplitter* backtest, QSplitter* strategyMgmt)
{
    if (mainLive) {
        const int w = qMax(400, mainLive->width());
        mainLive->setSizes({w / 4, w - w / 4});
    }
    if (backtest) {
        const int w = qMax(400, backtest->width());
        backtest->setSizes({qMin(280, w / 3), w - qMin(280, w / 3)});
    }
    if (strategyMgmt)
        strategyMgmt->setSizes({280, 520});
}

void applyMainTabDefault(QTabWidget* tabs)
{
    if (tabs)
        tabs->setCurrentIndex(0);
}

void applyMainWindowDockDefaults(QMainWindow* mainWindow, EventLogPanel* eventLogDock)
{
    if (!mainWindow || !eventLogDock)
        return;
    QList<QDockWidget*> docks;
    docks << eventLogDock;
    QList<int> heights;
    heights << UiTheme::kEventLogDockInitialHeight;
    mainWindow->resizeDocks(docks, heights, Qt::Vertical);
}

void applyFullDefaults(CIBTradeSystemView* view)
{
    if (!view)
        return;

    applySplitterDefaults(view->mainSplitter(),
                          view->backtestSplitter(),
                          view->strategyManagementPanel()
                              ? view->strategyManagementPanel()->horizontalSplitter()
                              : nullptr);
    applyMainTabDefault(view->mainTabWidget());
    applyMainWindowDockDefaults(view, view->eventLogPanel());

    applySystemTreeColumnDefaults(view->getPortfolioConfigTreeView());

    if (view->backtestStrategySelector()) {
        applyBacktestStrategyTreeDefaults(
            view->backtestStrategySelector()->strategyTreeView());
    }

    if (view->strategyManagementPanel()) {
        auto* cat = view->strategyManagementPanel()->catalogPanel();
        if (cat && cat->treePanel() && cat->treePanel()->treeView()) {
            applyStrategyCatalogTreeDefaults(cat->treePanel()->treeView());
            const int n = cat->treePanel()->treeView()->model()
                              ? cat->treePanel()->treeView()->model()->columnCount()
                              : 0;
            for (int c = 0; c < n - 1; ++c)
                cat->treePanel()->treeView()->resizeColumnToContents(c);
        }
        auto* detail = view->strategyManagementPanel()->detailPanel();
        if (detail && detail->versionTable()) {
            applyStrategyVersionTableDefaults(detail->versionTable());
        }
    }

    if (view->eventLogPanel())
        view->eventLogPanel()->resetLogTableColumnDefaults();
}

} // namespace UiLayoutDefaults
