#ifndef STRATEGYMANAGEMENTUNSAVEDDRAFTFLOW_H
#define STRATEGYMANAGEMENTUNSAVEDDRAFTFLOW_H

#include "IUnsavedChangesPrompt.h"

#include <memory>

class ISystemBackend;
class CIBTradeSystemView;

namespace StrategyMgmt {
class StrategyManagementPanel;
class StrategyDetailPanel;
}

// Centralizes Save / Discard / Cancel for unsaved pipeline edits in Strategy Management.
class StrategyManagementUnsavedDraftFlow
{
public:
    StrategyManagementUnsavedDraftFlow(ISystemBackend* backend,
                                       CIBTradeSystemView* view,
                                       StrategyMgmt::StrategyManagementPanel* panel);

    void setPrompt(std::unique_ptr<IUnsavedChangesPrompt> prompt);

    // Returns true if navigation may proceed (not dirty, saved, or discarded).
    bool tryResolveIfDirty(const QString& message);

    void saveWorkingAsNewVersion();

private:
    void discardWorkingToSelectedVersion(StrategyMgmt::StrategyDetailPanel* detail);

    ISystemBackend*                        m_backend = nullptr;
    CIBTradeSystemView*                    m_view    = nullptr;
    StrategyMgmt::StrategyManagementPanel* m_panel   = nullptr;
    std::unique_ptr<IUnsavedChangesPrompt> m_prompt;
};

#endif // STRATEGYMANAGEMENTUNSAVEDDRAFTFLOW_H
