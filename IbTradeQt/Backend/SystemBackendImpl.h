#ifndef SYSTEMBACKENDIMPL_H
#define SYSTEMBACKENDIMPL_H

#include "ISystemBackend.h"
#include "ModelTreeRepository.h"
#include "ModelTreeMapper.h"
#include <QHash>

class CBasicRoot;
class CGenericModelApi;
class CBaseModel;

class SystemBackendImpl : public ISystemBackend
{
    Q_OBJECT

public:
    explicit SystemBackendImpl(ModelTreeRepository* repo, QObject* parent = nullptr);
    ~SystemBackendImpl() override;

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

    // ---- Strategy Catalog ----
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

    // Canonical strategy definition sync — the single point for all config mutations.
    void syncDefinitionFromNode(const QString& strategyNodeUuid);
    // Extracts the canonical config subset (parameters + pipelineConfig + assetList).
    static QJsonObject extractCanonicalStrategyConfig(const QJsonObject& rawNodeConfigJson);
    // Creates definition + binding for a new or orphaned strategy node.
    void createDefinitionAndBinding(const QString& nodeUuid, const QString& name,
                                    int strategyKind, const QJsonObject& canonicalConfig);
    static QString nowUtcIso();

    static bool isStrategyType(ModelType type);
    static bool isDescendantOf(CGenericModelApi* node, CGenericModelApi* potentialAncestor);
    static QString categoryToJsonKey(const QString& category, bool& isArray);

    static QJsonObject definitionToJson(const DbStrategyDefinition& def);

    CBasicRoot* m_root = nullptr;
    ModelTreeRepository* m_repo;
    QHash<QString, CGenericModelApi*> m_uuidIndex;
    bool m_brokerConnected = false;
};

#endif // SYSTEMBACKENDIMPL_H
