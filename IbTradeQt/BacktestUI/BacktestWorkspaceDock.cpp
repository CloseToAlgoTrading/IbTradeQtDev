#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestPresenter.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "BacktestUI/BacktestRunHistoryPanel.h"
#include "BacktestUI/EquityChartWidget.h"
#include "BacktestUI/BacktestCandlestickWidget.h"
#include "BacktestUI/TradeLogWidget.h"
#include "BlockInspectorPanel.h"
#include "ThemePalette.h"
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QFont>
#include <QSizePolicy>
#include <QJsonDocument>
#include <QJsonObject>

namespace BacktestUI {

BacktestWorkspaceDock::BacktestWorkspaceDock(QWidget* parent)
    : QDockWidget(QStringLiteral("Backtest Workspace"), parent)
{
    setObjectName(QStringLiteral("BacktestWorkspaceDock"));
    m_btPresenter = new BacktestPresenter(this);
    setFeatures(QDockWidget::DockWidgetMovable  |
                QDockWidget::DockWidgetFloatable |
                QDockWidget::DockWidgetClosable);
    setMinimumWidth(UiTheme::kBacktestWorkspaceMinWidth);
    setMinimumHeight(UiTheme::kBacktestWorkspaceMinHeight);
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

    m_staleLabel = new QLabel(QString());
    m_staleLabel->setObjectName(QStringLiteral("BacktestStaleResultsLabel"));
    m_staleLabel->setWordWrap(true);
    m_staleLabel->setVisible(false);
    outerLayout->addWidget(m_staleLabel);

    m_actionRowLayout = new QHBoxLayout();
    m_actionRowLayout->setSpacing(4);
    m_saveBtn = new QPushButton(QStringLiteral("Save Changes"));
    m_resetBtn = new QPushButton(QStringLiteral("Reset to Baseline"));
    m_saveVerBtn = new QPushButton(QStringLiteral("Save as New Version"));
    m_actionRowLayout->addWidget(m_saveBtn);
    m_actionRowLayout->addWidget(m_resetBtn);
    m_actionRowLayout->addWidget(m_saveVerBtn);
    m_actionRowLayout->addStretch();
    outerLayout->addLayout(m_actionRowLayout);

    connect(m_saveBtn, &QPushButton::clicked, this, &BacktestWorkspaceDock::saveChangesRequested);
    connect(m_resetBtn, &QPushButton::clicked, this, &BacktestWorkspaceDock::resetToBaselineRequested);
    connect(m_saveVerBtn, &QPushButton::clicked, this, &BacktestWorkspaceDock::saveAsNewVersionRequested);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setObjectName(QStringLiteral("BacktestWorkspaceMainTabs"));
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_configPanel = new BacktestRunConfigPanel();
    m_tabWidget->addTab(m_configPanel, QStringLiteral("Run Configuration"));

    m_inspector = new BlockInspectorPanel();
    m_inspector->setDiffPanelVisible(true);

    connect(m_inspector, &BlockInspectorPanel::configChanged,
            this, [this](const QJsonObject& newConfig) {
        if (m_programmaticDockUpdate)
            return;
        emit userWorkspacePipelineEdited(newConfig);
    });

    connect(m_configPanel, &BacktestRunConfigPanel::userEdited,
            this, &BacktestWorkspaceDock::userRunFieldsEdited);

    m_historyPanel = new BacktestRunHistoryPanel();
    m_equityChart  = new EquityChartWidget();
    m_candleChart  = new BacktestCandlestickWidget();
    m_tradeLog     = new TradeLogWidget();

    m_tabWidget->addTab(m_historyPanel, QStringLiteral("Run History"));
    m_tabWidget->addTab(m_equityChart,  QStringLiteral("Equity Curve"));
    m_tabWidget->addTab(m_candleChart,  QStringLiteral("Candlestick"));
    m_tabWidget->addTab(m_tradeLog,     QStringLiteral("Trade Log"));

    outerLayout->addWidget(m_tabWidget, 1);
    setWidget(container);

    connect(m_configPanel, &BacktestRunConfigPanel::runRequested,
            this, &BacktestWorkspaceDock::runRequested);

    connect(m_historyPanel, &BacktestRunHistoryPanel::loadRunRequested,
            this, &BacktestWorkspaceDock::loadRunRequested);
}

void BacktestWorkspaceDock::applyWorkspaceSession(const Backtest::Workspace::Session& session,
                                                 bool clearResultPanels) {
    m_programmaticDockUpdate = true;

    m_currentStrategyId = session.key.kind == Backtest::Workspace::SessionKind::LiveNode
        ? session.key.nodeId
        : QString();
    m_isCatalogPreview = (session.key.kind == Backtest::Workspace::SessionKind::CatalogVersion);
    m_currentDisplayName     = session.displayName;
    m_currentPortfolioPath   = session.portfolioPath;
    m_currentStrategyDefId   = session.strategyDefId;
    m_currentStrategyVersion = session.strategyVersion > 0 ? session.strategyVersion : 1;

    const QString pipeJson = QString::fromUtf8(
        QJsonDocument(session.workingPipeline).toJson(QJsonDocument::Compact));

    m_configPanel->setStrategyContext(
        m_currentStrategyId, session.displayName, session.portfolioPath,
        pipeJson, session.strategyDefId, m_currentStrategyVersion);
    m_configPanel->setCatalogVersionId(session.catalogVersionId);

    const Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            session.workingPipeline.value(QStringLiteral("backtestProfile")).toObject());
    m_configPanel->applyProfile(profile);

    m_configPanel->applyRunFieldsSnapshot(session.workingRunFields);

    m_pipelineConfig = session.workingPipeline;
    hideBlockDetails();

    if (clearResultPanels) {
        m_equityChart->clear();
        m_candleChart->clear();
        m_tradeLog->clear();
        m_historyPanel->clear();
    }

    m_sessionDirty = session.dirty;
    setResultsStale(session.resultsStale);
    updateStrategyHeader();
    rebuildActionRow();

    m_tabWidget->setCurrentIndex(0);
    m_programmaticDockUpdate = false;
}

void BacktestWorkspaceDock::setSessionDirtyState(bool dirty)
{
    m_sessionDirty = dirty;
    updateStrategyHeader();
}

void BacktestWorkspaceDock::setResultsStale(bool stale)
{
    m_staleLabel->setVisible(stale);
    if (stale) {
        m_staleLabel->setText(QStringLiteral(
            "<i>Results shown are from the last run and may be outdated.</i>"));
    }
}

void BacktestWorkspaceDock::rebuildActionRow()
{
    m_saveBtn->setVisible(!m_isCatalogPreview);
    m_resetBtn->setText(m_isCatalogPreview
        ? QStringLiteral("Reset to Version Baseline")
        : QStringLiteral("Reset to Node Baseline"));
}

void BacktestWorkspaceDock::updateStrategyHeader() {
    QString versionBadge;
    if (!m_currentStrategyDefId.isEmpty()) {
        versionBadge = QString(
            "<span style='color:#6eb3f0; font-size:10px;'>"
            "&nbsp;|&nbsp;def v%1</span>")
            .arg(m_currentStrategyVersion);
    }
    const QString idShown = m_currentStrategyId.isEmpty()
        ? m_currentStrategyDefId.left(8)
        : m_currentStrategyId.left(8);
    QString dirtyTag;
    if (m_sessionDirty) {
        dirtyTag = QStringLiteral(
            "<span style='color:#ffb347; font-size:11px;'>&nbsp;|&nbsp;Modified</span>");
    }
    const QString text = QString(
        "<b style='color:#f0f0f0;'>%1</b>"
        "<span style='color:#b8b8b8; font-size:11px;'>&nbsp;&nbsp;%2</span>"
        "<span style='color:#909090; font-size:10px;'>&nbsp;|&nbsp;ID: %3</span>"
        "%4%5")
        .arg(m_currentDisplayName)
        .arg(m_currentPortfolioPath)
        .arg(idShown)
        .arg(versionBadge)
        .arg(dirtyTag);
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
    m_tabWidget->setCurrentIndex(2);
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
        m_inspectorTabIdx = m_tabWidget->addTab(m_inspector, QStringLiteral("Block Details"));
    }
    m_tabWidget->setCurrentIndex(m_inspectorTabIdx);
    m_inspector->showBlock(m_pipelineConfig, category, jsonKey, isArray, arrayIndex);
}

void BacktestWorkspaceDock::hideBlockDetails() {
    if (m_inspectorTabIdx >= 0) {
        m_tabWidget->removeTab(m_inspectorTabIdx);
        m_inspectorTabIdx = -1;
    }
}

} // namespace BacktestUI
