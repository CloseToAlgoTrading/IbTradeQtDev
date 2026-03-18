#ifndef BACKTESTUI_BACKTESTWORKSPACEDOCK_H
#define BACKTESTUI_BACKTESTWORKSPACEDOCK_H

// BacktestWorkspaceDock — dedicated QDockWidget for the Backtest Workflow.
//
// This dock is a pure view — it does NOT own BacktestController.
// CPresenter owns the controller; the dock only receives display calls.
//
// Layout:
//   ┌─ Backtest Workspace ─────────────────────────────────────────┐
//   │  Strategy: [name]    Portfolio: [path]    ID: [uuid prefix]  │
//   ├──────────────────────────────────────────────────────────────┤
//   │  BacktestRunConfigPanel (form + Run button + progress bar)   │
//   ├──────────────────────────────────────────────────────────────┤
//   │  [ Run History | Equity Curve | Candlestick | Trade Log ]    │
//   │  <tab content>                                               │
//   └──────────────────────────────────────────────────────────────┘
//
// Public interface (called by CPresenter only):
//   selectStrategy()  — switch to a new strategy, pre-populate form defaults
//   displayResult()   — populate all tabs from a loaded or just-finished run
//   setProgress()     — relay progress from BacktestController to config panel
//   setStatus()       — relay status string from BacktestController
//   setRunning()      — enable/disable Run button
//
// Signals forwarded to CPresenter:
//   runRequested(BacktestRunConfig)  — user clicked Run
//   loadRunRequested(runId)          — user selected a past run in history panel

#include <QDockWidget>
#include <QMap>
#include <QList>
#include "Backtest/BacktestDataTypes.h"
#include "DB/dbdatatypes.h"

class QLabel;
class QTabWidget;
class QWidget;
class QSplitter;

namespace BacktestUI {

class BacktestRunConfigPanel;
class BacktestRunHistoryPanel;
class EquityChartWidget;
class BacktestCandlestickWidget;
class TradeLogWidget;

class BacktestWorkspaceDock : public QDockWidget {
    Q_OBJECT

public:
    explicit BacktestWorkspaceDock(QWidget* parent = nullptr);

    // Select a strategy for backtesting. Pre-populates form from profile defaults.
    // Must be called before the user can launch a run.
    // strategyDefId / strategyVersion come from the strategy catalog (may be empty
    // for legacy nodes that haven't been through loadFromDb orphan repair yet).
    void selectStrategy(const QString& strategyId,
                        const QString& displayName,
                        const QString& portfolioPath,
                        const Backtest::BacktestProfile& profile,
                        const QString& pipelineConfigJson,
                        const QString& strategyDefId   = {},
                        int            strategyVersion = 1);

    // Populate all result tabs from a fully-loaded run (from DB or just finished).
    // run.histBars carries the OHLC data needed by the Candlestick chart.
    void displayResult(const Backtest::BacktestLoadedRun& run);

    // Reload the Run History panel for the current strategy (called after a new run completes).
    void setRunHistory(const QList<DbBacktestRunSummary>& runs);

    // Progress/status from BacktestController (relayed to config panel)
    void setProgress(int percent);
    void setStatus(const QString& status);
    void setRunning(bool running);

signals:
    void runRequested(const Backtest::BacktestRunConfig& config);
    void loadRunRequested(const QString& runId);

private:
    void buildDock();
    void updateStrategyHeader();

    QString m_currentStrategyId;
    QString m_currentDisplayName;
    QString m_currentPortfolioPath;
    QString m_currentStrategyDefId;
    int     m_currentStrategyVersion = 1;

    QLabel*                  m_headerLabel   = nullptr;

    BacktestRunConfigPanel*  m_configPanel   = nullptr;
    QTabWidget*              m_tabWidget     = nullptr;

    BacktestRunHistoryPanel* m_historyPanel  = nullptr;
    EquityChartWidget*       m_equityChart   = nullptr;
    BacktestCandlestickWidget* m_candleChart = nullptr;
    TradeLogWidget*          m_tradeLog      = nullptr;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTWORKSPACEDOCK_H
