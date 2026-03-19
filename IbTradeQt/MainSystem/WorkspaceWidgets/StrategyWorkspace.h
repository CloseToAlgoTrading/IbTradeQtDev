#ifndef STRATEGYWORKSPACE_H
#define STRATEGYWORKSPACE_H

#include "WorkspaceBase.h"

class QFormLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
class EventLogPanel;
class BlockInspectorPanel;
class RuntimePolicyEditor;

class StrategyWorkspace : public WorkspaceBase
{
    Q_OBJECT
public:
    explicit StrategyWorkspace(QWidget* parent = nullptr);

    void showBlockInProperties(const QString& category,
                               const QString& jsonKey,
                               bool isArray, int arrayIndex);
    void restoreStrategyProperties();

protected:
    void onContextSet() override;
    void onContextCleared() override;
    void refreshMetrics() override;

private:
    void buildOverviewTab();
    void buildPropertiesTab();
    void buildInfoTab();
    void buildLogsTab();
    void buildAssetsTab();
    void buildPolicyTab();

    void refreshOverview();
    void refreshProperties();
    void refreshInfo();
    void refreshAssets();
    void refreshPolicy();

    // Overview
    QWidget*     m_overviewWidget   = nullptr;
    QLabel*      m_ovStateSummary   = nullptr;
    QLabel*      m_ovCurrentPos     = nullptr;
    QLabel*      m_ovLatestSignal   = nullptr;
    QLabel*      m_ovWarnings       = nullptr;
    QLabel*      m_ovEvalMode       = nullptr;
    QLabel*      m_ovRebalMode      = nullptr;

    // Properties
    QWidget*     m_propertiesWidget = nullptr;
    QFormLayout* m_propertiesForm   = nullptr;

    // Info
    QWidget*     m_infoWidget       = nullptr;
    QFormLayout* m_infoForm         = nullptr;

    // Logs
    QWidget*     m_logsWidget       = nullptr;
    QTextEdit*   m_logsText         = nullptr;

    // Assets
    QWidget*     m_assetsWidget     = nullptr;
    QLineEdit*   m_assetsEdit       = nullptr;
    QLabel*      m_universeInfoLabel = nullptr;

    // Policy
    RuntimePolicyEditor* m_policyEditor = nullptr;

    // Block inspector (shown inside Properties tab when a block is selected)
    BlockInspectorPanel* m_inspector   = nullptr;
    bool                 m_showingBlock = false;
    QWidget*             m_propertiesScroll = nullptr;
};

#endif // STRATEGYWORKSPACE_H
