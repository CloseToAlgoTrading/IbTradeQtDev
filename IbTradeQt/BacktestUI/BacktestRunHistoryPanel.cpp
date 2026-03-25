#include "BacktestUI/BacktestRunHistoryPanel.h"
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDateTime>
#include <QMenu>
#include <QAction>

namespace BacktestUI {

BacktestRunHistoryPanel::BacktestRunHistoryPanel(QWidget* parent)
    : QWidget(parent)
{
    m_model = new QStandardItemModel(0, ColCount, this);
    m_model->setHorizontalHeaderLabels({
        QStringLiteral("Date"),
        QStringLiteral("Period"),
        QStringLiteral("Symbols"),
        QStringLiteral("Return"),
        QStringLiteral("Sharpe"),
        QStringLiteral("Status"),
        QStringLiteral("Source")
    });

    m_table = new QTableView(this);
    m_table->setObjectName(QStringLiteral("BacktestRunHistoryTable"));
    m_table->setModel(m_model);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setTextElideMode(Qt::ElideRight);

    QHeaderView* header = m_table->horizontalHeader();
    header->setStretchLastSection(true);
    // Interactive columns (user can drag headers). Symbols is not ResizeToContents — long lists
    // would make that column dominate the table. Last column stretches via stretchLastSection.
    for (int c = 0; c < ColSource; ++c)
        header->setSectionResizeMode(c, QHeaderView::Interactive);
    // Default: show roughly a handful of tickers; user can widen via column border drag.
    {
        const int sample = m_table->fontMetrics().horizontalAdvance(
            QStringLiteral("AAPL, MSFT, NVDA, AVGO, "));
        m_table->setColumnWidth(ColSymbols, sample + 24);
    }
    m_table->verticalHeader()->setVisible(false);

    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableView::customContextMenuRequested,
            this, &BacktestRunHistoryPanel::onCustomContextMenu);
    connect(m_table, &QTableView::activated,
            this, &BacktestRunHistoryPanel::onRowActivated);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
}

void BacktestRunHistoryPanel::setRuns(const QList<DbBacktestRunSummary>& runs) {
    m_model->removeRows(0, m_model->rowCount());
    m_model->setRowCount(runs.size());

    for (int i = 0; i < runs.size(); ++i) {
        const auto& r = runs[i];

        auto makeItem = [&](const QString& text) {
            auto* item = new QStandardItem(text);
            item->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
            return item;
        };
        auto makeSymbolsItem = [&](const QString& text) {
            auto* item = new QStandardItem(text);
            item->setData(int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
            return item;
        };

        QDateTime createdAt = QDateTime::fromString(r.createdAt, Qt::ISODate).toLocalTime();
        QString period = r.startDate.left(10) + " – " + r.endDate.left(10);

        QStandardItem* dateItem = makeItem(createdAt.toString("yyyy-MM-dd hh:mm"));
        dateItem->setData(r.runId, Qt::UserRole);
        m_model->setItem(i, ColDate,    dateItem);
        m_model->setItem(i, ColPeriod,  makeItem(period));
        QStandardItem* symItem = makeSymbolsItem(r.symbols);
        symItem->setToolTip(r.symbols);
        m_model->setItem(i, ColSymbols, symItem);
        m_model->setItem(i, ColReturn,  makeItem(QString("%1%").arg(r.totalReturn * 100.0, 0, 'f', 1)));
        m_model->setItem(i, ColSharpe,  makeItem(QString::number(r.sharpeRatio, 'f', 2)));
        m_model->setItem(i, ColStatus,  makeItem(r.status));
        m_model->setItem(i, ColSource,  makeItem(r.dataSourceId));
    }

    // Tight layout for compact columns; leave Symbols width as set in ctor / user-adjusted.
    for (int c : {ColDate, ColPeriod, ColReturn, ColSharpe, ColStatus}) {
        m_table->resizeColumnToContents(c);
    }
}

void BacktestRunHistoryPanel::clear() {
    m_model->removeRows(0, m_model->rowCount());
}

void BacktestRunHistoryPanel::onRowActivated(const QModelIndex& index) {
    if (!index.isValid()) return;
    QStandardItem* dateItem = m_model->item(index.row(), ColDate);
    if (!dateItem) return;
    const QString runId = dateItem->data(Qt::UserRole).toString();
    if (runId.isEmpty()) return;
    emit loadRunRequested(runId);
}

void BacktestRunHistoryPanel::onCustomContextMenu(const QPoint& pos)
{
    const QModelIndex idx = m_table->indexAt(pos);
    if (!idx.isValid())
        return;
    QStandardItem* dateItem = m_model->item(idx.row(), ColDate);
    if (!dateItem)
        return;
    const QString runId = dateItem->data(Qt::UserRole).toString();
    if (runId.isEmpty())
        return;

    QMenu menu(this);
    QAction* del = menu.addAction(QStringLiteral("Delete…"));
    if (menu.exec(m_table->viewport()->mapToGlobal(pos)) == del)
        emit deleteRunRequested(runId);
}

} // namespace BacktestUI
