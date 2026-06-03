#ifndef SYSTEMBACKENDIMPL_H
#define SYSTEMBACKENDIMPL_H

#include "ISystemBackend.h"
#include "IModelTreeRepository.h"
#include "ModelTreeMapper.h"
#include <QHash>

class CBasicRoot;
class CGenericModelApi;
class CBaseModel;

class SystemBackendImpl : public ISystemBackend
{
    Q_OBJECT

public:
    /** Caller owns the repository lifetime; backend does not delete. */
    explicit SystemBackendImpl(IModelTreeRepository* repo, QObject* parent = nullptr);
    ~SystemBackendImpl() override;

    IModelTreeRepository* modelTreeRepository() const { return m_repo; }

    // ---- Model Service: Topology CRUD ----
    QString createAccount(const QString& name) override;
    QString createPortfolio(const QString& accountId, const QString& name) override;
    QString createStrategy(const QString& portfolioId, ModelType type) override;
    bool removeNode(const QString& uuid) override;
    bool renameNode(const QString& uuid, const QString& name) override;
    bool setNodeActive(const QString& uuid, bool active) override;
    bool moveNode(const QString& uuid, const QString& newParentUuid, int newSortOrder) override;

    // ---- Model Service: Embedded Block CRUD ----
    bool addBlock(const QString& strategyId, const QString& category, const QString& blockId, const QJsonObject& defaultConfig = {}) override;
    bool removeBlock(const QString& strategyId, const QString& category, int index) override;

    // ---- Model Service: Config ----
    QJsonObject nodeConfig(const QString& uuid) const override;
    bool updateNodeConfig(const QString& uuid, const QJsonObject& config) override;
    QJsonObject pipelineConfig(const QString& strategyId) const override;
    bool updatePipelineConfig(const QString& strategyId, const QJsonObject& config) override;

    // ---- Model Service: Tree Queries ----
    QJsonObject fullTreeSnapshot() const override;
    QJsonArray listAccounts() const override;
    QJsonArray listPortfolios(const QString& accountId) const override;
    QJsonArray listStrategies(const QString& portfolioId) const override;
    QJsonObject nodeInfo(const QString& uuid) const override;

    // ---- Runtime Service ----
    bool startStrategy(const QString& uuid) override;
    bool stopStrategy(const QString& uuid) override;
    bool connectBroker() override;
    bool disconnectBroker() override;
    QJsonObject runtimeState(const QString& uuid) const override;
    bool isBrokerConnected() const override;

    // ---- Persistence ----
    bool loadFromDb() override;
    bool importFromJsonFile(const QString& path) override;
    bool exportToJsonFile(const QString& path) override;

    // ---- Access ----
    CBasicRoot* dataRoot() const override { return m_root; }

    // ---- Strategy Catalog (v2: families + versions) ----
    QString     createStrategyCatalogEntry(const QString& name, int kind,
                                            const QJsonObject& initialConfig = {},
                                            const QString& description = {}) override;
    bool        updateStrategyCatalogMeta(const QString& strategyId,
                                           const QString& name,
                                           const QString& description,
                                           const QString& tags,
                                           const QString& lifecycleState) override;
    bool        setStrategyLifecycle(const QString& strategyId,
                                     StrategyLifecycle::ManualLifecycle lifecycle) override;
    QJsonObject strategyLifecycleSummary(const QString& strategyId) const override;
    QJsonObject strategyCatalogEntry(const QString& strategyId) const override;
    QJsonArray  listStrategyCatalog(bool includeArchived = false) const override;
    bool        archiveStrategyCatalogEntry(const QString& strategyId) override;
    bool        deleteStrategyCatalogCascade(const QString& strategyId) override;
    bool        isCatalogStrategyActiveInLive(const QString& strategyId) const override;

    QString     createStrategyVersion(const QString& strategyId,
                                       const QJsonObject& config,
                                       const QString& notes = {},
                                       const QString& fromVersionId = {}) override;
    QJsonObject strategyVersionInfo(const QString& versionId) const override;
    QJsonArray  listStrategyVersions(const QString& strategyId) const override;
    bool        deleteStrategyVersion(const QString& versionId) override;
    bool        publishVersion(const QString& versionId) override;
    bool        unpublishVersion(const QString& versionId) override;
    bool        setVersionLifecycle(const QString& versionId,
                                    StrategyLifecycle::ManualLifecycle lifecycle) override;

    bool        bindLiveNodeToVersion(const QString& nodeId,
                                       const QString& strategyId,
                                       const QString& versionId) override;
    QJsonObject bindingForNode(const QString& nodeId) const override;
    QString     createLiveNodeForExistingCatalog(const QString& portfolioId,
                                                  ModelType type,
                                                  const QString& strategyId,
                                                  const QString& versionId) override;
    bool        isNodeDivergedFromVersion(const QString& nodeId) const override;

    // ---- Legacy Strategy Catalog (deprecated, delegate to v2) ----
    QString     createStrategyDefinition(const QString& name, int kind,
                                         const QJsonObject& fullConfig) override;
    bool        updateStrategyDefinition(const QString& defId,
                                         const QJsonObject& fullConfig) override;
    QJsonObject strategyDefinition(const QString& defId) const override;
    QJsonArray  listStrategyDefinitions(bool includeArchived = false) const override;
    bool        archiveStrategyDefinition(const QString& defId) override;
    bool        bindLiveNodeToDefinition(const QString& nodeId,
                                         const QString& defId) override;
    QJsonObject strategyDefinitionForNode(const QString& nodeId) const override;

    // ---- Backtest Run Profiles ----
    QString    createBacktestRunProfile(const QString& ownerType,
                                        const QString& ownerRefId,
                                        const QString& name,
                                        const QJsonObject& runConfig) override;
    QJsonArray listBacktestRunProfiles(const QString& ownerType,
                                       const QString& ownerRefId) const override;

private:
    CGenericModelApi* findNodeByUuid(const QString& uuid) const;
    void rebuildUuidIndex();
    void wireAllRuntimeSignals();
    void wireRuntimeSignals(CGenericModelApi* node);
    void persistNode(CGenericModelApi* node);

    // Detects whether a node's current config diverges from its pinned version.
    // Emits nodeConfigDiverged() if so. Does NOT create versions.
    void detectVersionDivergence(const QString& strategyNodeUuid);
    // Extracts the canonical config subset (pipelineConfig + assetList).
    static QJsonObject extractCanonicalStrategyConfig(const QJsonObject& rawNodeConfigJson);
    // Creates catalog entry + v1 + binding for a new or orphaned strategy node.
    void createCatalogEntryAndBinding(const QString& nodeUuid, const QString& name,
                                      int strategyKind, const QJsonObject& canonicalConfig);
    static QString nowUtcIso();

    static bool isStrategyType(ModelType type);
    static bool isDescendantOf(CGenericModelApi* node, CGenericModelApi* potentialAncestor);
    static QString categoryToJsonKey(const QString& category, bool& isArray);

    static QJsonObject definitionToJson(const DbStrategyDefinition& def);
    static QJsonObject strategyToJson(const DbStrategy& s);
    static QJsonObject versionToJson(const DbStrategyVersion& v);
    int publishedVersionCount(const QString& strategyId) const;
    StrategyLifecycle::Summary lifecycleSummaryForStrategy(const DbStrategy& s) const;
    QJsonObject strategyToJsonWithLifecycle(const DbStrategy& s) const;

    CBasicRoot* m_root = nullptr;
    IModelTreeRepository* m_repo = nullptr;
    QHash<QString, CGenericModelApi*> m_uuidIndex;
    bool m_brokerConnected = false;
};

#endif // SYSTEMBACKENDIMPL_H
