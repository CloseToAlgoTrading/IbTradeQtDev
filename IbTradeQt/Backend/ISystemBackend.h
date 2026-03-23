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

    // ---- Strategy Catalog (v2: families + versions) ----

    // Creates a strategy family/container in the catalog.
    // Returns the new strategy UUID, or empty string on failure.
    virtual QString     createStrategyCatalogEntry(const QString& name, int kind,
                                                    const QJsonObject& initialConfig = {},
                                                    const QString& description = {}) = 0;
    // Updates metadata (non-config) fields of a catalog strategy.
    virtual bool        updateStrategyCatalogMeta(const QString& strategyId,
                                                   const QString& name,
                                                   const QString& description,
                                                   const QString& tags,
                                                   const QString& lifecycleState) = 0;
    // Returns a QJsonObject representing the catalog entry, or empty if not found.
    virtual QJsonObject strategyCatalogEntry(const QString& strategyId) const = 0;
    // Lists all catalog entries; set includeArchived to true to include archived ones.
    virtual QJsonArray  listStrategyCatalog(bool includeArchived = false) const = 0;
    // Archives a catalog entry (soft delete). Does not remove bindings or run history.
    virtual bool        archiveStrategyCatalogEntry(const QString& strategyId) = 0;
    /** Removes catalog strategy, all versions, live tree nodes bound to it, run profiles, and backtest runs. */
    virtual bool        deleteStrategyCatalogCascade(const QString& strategyId) = 0;
    /** True if any live portfolio strategy node bound to this catalog id has "On" enabled. */
    virtual bool        isCatalogStrategyActiveInLive(const QString& strategyId) const = 0;

    // Creates an immutable config snapshot (version) for a strategy family.
    // Returns the new version UUID, or empty string on failure.
    virtual QString     createStrategyVersion(const QString& strategyId,
                                               const QJsonObject& config,
                                               const QString& notes = {},
                                               const QString& fromVersionId = {}) = 0;
    // Returns version info JSON, or empty if not found.
    virtual QJsonObject strategyVersionInfo(const QString& versionId) const = 0;
    // Lists all versions for a strategy, ordered by version_number ASC.
    virtual QJsonArray  listStrategyVersions(const QString& strategyId) const = 0;
    // Marks a version as published (eligible for deployment/selection).
    virtual bool        publishVersion(const QString& versionId) = 0;

    // Binds a live tree node to a specific strategy version.
    virtual bool        bindLiveNodeToVersion(const QString& nodeId,
                                               const QString& strategyId,
                                               const QString& versionId) = 0;
    // Returns binding info (strategy_id + version_id + version config) for a node.
    virtual QJsonObject bindingForNode(const QString& nodeId) const = 0;

    // Creates a live tree node bound to an existing catalog strategy+version,
    // without creating a new catalog entry. Returns the new node UUID.
    virtual QString     createLiveNodeForExistingCatalog(const QString& portfolioId,
                                                         ModelType type,
                                                         const QString& strategyId,
                                                         const QString& versionId) = 0;

    // Returns true if the node's current config differs from its pinned version.
    // Does NOT create a new version — purely a detector.
    virtual bool        isNodeDivergedFromVersion(const QString& nodeId) const = 0;

    // ---- Legacy Strategy Catalog (deprecated, delegates to v2 catalog) ----

    virtual QString     createStrategyDefinition(const QString& name, int kind,
                                                 const QJsonObject& fullConfig) = 0;
    virtual bool        updateStrategyDefinition(const QString& defId,
                                                 const QJsonObject& fullConfig) = 0;
    virtual QJsonObject strategyDefinition(const QString& defId) const = 0;
    virtual QJsonArray  listStrategyDefinitions(bool includeArchived = false) const = 0;
    virtual bool        archiveStrategyDefinition(const QString& defId) = 0;
    virtual bool        bindLiveNodeToDefinition(const QString& nodeId,
                                                 const QString& defId) = 0;
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

    // v3 catalog signals
    void strategyCatalogChanged(const QString& strategyId);
    void strategyVersionCreated(const QString& strategyId, const QString& versionId);
    // Emitted when detectVersionDivergence() finds the node config differs from its pinned version.
    void nodeConfigDiverged(const QString& nodeId);
};

#endif // ISYSTEMBACKEND_H
