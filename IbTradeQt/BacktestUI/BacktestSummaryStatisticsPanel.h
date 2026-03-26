#ifndef BACKTESTUI_BACKTESTSUMMARYSTATISTICSPANEL_H
#define BACKTESTUI_BACKTESTSUMMARYSTATISTICSPANEL_H

#include "Backtest/BacktestSummaryFormatter.h"

#include <QWidget>

class QTableView;
class QStandardItemModel;

namespace BacktestUI {

/// Displays rows from BacktestSummaryFormatter (view only — no metric math).
class BacktestSummaryStatisticsPanel : public QWidget {
    Q_OBJECT

public:
    explicit BacktestSummaryStatisticsPanel(QWidget* parent = nullptr);

    void setSummaryRows(const QVector<Backtest::BacktestSummaryRow>& rows,
                        bool threeColumns,
                        const QString& benchmarkColumnTitle = QString());

    void clear();

    QTableView* tableView() const { return m_table; }

signals:
    void headerStateRestorable();

private:
    QTableView*           m_table  = nullptr;
    QStandardItemModel*   m_model  = nullptr;
};

} // namespace BacktestUI

#endif
