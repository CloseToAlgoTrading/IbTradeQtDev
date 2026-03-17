#ifndef STRATEGYWORKSPACE_H
#define STRATEGYWORKSPACE_H

#include "WorkspaceBase.h"

class QFormLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
class EventLogPanel;

class StrategyWorkspace : public WorkspaceBase
{
    Q_OBJECT
public:
    explicit StrategyWorkspace(QWidget* parent = nullptr);

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
    void buildBacktestTab();

    void refreshOverview();
    void refreshProperties();
    void refreshInfo();
    void refreshAssets();

    // Overview
    QWidget*     m_overviewWidget   = nullptr;
    QLabel*      m_ovStateSummary   = nullptr;
    QLabel*      m_ovCurrentPos     = nullptr;
    QLabel*      m_ovLatestSignal   = nullptr;
    QLabel*      m_ovWarnings       = nullptr;

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

    // Backtest placeholder
    QWidget*     m_backtestWidget   = nullptr;
};

#endif // STRATEGYWORKSPACE_H
