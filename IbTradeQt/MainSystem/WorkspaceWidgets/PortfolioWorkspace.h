#ifndef PORTFOLIOWORKSPACE_H
#define PORTFOLIOWORKSPACE_H

#include "WorkspaceBase.h"

class QFormLayout;
class QLabel;

class PortfolioWorkspace : public WorkspaceBase
{
    Q_OBJECT
public:
    explicit PortfolioWorkspace(QWidget* parent = nullptr);

protected:
    void onContextSet() override;
    void onContextCleared() override;
    void refreshMetrics() override;

private:
    void buildOverviewTab();
    void buildPropertiesTab();
    void buildInfoTab();
    void buildLogsTab();

    void refreshOverview();
    void refreshProperties();
    void refreshInfo();

    QWidget*     m_overviewWidget = nullptr;
    QLabel*      m_ovStrategies   = nullptr;
    QLabel*      m_ovExposure     = nullptr;
    QLabel*      m_ovPnL          = nullptr;

    QWidget*     m_propertiesWidget = nullptr;
    QFormLayout* m_propertiesForm   = nullptr;

    QWidget*     m_infoWidget = nullptr;
    QFormLayout* m_infoForm   = nullptr;
};

#endif // PORTFOLIOWORKSPACE_H
