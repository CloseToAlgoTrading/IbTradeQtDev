#ifndef ACCOUNTWORKSPACE_H
#define ACCOUNTWORKSPACE_H

#include "WorkspaceBase.h"

class QFormLayout;
class QLabel;

class AccountWorkspace : public WorkspaceBase
{
    Q_OBJECT
public:
    explicit AccountWorkspace(QWidget* parent = nullptr);

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
    QLabel*      m_ovBrokerStatus = nullptr;
    QLabel*      m_ovPortfolios   = nullptr;
    QLabel*      m_ovNetLiq       = nullptr;

    QWidget*     m_propertiesWidget = nullptr;
    QFormLayout* m_propertiesForm   = nullptr;

    QWidget*     m_infoWidget = nullptr;
    QFormLayout* m_infoForm   = nullptr;
};

#endif // ACCOUNTWORKSPACE_H
