#include "UiLayoutDefaults.h"
#include "SystemTreeModel.h"
#include "BacktestUI/BacktestTreeModel.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableView>
#include <QTableWidget>
#include <QTreeView>

namespace UiLayoutDefaults {

void applySystemTreeColumnDefaults(QTreeView* treeView)
{
    if (!treeView)
        return;
    treeView->header()->setStretchLastSection(true);
    treeView->header()->setSectionResizeMode(SystemTreeModel::ColName, QHeaderView::Interactive);
    treeView->header()->setSectionResizeMode(SystemTreeModel::ColEnabled, QHeaderView::Fixed);
    treeView->header()->setSectionResizeMode(SystemTreeModel::ColStatus, QHeaderView::Interactive);
    treeView->setColumnWidth(SystemTreeModel::ColName, 200);
    treeView->setColumnWidth(SystemTreeModel::ColEnabled, 50);
    treeView->setColumnWidth(SystemTreeModel::ColStatus, 110);
    treeView->expandAll();
    for (int c = 0; c < SystemTreeModel::ColumnCount - 1; ++c)
        treeView->resizeColumnToContents(c);
}

void applyBacktestStrategyTreeDefaults(QTreeView* tv)
{
    if (!tv)
        return;
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tv->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tv->header()->setStretchLastSection(true);
    tv->header()->setSectionResizeMode(BacktestUI::BacktestTreeModel::ColName,
                                       QHeaderView::Interactive);
    tv->header()->setSectionResizeMode(BacktestUI::BacktestTreeModel::ColVersion,
                                       QHeaderView::Stretch);
    tv->resizeColumnToContents(BacktestUI::BacktestTreeModel::ColName);
}

void applyEventLogTableDefaults(QTableView* tv)
{
    if (!tv)
        return;
    tv->horizontalHeader()->setStretchLastSection(true);
    tv->setColumnWidth(0, 140);
    tv->setColumnWidth(1, 160);
    tv->setColumnWidth(2, 120);
    tv->setColumnWidth(3, 60);
}

void applyStrategyVersionTableDefaults(QTableWidget* tw)
{
    if (!tw)
        return;
    tw->horizontalHeader()->setStretchLastSection(true);
}

} // namespace UiLayoutDefaults
