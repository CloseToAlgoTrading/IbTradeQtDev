#ifndef ISYSTEMBACKEND_H
#define ISYSTEMBACKEND_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include "ModelType.h"

class CBasicRoot;

class ISystemBackend : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~ISystemBackend() = default;

    // ---- Model Service: Topology CRUD ----
    virtual QString createAccount(const QString& name) = 0;
    virtual QString createPortfolio(const QString& accountId, const QString& name) = 0;
    virtual QString createStrategy(const QString& portfolioId, ModelType type) = 0;
    virtual bool removeNode(const QString& uuid) = 0;
    virtual bool renameNode(const QString& uuid, const QString& name) = 0;
    virtual bool setNodeActive(const QString& uuid, bool active) = 0;
    virtual bool moveNode(const QString& uuid, const QString& newParentUuid, int newSortOrder) = 0;

    // ---- Model Service: Embedded Block CRUD ----
    virtual bool addBlock(const QString& strategyId, const QString& category, const QString& blockId, const QJsonObject& defaultConfig = {}) = 0;
    virtual bool removeBlock(const QString& strategyId, const QString& category, int index) = 0;

    // ---- Model Service: Config ----
    virtual QJsonObject nodeConfig(const QString& uuid) const = 0;
    virtual bool updateNodeConfig(const QString& uuid, const QJsonObject& config) = 0;
    virtual QJsonObject pipelineConfig(const QString& strategyId) const = 0;
    virtual bool updatePipelineConfig(const QString& strategyId, const QJsonObject& config) = 0;

    // ---- Model Service: Tree Queries (persistent state only) ----
    virtual QJsonObject fullTreeSnapshot() const = 0;
    virtual QJsonArray listAccounts() const = 0;
    virtual QJsonArray listPortfolios(const QString& accountId) const = 0;
    virtual QJsonArray listStrategies(const QString& portfolioId) const = 0;
    virtual QJsonObject nodeInfo(const QString& uuid) const = 0;

    // ---- Runtime Service (transient state) ----
    virtual bool startStrategy(const QString& uuid) = 0;
    virtual bool stopStrategy(const QString& uuid) = 0;
    virtual bool connectBroker() = 0;
    virtual bool disconnectBroker() = 0;
    virtual QJsonObject runtimeState(const QString& uuid) const = 0;
    virtual bool isBrokerConnected() const = 0;

    // ---- Persistence ----
    virtual bool loadFromDb() = 0;
    virtual bool importFromJsonFile(const QString& path) = 0;
    virtual bool exportToJsonFile(const QString& path) = 0;

    // ---- Access (transition period) ----
    virtual CBasicRoot* dataRoot() const = 0;

    // ---- Strategy Catalog ----
    // Creates a canonical strategy definition (independent of live tree placement).
    // Returns the new definition UUID, or empty string on failure.
    virtual QString     createStrategyDefinition(const QString& name, int kind,
                                                 const QJsonObject& fullConfig) = 0;
    // Updates the canonical config of an existing definition.
    // Version increment and timestamp are computed by the implementation.
    virtual bool        updateStrategyDefinition(const QString& defId,
                                                 const QJsonObject& fullConfig) = 0;
    // Returns a QJsonObject representing the definition, or empty object if not found.
    virtual QJsonObject strategyDefinition(const QString& defId) const = 0;
    // Lists all strategy definitions; set includeArchived to true to include archived ones.
    virtual QJsonArray  listStrategyDefinitions(bool includeArchived = false) const = 0;
    // Archives a definition (soft delete). Does not remove bindings or run history.
    virtual bool        archiveStrategyDefinition(const QString& defId) = 0;
    // Explicitly binds an existing live strategy node to a canonical definition.
    // Used during import/migration; normally auto-created by createStrategy().
    virtual bool        bindLiveNodeToDefinition(const QString& nodeId,
                                                 const QString& defId) = 0;
    // Returns the definition JSON for the strategy node's bound definition,
    // or empty object if no binding exists.
    virtual QJsonObject strategyDefinitionForNode(const QString& nodeId) const = 0;

    // ---- Backtest Run Profiles ----
    // owner_type: "strategy_definition" | "live_strategy" | "portfolio" | "account"
    // owner_ref_id: UUID of the owning object.
    // Returns the new profile UUID, or empty string on failure.
    virtual QString    createBacktestRunProfile(const QString& ownerType,
                                                const QString& ownerRefId,
                                                const QString& name,
                                                const QJsonObject& runConfig) = 0;
    virtual QJsonArray listBacktestRunProfiles(const QString& ownerType,
                                               const QString& ownerRefId) const = 0;

signals:
    void nodeCreated(const QString& uuid, const QString& parentUuid, int modelType);
    void nodeRemoved(const QString& uuid);
    void nodeRenamed(const QString& uuid, const QString& newName);
    void nodeActiveChanged(const QString& uuid, bool active);
    void nodeMoved(const QString& uuid, const QString& newParentUuid);
    void configChanged(const QString& uuid);
    void pipelineConfigChanged(const QString& uuid, const QJsonObject& config);
    void treeLoaded();

    void strategyStateChanged(const QString& uuid, int displayState);
    void pnlUpdated(const QString& uuid, double pnl);
    void brokerConnectionChanged(bool connected);
    void logMessage(const QString& source, const QString& level, const QString& message);

    // Emitted when a canonical strategy definition's config or metadata changes.
    void strategyDefinitionChanged(const QString& defId);
};

#endif // ISYSTEMBACKEND_H
