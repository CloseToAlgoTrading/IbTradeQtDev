#ifndef STRATEGYMANAGEMENTCOORDINATOR_H
#define STRATEGYMANAGEMENTCOORDINATOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include "cgenericmodelApi.h"

class ISystemBackend;
class CIBTradeSystemView;
class StrategyManagementUnsavedDraftFlow;

namespace StrategyMgmt {
class StrategyManagementPanel;
class StrategyDetailPanel;
}

class StrategyManagementCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit StrategyManagementCoordinator(QObject* parent = nullptr);
    ~StrategyManagementCoordinator() override;

    void setView(CIBTradeSystemView* view);
    void setBackend(ISystemBackend* backend);
    void setPanel(StrategyMgmt::StrategyManagementPanel* panel);

    void wireSignals();
    void refreshCatalog();

    StrategyMgmt::StrategyManagementPanel* panel() const { return m_panel; }

    bool tryResolveUnsavedStrategyDraft(const QString& message);

signals:
    void openInBacktest(const QString& strategyId, const QString& versionId);
    void refreshLiveTree();

private slots:
    void onStrategySelected(const QString& strategyId);
    void onCreateStrategy(const QString& name, const QJsonObject& initialConfig);
    void onMetadataChanged(const QString& strategyId,
                           const QString& name,
                           const QString& description,
                           const QString& tags,
                           const QString& lifecycleState);
    void onPublishVersion(const QString& strategyId, const QString& versionId);
    void onUnpublishVersion(const QString& strategyId, const QString& versionId);
    void onSaveVersion(const QString& strategyId, const QJsonObject& config,
                       const QString& notes);
    void onDeleteVersion(const QString& strategyId, const QString& versionId);
    void onDeployVersion(const QString& strategyId, const QString& versionId,
                         const QString& targetStrategyNodeId);
    void onBacktestVersion(const QString& strategyId, const QString& versionId);

    void retireStrategy(const QString& strategyId);
    void confirmAndDeleteStrategy(const QString& strategyId);
    void onStrategyVersionRowChangeRequested(int newRow, int previousRow);

private:
    CIBTradeSystemView*                  m_view    = nullptr;
    ISystemBackend*                      m_backend = nullptr;
    StrategyMgmt::StrategyManagementPanel* m_panel = nullptr;
    std::unique_ptr<StrategyManagementUnsavedDraftFlow> m_unsavedDraftFlow;
};

#endif // STRATEGYMANAGEMENTCOORDINATOR_H
