#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestPresenter.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "BacktestUI/BacktestRunHistoryPanel.h"
#include "BacktestUI/EquityChartWidget.h"
#include "BacktestUI/BacktestCandlestickWidget.h"
#include "BacktestUI/TradeLogWidget.h"
#include "BlockInspectorPanel.h"
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QFont>
#include <QSizePolicy>
#include <QJsonDocument>
#include <QJsonObject>

namespace BacktestUI {

BacktestWorkspaceDock::BacktestWorkspaceDock(QWidget* parent)
    : QDockWidget(QStringLiteral("Backtest Workspace"), parent)
{
    m_btPresenter = new BacktestPresenter(this);
    setFeatures(QDockWidget::DockWidgetMovable  |
                QDockWidget::DockWidgetFloatable |
                QDockWidget::DockWidgetClosable);
    setMinimumSize(700, 500);
    buildDock();
}

void BacktestWorkspaceDock::buildDock() {
    auto* container = new QWidget(this);
    auto* outerLayout = new QVBoxLayout(container);
    outerLayout->setContentsMargins(6, 6, 6, 6);
    outerLayout->setSpacing(4);

    m_headerLabel = new QLabel(QStringLiteral("No strategy selected"));
    m_headerLabel->setObjectName(QStringLiteral("BacktestWorkspaceContextHeader"));
    m_headerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    outerLayout->addWidget(m_headerLabel);

    auto* splitter = new QSplitter(Qt::Vertical, container);
    splitter->setChildrenCollapsible(false);

    // Top: tabbed config area — tab underline styled in operations-console.qss
    m_configTabs = new QTabWidget();
    m_configTabs->setObjectName(QStringLiteral("BacktestWorkspaceConfigTabs"));
    m_configTabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_configPanel = new BacktestRunConfigPanel();
    m_configTabs->addTab(m_configPanel, QStringLiteral("Run Configuration"));

    m_inspector = new BlockInspectorPanel();
    m_inspector->setDiffPanelVisible(true);

    // Inspector param edits update stored config and run config panel
    connect(m_inspector, &BlockInspectorPanel::configChanged,
            this, [this](const QJsonObject& newConfig) {
        m_pipelineConfig = newConfig;
        QString json = QString::fromUtf8(
            QJsonDocument(newConfig).toJson(QJsonDocument::Compact));
        m_configPanel->setStrategyContext(
            m_currentStrategyId, m_currentDisplayName,
            m_currentPortfolioPath, json,
            m_currentStrategyDefId, m_currentStrategyVersion);
    });

    splitter->addWidget(m_configTabs);

    // Bottom: results tabs — same QSS object as config tabs
    m_tabWidget = new QTabWidget();
    m_tabWidget->setObjectName(QStringLiteral("BacktestWorkspaceResultTabs"));
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
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    outerLayout->addWidget(splitter, 1);
    setWidget(container);

    connect(m_configPanel, &BacktestRunConfigPanel::runRequested,
            this, &BacktestWorkspaceDock::runRequested);

    connect(m_historyPanel, &BacktestRunHistoryPanel::loadRunRequested,
            this, &BacktestWorkspaceDock::loadRunRequested);
}

void BacktestWorkspaceDock::selectStrategy(const QString& strategyId,
                                            const QString& displayName,
                                            const QString& portfolioPath,
                                            const Backtest::BacktestProfile& profile,
                                            const QString& pipelineConfigJson,
                                            const QString& strategyDefId,
                                            int            strategyVersion,
                                            const QString& catalogVersionId) {
    m_currentStrategyId      = strategyId;
    m_currentDisplayName     = displayName;
    m_currentPortfolioPath   = portfolioPath;
    m_currentStrategyDefId   = strategyDefId;
    m_currentStrategyVersion = strategyVersion > 0 ? strategyVersion : 1;

    updateStrategyHeader();

    m_configPanel->setStrategyContext(strategyId, displayName, portfolioPath,
                                      pipelineConfigJson,
                                      strategyDefId, m_currentStrategyVersion);
    m_configPanel->setCatalogVersionId(catalogVersionId);
    m_configPanel->applyProfile(profile);

    m_pipelineConfig = QJsonDocument::fromJson(pipelineConfigJson.toUtf8()).object();
    hideBlockDetails();

    m_equityChart->clear();
    m_candleChart->clear();
    m_tradeLog->clear();
    m_historyPanel->clear();
}

void BacktestWorkspaceDock::updateStrategyHeader() {
    QString versionBadge;
    if (!m_currentStrategyDefId.isEmpty()) {
        versionBadge = QString(
            "<span style='color:#6eb3f0; font-size:10px;'>"
            "&nbsp;|&nbsp;def v%1</span>")
            .arg(m_currentStrategyVersion);
    }
    /* Colors tuned for dark header (see #BacktestWorkspaceContextHeader in operations-console.qss) */
    const QString text = QString(
        "<b style='color:#f0f0f0;'>%1</b>"
        "<span style='color:#b8b8b8; font-size:11px;'>&nbsp;&nbsp;%2</span>"
        "<span style='color:#909090; font-size:10px;'>&nbsp;|&nbsp;ID: %3</span>"
        "%4")
        .arg(m_currentDisplayName)
        .arg(m_currentPortfolioPath)
        .arg(m_currentStrategyId.left(8))
        .arg(versionBadge);
    m_headerLabel->setText(text);
}

void BacktestWorkspaceDock::displayResult(const Backtest::BacktestLoadedRun& run) {
    const auto& result = run.result;

    m_equityChart->setData(
        result.equityCurve,
        result.benchmark.equityCurve,
        result.benchmark.symbol);

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

    m_tradeLog->setFills(result.tradeLog);
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

void BacktestWorkspaceDock::showBlockDetails(const QString& category,
                                              const QString& jsonKey,
                                              bool isArray, int arrayIndex,
                                              const QJsonObject& pipelineConfig) {
    m_pipelineConfig = pipelineConfig;
    if (m_inspectorTabIdx < 0) {
        m_inspectorTabIdx = m_configTabs->addTab(m_inspector, QStringLiteral("Block Details"));
    }
    m_configTabs->setCurrentIndex(m_inspectorTabIdx);
    m_inspector->showBlock(m_pipelineConfig, category, jsonKey, isArray, arrayIndex);
}

void BacktestWorkspaceDock::hideBlockDetails() {
    if (m_inspectorTabIdx >= 0) {
        m_configTabs->removeTab(m_inspectorTabIdx);
        m_inspectorTabIdx = -1;
    }
}

} // namespace BacktestUI
