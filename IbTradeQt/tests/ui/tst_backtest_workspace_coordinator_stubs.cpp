#include "ui/tst_backtest_workspace_coordinator.h"

namespace {

int g_displayedResultCount = 0;
int g_runHistorySetCount = 0;
QString g_lastDisplayedRunId;
int g_resultsStaleSetCount = 0;
bool g_lastResultsStale = false;
bool g_isEmptyStateVisible = false;
int g_lastDisplayedHistoricalSymbolCount = 0;

} // namespace

namespace BacktestWorkspaceCoordinatorTestProbe {

void reset()
{
    g_displayedResultCount = 0;
    g_runHistorySetCount = 0;
    g_lastDisplayedRunId.clear();
    g_resultsStaleSetCount = 0;
    g_lastResultsStale = false;
    g_isEmptyStateVisible = false;
    g_lastDisplayedHistoricalSymbolCount = 0;
}

int displayedResultCount() { return g_displayedResultCount; }
int runHistorySetCount() { return g_runHistorySetCount; }
QString lastDisplayedRunId() { return g_lastDisplayedRunId; }
int resultsStaleSetCount() { return g_resultsStaleSetCount; }
bool lastResultsStale() { return g_lastResultsStale; }
bool isEmptyStateVisible() { return g_isEmptyStateVisible; }
int lastDisplayedHistoricalSymbolCount() { return g_lastDisplayedHistoricalSymbolCount; }

} // namespace BacktestWorkspaceCoordinatorTestProbe

namespace BacktestUI {

BacktestWorkspaceDock::BacktestWorkspaceDock(QWidget* parent)
    : QDockWidget(parent)
{
    m_configPanel = new BacktestRunConfigPanel(this);
    setWidget(m_configPanel);
}

void BacktestWorkspaceDock::showEmptyState()
{
    g_isEmptyStateVisible = true;
    m_currentStrategyId.clear();
    m_currentDisplayName.clear();
    m_currentPortfolioPath.clear();
    m_currentStrategyDefId.clear();
    m_currentStrategyVersion = 1;
    m_isCatalogPreview = false;
    m_sessionDirty = false;
    m_pipelineConfig = QJsonObject();
}

void BacktestWorkspaceDock::applyWorkspaceSession(const Backtest::Workspace::Session& session,
                                                  bool)
{
    g_isEmptyStateVisible = false;
    m_currentStrategyId = session.key.kind == Backtest::Workspace::SessionKind::LiveNode
        ? session.key.nodeId
        : QString();
    m_currentDisplayName = session.displayName;
    m_currentPortfolioPath = session.portfolioPath;
    m_currentStrategyDefId = session.strategyDefId;
    m_currentStrategyVersion = session.strategyVersion > 0 ? session.strategyVersion : 1;
    m_isCatalogPreview = (session.key.kind == Backtest::Workspace::SessionKind::CatalogVersion);
    m_sessionDirty = session.dirty;
    m_pipelineConfig = session.workingPipeline;

    const QString pipeJson = QString::fromUtf8(
        QJsonDocument(session.workingPipeline).toJson(QJsonDocument::Compact));
    m_configPanel->setStrategyContext(
        m_currentStrategyId,
        session.displayName,
        session.portfolioPath,
        pipeJson,
        session.strategyDefId,
        m_currentStrategyVersion);
    m_configPanel->setCatalogVersionId(session.catalogVersionId);

    const Backtest::BacktestProfile profile =
        Backtest::BacktestProfile::fromJson(
            session.workingPipeline.value(QStringLiteral("backtestProfile")).toObject());
    m_configPanel->applyProfile(profile);
    m_configPanel->applyRunFieldsSnapshot(session.workingRunFields);
}

void BacktestWorkspaceDock::displayResult(const Backtest::BacktestLoadedRun& run, const QString&)
{
    ++g_displayedResultCount;
    g_lastDisplayedRunId = run.record.runId;
    g_lastDisplayedHistoricalSymbolCount = run.histBars.size();
}

void BacktestWorkspaceDock::applyLoadedRunConfiguration(const Backtest::BacktestLoadedRun& run)
{
    if (m_configPanel)
        m_configPanel->applyRunConfigFields(
            Backtest::BacktestRunConfig::fromJson(
                QJsonDocument::fromJson(run.record.configJson.toUtf8()).object()));
}

void BacktestWorkspaceDock::setRunHistory(const QList<DbBacktestRunSummary>&)
{
    ++g_runHistorySetCount;
}
void BacktestWorkspaceDock::clearDisplayedBacktestResult() {}
void BacktestWorkspaceDock::setProgress(int percent)
{
    if (m_configPanel)
        m_configPanel->setProgress(percent);
}
void BacktestWorkspaceDock::setStatus(const QString& status)
{
    if (m_configPanel)
        m_configPanel->setStatus(status);
}
void BacktestWorkspaceDock::setRunning(bool running)
{
    if (m_configPanel)
        m_configPanel->setRunning(running);
}
void BacktestWorkspaceDock::showBlockDetails(const QString&, const QString&, bool, int,
                                             const QJsonObject&) {}
void BacktestWorkspaceDock::hideBlockDetails() {}
void BacktestWorkspaceDock::setResultsStale(bool stale)
{
    ++g_resultsStaleSetCount;
    g_lastResultsStale = stale;
}
void BacktestWorkspaceDock::setSessionDirtyState(bool dirty) { m_sessionDirty = dirty; }
void BacktestWorkspaceDock::setResultRendering(bool on, const QString& statusWhenDone)
{
    if (m_configPanel)
        m_configPanel->setRenderingResults(on, statusWhenDone);
}

BacktestStrategySelector::BacktestStrategySelector(QWidget* parent)
    : QWidget(parent)
{
}

QTreeView* BacktestStrategySelector::strategyTreeView() const
{
    return nullptr;
}

void BacktestStrategySelector::populate(const QList<StrategyListItem>&) {}
void BacktestStrategySelector::populateCatalog(const QList<CatalogVersionItem>&) {}
void BacktestStrategySelector::highlightStrategy(const QString&) {}
void BacktestStrategySelector::onItemDoubleClicked(const QModelIndex&) {}
void BacktestStrategySelector::onSelectClicked() {}

} // namespace BacktestUI

int PreparePreflightDialog::run(QWidget*, const Backtest::BacktestPreFlightResult&, bool, bool*)
{
    return QDialog::Accepted;
}

YahooSymbolCheckDialog::Choice YahooSymbolCheckDialog::run(
    QWidget*, const QHash<QString, QString>&)
{
    return YahooSymbolCheckDialog::Choice::Cancelled;
}

void CIBTradeSystemView::switchToBacktestTab() {}
void CIBTradeSystemView::switchToDataManagementTab() {}
