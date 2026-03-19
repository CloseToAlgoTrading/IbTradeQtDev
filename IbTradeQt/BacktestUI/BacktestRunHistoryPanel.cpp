#include "BacktestUI/BacktestRunHistoryPanel.h"
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDateTime>

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
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);

    connect(m_table, &QTableView::activated,
            this, &BacktestRunHistoryPanel::onRowActivated);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
}

void BacktestRunHistoryPanel::setRuns(const QList<DbBacktestRunSummary>& runs) {
    m_runs = runs;
    m_model->removeRows(0, m_model->rowCount());
    m_model->setRowCount(runs.size());

    for (int i = 0; i < runs.size(); ++i) {
        const auto& r = runs[i];

        auto makeItem = [&](const QString& text) {
            auto* item = new QStandardItem(text);
            item->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
            return item;
        };

        QDateTime createdAt = QDateTime::fromString(r.createdAt, Qt::ISODate).toLocalTime();
        QString period = r.startDate.left(10) + " – " + r.endDate.left(10);

        m_model->setItem(i, ColDate,    makeItem(createdAt.toString("yyyy-MM-dd hh:mm")));
        m_model->setItem(i, ColPeriod,  makeItem(period));
        m_model->setItem(i, ColSymbols, makeItem(r.symbols));
        m_model->setItem(i, ColReturn,  makeItem(QString("%1%").arg(r.totalReturn * 100.0, 0, 'f', 1)));
        m_model->setItem(i, ColSharpe,  makeItem(QString::number(r.sharpeRatio, 'f', 2)));
        m_model->setItem(i, ColStatus,  makeItem(r.status));
        m_model->setItem(i, ColSource,  makeItem(r.dataSourceId));
    }
}

void BacktestRunHistoryPanel::clear() {
    m_runs.clear();
    m_model->removeRows(0, m_model->rowCount());
}

void BacktestRunHistoryPanel::onRowActivated(const QModelIndex& index) {
    if (!index.isValid()) return;
    const int row = index.row();
    if (row < 0 || row >= m_runs.size()) return;
    emit loadRunRequested(m_runs.at(row).runId);
}

} // namespace BacktestUI
