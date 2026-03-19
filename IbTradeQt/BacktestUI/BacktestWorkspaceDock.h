#ifndef BACKTESTUI_BACKTESTWORKSPACEDOCK_H
#define BACKTESTUI_BACKTESTWORKSPACEDOCK_H

#include <QDockWidget>
#include <QMap>
#include <QList>
#include "Backtest/BacktestDataTypes.h"
#include "DB/dbdatatypes.h"

class QLabel;
class QTabWidget;
class QWidget;
class QSplitter;
class BlockInspectorPanel;
class BacktestPresenter;

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

    void selectStrategy(const QString& strategyId,
                        const QString& displayName,
                        const QString& portfolioPath,
                        const Backtest::BacktestProfile& profile,
                        const QString& pipelineConfigJson,
                        const QString& strategyDefId     = {},
                        int            strategyVersion   = 1,
                        const QString& catalogVersionId  = {});

    void displayResult(const Backtest::BacktestLoadedRun& run);
    void setRunHistory(const QList<DbBacktestRunSummary>& runs);

    void setProgress(int percent);
    void setStatus(const QString& status);
    void setRunning(bool running);

    void showBlockDetails(const QString& category, const QString& jsonKey,
                          bool isArray, int arrayIndex,
                          const QJsonObject& pipelineConfig);
    void hideBlockDetails();

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

    QLabel*                  m_headerLabel     = nullptr;

    QTabWidget*              m_configTabs      = nullptr;
    BacktestRunConfigPanel*  m_configPanel     = nullptr;
    BlockInspectorPanel*     m_inspector       = nullptr;
    int                      m_inspectorTabIdx = -1;
    QTabWidget*              m_tabWidget       = nullptr;

    BacktestRunHistoryPanel* m_historyPanel    = nullptr;
    EquityChartWidget*       m_equityChart     = nullptr;
    BacktestCandlestickWidget* m_candleChart   = nullptr;
    TradeLogWidget*          m_tradeLog        = nullptr;

    QJsonObject              m_pipelineConfig;
    BacktestPresenter*       m_btPresenter    = nullptr;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTWORKSPACEDOCK_H
