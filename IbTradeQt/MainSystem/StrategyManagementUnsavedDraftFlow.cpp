#include "StrategyManagementUnsavedDraftFlow.h"
#include "ISystemBackend.h"
#include "ibtradesystemview.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"

#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>

StrategyManagementUnsavedDraftFlow::StrategyManagementUnsavedDraftFlow(
    ISystemBackend* backend,
    CIBTradeSystemView* view,
    StrategyMgmt::StrategyManagementPanel* panel)
    : m_backend(backend)
    , m_view(view)
    , m_panel(panel)
    , m_prompt(std::make_unique<QtUnsavedChangesPrompt>())
{
}

void StrategyManagementUnsavedDraftFlow::setPrompt(std::unique_ptr<IUnsavedChangesPrompt> prompt)
{
    if (prompt)
        m_prompt = std::move(prompt);
    else
        m_prompt = std::make_unique<QtUnsavedChangesPrompt>();
}

bool StrategyManagementUnsavedDraftFlow::tryResolveIfDirty(const QString& message)
{
    if (!m_backend || !m_view || !m_panel)
        return true;
    auto* detail = m_panel->detailPanel();
    if (!detail || !detail->isConfigDirty())
        return true;

    const UnsavedPromptChoice choice = m_prompt->askSaveDiscardCancel(
        m_view,
        QStringLiteral("Unsaved strategy changes"),
        message);

    if (choice == UnsavedPromptChoice::Cancel)
        return false;

    if (choice == UnsavedPromptChoice::Discard) {
        discardWorkingToSelectedVersion(detail);
        return true;
    }

    saveWorkingAsNewVersion();
    return true;
}

void StrategyManagementUnsavedDraftFlow::discardWorkingToSelectedVersion(
    StrategyMgmt::StrategyDetailPanel* detail)
{
    detail->resetWorkingToSelectedVersion();
}

void StrategyManagementUnsavedDraftFlow::saveWorkingAsNewVersion()
{
    auto* detail = m_panel->detailPanel();
    if (!detail)
        return;
    const QString strategyId = detail->currentStrategyId();
    if (strategyId.isEmpty())
        return;

    QString notes = QInputDialog::getText(
        m_view,
        QStringLiteral("New Version"),
        QStringLiteral("Notes for this version:"));

    m_backend->createStrategyVersion(strategyId, detail->workingConfig(), notes);

    const QJsonObject entry = m_backend->strategyCatalogEntry(strategyId);
    const QJsonArray versions = m_backend->listStrategyVersions(strategyId);
    m_panel->showStrategyDetail(entry, versions);
}
