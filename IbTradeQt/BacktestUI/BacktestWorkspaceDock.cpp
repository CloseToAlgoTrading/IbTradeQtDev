#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestPresenter.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "BacktestUI/BacktestRunHistoryPanel.h"
#include "BacktestUI/EquityChartWidget.h"
#include "BacktestUI/BacktestSummaryStatisticsPanel.h"
#include "BacktestUI/BacktestCandlestickWidget.h"
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/BacktestSummaryFormatter.h"
#include "Pipeline/StrategyRuntimePolicy.h"
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
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <cmath>

namespace BacktestUI {

namespace {

QString resolveBenchmarkSymbolForDisplay(const Backtest::BacktestLoadedRun& run,
                                         const QJsonObject& workingPipeline,
                                         const QString& panelBenchmarkSymbol)
{
    QString sym = run.result.benchmark.symbol.trimmed().toUpper();
    if (!sym.isEmpty())
        return sym;
    if (!run.record.configJson.isEmpty()) {
        const QJsonObject o = QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
        sym = Backtest::BacktestRunConfig::fromJson(o).benchmarkSymbol.trimmed().toUpper();
    }
    if (sym.isEmpty()) {
        sym = panelBenchmarkSymbol.trimmed().toUpper();
    }
    if (sym.isEmpty()) {
        QJsonObject pipelineForProfile = workingPipeline;
        if (!run.record.configJson.isEmpty()) {
            const QJsonObject runCfg =
                QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
            const QString pcj = runCfg.value(QStringLiteral("pipelineConfigJson")).toString();
            if (!pcj.isEmpty()) {
                const QJsonDocument d = QJsonDocument::fromJson(pcj.toUtf8());
                if (d.isObject())
                    pipelineForProfile = d.object();
            }
        }
        sym = Backtest::BacktestProfile::fromJson(
                  pipelineForProfile.value(QStringLiteral("backtestProfile")).toObject())
                  .defaultBenchmark.trimmed()
                  .toUpper();
    }
    // Persisted runs may have benchmark metrics / equity but a blank symbol (legacy rows).
    if (sym.isEmpty()
        && Backtest::BacktestSummaryFormatter::hasBenchmarkComparisonData(run.result)) {
        sym = QStringLiteral("SPY");
    }
    return sym;
}

} // namespace

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
    m_equityTab    = new QWidget();
    {
        auto* equityLayout = new QVBoxLayout(m_equityTab);
        equityLayout->setContentsMargins(0, 0, 0, 0);
        equityLayout->setSpacing(4);
        m_equityChart = new EquityChartWidget();
        m_summaryStatisticsPanel = new BacktestSummaryStatisticsPanel();
        equityLayout->addWidget(m_equityChart, 1);
        equityLayout->addWidget(m_summaryStatisticsPanel, 0);
    }
    m_candleChart  = new BacktestCandlestickWidget();
    m_tradeLog     = new TradeLogWidget();

    m_tabWidget->addTab(m_historyPanel, QStringLiteral("Run History"));
    m_tabWidget->addTab(m_equityTab,      QStringLiteral("Equity Curve"));
    m_tabWidget->addTab(m_candleChart,  QStringLiteral("Candlestick"));
    m_tabWidget->addTab(m_tradeLog,     QStringLiteral("Trade Log"));

    outerLayout->addWidget(m_tabWidget, 1);
    setWidget(container);

    connect(m_configPanel, &BacktestRunConfigPanel::runRequested,
            this, &BacktestWorkspaceDock::runRequested);
    connect(m_configPanel, &BacktestRunConfigPanel::prepareRunRequested,
            this, &BacktestWorkspaceDock::prepareRunRequested);
    connect(m_configPanel, &BacktestRunConfigPanel::stopRequested,
            this, &BacktestWorkspaceDock::stopBacktestRequested);

    connect(m_historyPanel, &BacktestRunHistoryPanel::loadRunRequested,
            this, &BacktestWorkspaceDock::loadRunRequested);
    connect(m_historyPanel, &BacktestRunHistoryPanel::deleteRunRequested,
            this, &BacktestWorkspaceDock::deleteRunRequested);
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
        if (m_summaryStatisticsPanel)
            m_summaryStatisticsPanel->clear();
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
            "<i>Results shown are from the last stored run and do not reflect the current "
            "temporary workspace changes yet.</i>"));
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

void BacktestWorkspaceDock::displayResult(const Backtest::BacktestLoadedRun& run,
                                          const QString& statusAfterRendering)
{
    const QString panelBench =
        m_configPanel ? m_configPanel->currentConfig().benchmarkSymbol : QString();
    const QString benchSym =
        resolveBenchmarkSymbolForDisplay(run, m_pipelineConfig, panelBench);

    QJsonObject pipelineForSummary = m_pipelineConfig;
    if (!run.record.configJson.isEmpty()) {
        const QJsonObject runCfg =
            QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
        const QString pcj = runCfg.value(QStringLiteral("pipelineConfigJson")).toString();
        if (!pcj.isEmpty()) {
            const QJsonDocument d = QJsonDocument::fromJson(pcj.toUtf8());
            if (d.isObject())
                pipelineForSummary = d.object();
        }
    }

    setResultRendering(true);
    const Backtest::BacktestLoadedRun runCopy = run;
    auto* watcher = new QFutureWatcher<PreparedBacktestDisplay>(this);
    connect(watcher, &QFutureWatcher<PreparedBacktestDisplay>::finished, this,
            [this, watcher, statusAfterRendering]() {
                PreparedBacktestDisplay p = watcher->result();
                watcher->deleteLater();
                applyPreparedDisplay(p);
                setResultRendering(false, statusAfterRendering);
            });
    watcher->setFuture(QtConcurrent::run([runCopy, benchSym, pipelineForSummary]() {
        return prepareBacktestDisplay(runCopy, benchSym, pipelineForSummary);
    }));
}

void BacktestWorkspaceDock::applyPreparedDisplay(const PreparedBacktestDisplay& p)
{
    const bool hasBm = !p.benchmarkPnl.isEmpty() || p.threeColSummary;
    m_equityChart->setPnlSeriesData(p.strategyPnl, p.benchmarkPnl, p.benchmarkSymbol, hasBm);
    m_summaryStatisticsPanel->setSummaryRows(p.summaryRows, p.threeColSummary,
                                               p.benchmarkColumnHeader);
    m_candleChart->setData(p.histBars, p.dbFills);
    m_tradeLog->setFills(p.tradeLog);
    m_tabWidget->setCurrentIndex(2);
}

void BacktestWorkspaceDock::setResultRendering(bool on, const QString& statusWhenDone)
{
    if (m_configPanel)
        m_configPanel->setRenderingResults(on, statusWhenDone);
}

void BacktestWorkspaceDock::applyLoadedRunConfiguration(const Backtest::BacktestLoadedRun& run)
{
    if (!m_configPanel || run.record.configJson.isEmpty())
        return;
    const QJsonObject o = QJsonDocument::fromJson(run.record.configJson.toUtf8()).object();
    const Backtest::BacktestRunConfig rc = Backtest::BacktestRunConfig::fromJson(o);
    m_configPanel->applyRunConfigFields(rc);
}

void BacktestWorkspaceDock::setRunHistory(const QList<DbBacktestRunSummary>& runs) {
    m_historyPanel->setRuns(runs);
}

void BacktestWorkspaceDock::clearDisplayedBacktestResult()
{
    m_equityChart->clear();
    if (m_summaryStatisticsPanel)
        m_summaryStatisticsPanel->clear();
    m_candleChart->clear();
    m_tradeLog->clear();
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
