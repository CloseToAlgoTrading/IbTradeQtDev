#include "BacktestUI/BacktestSummaryStatisticsPanel.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QStandardItem>
#include <QTableView>
#include <QVBoxLayout>

namespace BacktestUI {

BacktestSummaryStatisticsPanel::BacktestSummaryStatisticsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* title = new QLabel(QStringLiteral("Summary statistics"));
    title->setObjectName(QStringLiteral("BacktestSummaryStatisticsTitle"));

    m_model = new QStandardItemModel(this);
    m_table = new QTableView(this);
    m_table->setObjectName(QStringLiteral("BacktestSummaryStatisticsTable"));
    m_table->setModel(m_model);
    m_table->setSortingEnabled(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->addWidget(title);
    layout->addWidget(m_table);
}

void BacktestSummaryStatisticsPanel::setSummaryRows(const QVector<Backtest::BacktestSummaryRow>& rows,
                                                     bool threeColumns,
                                                     const QString& benchmarkColumnTitle)
{
    m_model->clear();
    const int cols = threeColumns ? 3 : 2;
    m_model->setColumnCount(cols);
    if (threeColumns) {
        const QString benchHdr = benchmarkColumnTitle.isEmpty()
            ? QStringLiteral("Benchmark")
            : benchmarkColumnTitle;
        m_model->setHorizontalHeaderLabels({
            QStringLiteral("Metric"),
            QStringLiteral("Strategy"),
            benchHdr,
        });
    } else {
        m_model->setHorizontalHeaderLabels({
            QStringLiteral("Metric"),
            QStringLiteral("Strategy"),
        });
    }

    m_model->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const Backtest::BacktestSummaryRow& r = rows[i];
        auto* m = new QStandardItem(r.metric);
        auto* s = new QStandardItem(r.strategyValue);
        m->setFlags(m->flags() & ~Qt::ItemIsEditable);
        s->setFlags(s->flags() & ~Qt::ItemIsEditable);
        m_model->setItem(i, 0, m);
        m_model->setItem(i, 1, s);
        if (threeColumns) {
            auto* b = new QStandardItem(r.benchmarkValue);
            b->setFlags(b->flags() & ~Qt::ItemIsEditable);
            m_model->setItem(i, 2, b);
        }
    }
}

void BacktestSummaryStatisticsPanel::clear()
{
    m_model->clear();
    m_model->setRowCount(0);
    m_model->setColumnCount(0);
}

} // namespace BacktestUI
