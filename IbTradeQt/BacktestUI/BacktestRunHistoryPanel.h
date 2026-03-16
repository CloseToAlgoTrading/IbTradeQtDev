#ifndef BACKTESTUI_BACKTESTRUNHISTORYPANEL_H
#define BACKTESTUI_BACKTESTRUNHISTORYPANEL_H

// BacktestRunHistoryPanel — table of past backtest runs for the selected strategy.
//
// Columns: Date | Period | Symbols | Return | Sharpe | Status | Source
// Clicking a row emits loadRunRequested(runId) — CPresenter then fetches
// the full BacktestLoadedRun from DB and delivers it to the dock for display
// without re-running the backtest.

#include <QWidget>
#include <QList>
#include "DB/dbdatatypes.h"

class QTableView;
class QStandardItemModel;

namespace BacktestUI {

class BacktestRunHistoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit BacktestRunHistoryPanel(QWidget* parent = nullptr);

    // Populate with summary rows for the current strategy.
    // Replaces all existing rows.
    void setRuns(const QList<DbBacktestRunSummary>& runs);

    // Clear all rows.
    void clear();

signals:
    // Emitted when the user clicks a row. CPresenter fetches the full run.
    void loadRunRequested(const QString& runId);

private slots:
    void onRowActivated(const QModelIndex& index);

private:
    QTableView*         m_table = nullptr;
    QStandardItemModel* m_model = nullptr;

    QList<DbBacktestRunSummary> m_runs; // stored so we can look up runId by row

    static constexpr int ColDate    = 0;
    static constexpr int ColPeriod  = 1;
    static constexpr int ColSymbols = 2;
    static constexpr int ColReturn  = 3;
    static constexpr int ColSharpe  = 4;
    static constexpr int ColStatus  = 5;
    static constexpr int ColSource  = 6;
    static constexpr int ColCount   = 7;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTRUNHISTORYPANEL_H
