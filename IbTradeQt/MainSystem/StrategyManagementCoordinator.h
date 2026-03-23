#ifndef STRATEGYMANAGEMENTCOORDINATOR_H
#define STRATEGYMANAGEMENTCOORDINATOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "cgenericmodelApi.h"

class ISystemBackend;
class CIBTradeSystemView;

namespace StrategyMgmt {
class StrategyManagementPanel;
class StrategyCatalogPanel;
class StrategyDetailPanel;
}

class StrategyManagementCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit StrategyManagementCoordinator(QObject* parent = nullptr);

    void setView(CIBTradeSystemView* view);
    void setBackend(ISystemBackend* backend);
    void setPanel(StrategyMgmt::StrategyManagementPanel* panel);

    void wireSignals();
    void refreshCatalog();

    StrategyMgmt::StrategyManagementPanel* panel() const { return m_panel; }

signals:
    void openInBacktest(const QString& strategyId, const QString& versionId);
    void refreshLiveTree();

private slots:
    void onStrategySelected(const QString& strategyId);
    void onCreateStrategy(const QString& name, const QJsonObject& initialConfig);
    void onRenameStrategy(const QString& strategyId, const QString& newName);
    void onPublishVersion(const QString& strategyId, const QString& versionId);
    void onUnpublishVersion(const QString& strategyId, const QString& versionId);
    void onSaveVersion(const QString& strategyId, const QJsonObject& config,
                       const QString& notes);
    void onDeleteVersion(const QString& strategyId, const QString& versionId);
    void onDeployVersion(const QString& strategyId, const QString& versionId,
                         const QString& targetStrategyNodeId);
    void onBacktestVersion(const QString& strategyId, const QString& versionId);

    void confirmAndDeleteStrategy(const QString& strategyId);

private:
    CIBTradeSystemView*                  m_view    = nullptr;
    ISystemBackend*                      m_backend = nullptr;
    StrategyMgmt::StrategyManagementPanel* m_panel = nullptr;
};

#endif // STRATEGYMANAGEMENTCOORDINATOR_H
