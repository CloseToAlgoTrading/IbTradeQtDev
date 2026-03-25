#ifndef BACKTESTUI_BACKTESTWORKSPACEDOCK_H
#define BACKTESTUI_BACKTESTWORKSPACEDOCK_H

#include <QDockWidget>
#include <QList>
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/BacktestWorkspaceSession.h"
#include "BacktestResultDisplayPreparer.h"
#include "DB/dbdatatypes.h"

class QLabel;
class QTabWidget;
class QWidget;
class QPushButton;
class QHBoxLayout;
class BlockInspectorPanel;
class BacktestPresenter;

namespace BacktestUI {

class BacktestRunConfigPanel;
class BacktestRunHistoryPanel;
class EquityChartWidget;
class BacktestSummaryStatisticsPanel;
class BacktestCandlestickWidget;
class TradeLogWidget;

class BacktestWorkspaceDock : public QDockWidget {
    Q_OBJECT

public:
    explicit BacktestWorkspaceDock(QWidget* parent = nullptr);

    void applyWorkspaceSession(const Backtest::Workspace::Session& session,
                               bool clearResultPanels);

    void displayResult(const Backtest::BacktestLoadedRun& run,
                       const QString& statusAfterRendering = QString());

    /// Restore Run Configuration tab from a persisted run (historical row).
    void applyLoadedRunConfiguration(const Backtest::BacktestLoadedRun& run);

    void setRunHistory(const QList<DbBacktestRunSummary>& runs);

    void setProgress(int percent);
    void setStatus(const QString& status);
    void setRunning(bool running);

    void showBlockDetails(const QString& category, const QString& jsonKey,
                          bool isArray, int arrayIndex,
                          const QJsonObject& pipelineConfig);
    void hideBlockDetails();

    void setResultsStale(bool stale);
    void setSessionDirtyState(bool dirty);

    void setResultRendering(bool on, const QString& statusWhenDone = QString());

    BacktestRunConfigPanel* runConfigPanel() const { return m_configPanel; }

signals:
    void runRequested(const Backtest::BacktestRunConfig& config);
    void prepareRunRequested(const Backtest::BacktestRunConfig& config);
    void stopBacktestRequested();
    void loadRunRequested(const QString& runId);

    void userWorkspacePipelineEdited(const QJsonObject& newWorkingPipeline);
    void userRunFieldsEdited();

    void saveChangesRequested();
    void resetToBaselineRequested();
    void saveAsNewVersionRequested();

private:
    void buildDock();
    void updateStrategyHeader();
    void rebuildActionRow();
    void applyPreparedDisplay(const PreparedBacktestDisplay& prepared);

    QString m_currentStrategyId;
    QString m_currentDisplayName;
    QString m_currentPortfolioPath;
    QString m_currentStrategyDefId;
    int     m_currentStrategyVersion = 1;
    bool    m_isCatalogPreview       = false;

    QLabel*                  m_headerLabel     = nullptr;
    QLabel*                  m_staleLabel      = nullptr;
    QHBoxLayout*             m_actionRowLayout = nullptr;
    QPushButton*             m_saveBtn         = nullptr;
    QPushButton*             m_resetBtn        = nullptr;
    QPushButton*             m_saveVerBtn      = nullptr;

    QTabWidget*              m_tabWidget       = nullptr;
    BacktestRunConfigPanel*  m_configPanel     = nullptr;
    BlockInspectorPanel*     m_inspector       = nullptr;
    int                      m_inspectorTabIdx = -1;

    BacktestRunHistoryPanel* m_historyPanel    = nullptr;
    QWidget*                 m_equityTab       = nullptr;
    EquityChartWidget*       m_equityChart     = nullptr;
    BacktestSummaryStatisticsPanel* m_summaryStatisticsPanel = nullptr;
    BacktestCandlestickWidget* m_candleChart   = nullptr;
    TradeLogWidget*          m_tradeLog        = nullptr;

    QJsonObject              m_pipelineConfig;
    bool                     m_programmaticDockUpdate = false;
    bool                     m_sessionDirty         = false;

    BacktestPresenter*       m_btPresenter    = nullptr;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTWORKSPACEDOCK_H
