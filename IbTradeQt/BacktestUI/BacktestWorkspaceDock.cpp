#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "BacktestUI/BacktestRunHistoryPanel.h"
#include "BacktestUI/EquityChartWidget.h"
#include "BacktestUI/BacktestCandlestickWidget.h"
#include "BacktestUI/TradeLogWidget.h"
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QFont>
#include <QSizePolicy>

namespace BacktestUI {

BacktestWorkspaceDock::BacktestWorkspaceDock(QWidget* parent)
    : QDockWidget(QStringLiteral("Backtest Workspace"), parent)
{
    setFeatures(QDockWidget::DockWidgetMovable  |
                QDockWidget::DockWidgetFloatable |
                QDockWidget::DockWidgetClosable);
    // Make the dock reasonably large by default so charts are visible
    setMinimumSize(700, 500);
    buildDock();
}

void BacktestWorkspaceDock::buildDock() {
    auto* container = new QWidget(this);
    auto* outerLayout = new QVBoxLayout(container);
    outerLayout->setContentsMargins(6, 6, 6, 6);
    outerLayout->setSpacing(4);

    // Strategy header (fixed height)
    m_headerLabel = new QLabel(QStringLiteral("No strategy selected"));
    m_headerLabel->setStyleSheet(
        QStringLiteral("font-weight: bold; font-size: 13px; "
                        "padding: 4px; background: #f0f4f8; "
                        "border-radius: 4px;"));
    m_headerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    outerLayout->addWidget(m_headerLabel);

    // Vertical splitter: config panel (top, collapsible) | results tabs (bottom, expands)
    auto* splitter = new QSplitter(Qt::Vertical, container);
    splitter->setChildrenCollapsible(false);

    // Config panel
    m_configPanel = new BacktestRunConfigPanel();
    m_configPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    splitter->addWidget(m_configPanel);

    // Tab widget for results
    m_tabWidget = new QTabWidget();
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_historyPanel = new BacktestRunHistoryPanel();
    m_equityChart  = new EquityChartWidget();
    m_candleChart  = new BacktestCandlestickWidget();
    m_tradeLog     = new TradeLogWidget();

    m_tabWidget->addTab(m_historyPanel, QStringLiteral("Run History"));
    m_tabWidget->addTab(m_equityChart,  QStringLiteral("Equity Curve"));
    m_tabWidget->addTab(m_candleChart,  QStringLiteral("Candlestick"));
    m_tabWidget->addTab(m_tradeLog,     QStringLiteral("Trade Log"));

    splitter->addWidget(m_tabWidget);

    // Give most space to the results area (1:3 ratio)
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    outerLayout->addWidget(splitter, 1);

    setWidget(container);

    // Wire internal signals upward to CPresenter
    connect(m_configPanel, &BacktestRunConfigPanel::runRequested,
            this, &BacktestWorkspaceDock::runRequested);

    connect(m_historyPanel, &BacktestRunHistoryPanel::loadRunRequested,
            this, &BacktestWorkspaceDock::loadRunRequested);
}

void BacktestWorkspaceDock::selectStrategy(const QString& strategyId,
                                            const QString& displayName,
                                            const QString& portfolioPath,
                                            const Backtest::BacktestProfile& profile,
                                            const QString& pipelineConfigJson) {
    m_currentStrategyId   = strategyId;
    m_currentDisplayName  = displayName;
    m_currentPortfolioPath = portfolioPath;

    updateStrategyHeader();

    m_configPanel->setStrategyContext(strategyId, displayName, portfolioPath, pipelineConfigJson);
    m_configPanel->applyProfile(profile);

    // Clear result tabs since a different strategy is now selected
    m_equityChart->clear();
    m_candleChart->clear();
    m_tradeLog->clear();
    m_historyPanel->clear();
}

void BacktestWorkspaceDock::updateStrategyHeader() {
    const QString text = QString(
        "<b>%1</b>"
        "<span style='color:#777; font-size:11px;'>&nbsp;&nbsp;%2</span>"
        "<span style='color:#aaa; font-size:10px;'>&nbsp;|&nbsp;ID: %3</span>")
        .arg(m_currentDisplayName)
        .arg(m_currentPortfolioPath)
        .arg(m_currentStrategyId.left(8));
    m_headerLabel->setText(text);
}

void BacktestWorkspaceDock::displayResult(const Backtest::BacktestLoadedRun& run) {
    const auto& result = run.result;

    // Equity curve
    m_equityChart->setData(
        result.equityCurve,
        result.benchmark.equityCurve,
        result.benchmark.symbol);

    // Candlestick chart — bars come from run.histBars (populated by HistoricalDataManager)
    {
        QList<DbBacktestTrade> dbFills;
        for (const auto& fill : result.tradeLog) {
            DbBacktestTrade t;
            t.runId     = run.record.runId;
            t.symbol    = fill.symbol;
            t.side      = fill.quantity > 0 ? QStringLiteral("BUY") : QStringLiteral("SELL");
            t.quantity  = std::abs(fill.quantity);
            t.fillPrice = fill.fillPrice;
            t.timestamp = fill.timestamp.toUTC().toString(Qt::ISODate);
            dbFills.append(t);
        }
        m_candleChart->setData(run.histBars, dbFills);
    }

    // Trade log
    m_tradeLog->setFills(result.tradeLog);

    // Switch to Equity Curve tab to show result immediately
    m_tabWidget->setCurrentIndex(1);
}

void BacktestWorkspaceDock::setRunHistory(const QList<DbBacktestRunSummary>& runs) {
    m_historyPanel->setRuns(runs);
}

void BacktestWorkspaceDock::setProgress(int percent) {
    m_configPanel->setProgress(percent);
}

void BacktestWorkspaceDock::setStatus(const QString& status) {
    m_configPanel->setStatus(status);
}

void BacktestWorkspaceDock::setRunning(bool running) {
    m_configPanel->setRunning(running);
}

} // namespace BacktestUI
