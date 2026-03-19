#ifndef MODELTREEREPOSITORY_H
#define MODELTREEREPOSITORY_H

#include "ModelNodeRecord.h"
#include "DB/dbdatatypes.h"
#include <QList>
#include <QString>
#include <optional>

class QSqlDatabase;

class ModelTreeRepository
{
public:
    explicit ModelTreeRepository(const QString& dbPath, const QString& connectionName);
    ~ModelTreeRepository();

    bool initialize();

    // ---- model_nodes CRUD ----
    bool insertNode(const ModelNodeRecord& record);
    bool updateNode(const ModelNodeRecord& record);
    bool deleteNode(const QString& uuid);
    std::optional<ModelNodeRecord> fetchNode(const QString& uuid) const;
    QList<ModelNodeRecord> fetchChildren(const QString& parentUuid) const;
    QList<ModelNodeRecord> fetchTopLevel() const;
    QList<ModelNodeRecord> fetchAll() const;

    // Bulk (migration/import only)
    bool replaceAll(const QList<ModelNodeRecord>& records);

    int nextSortOrder(const QString& parentUuid) const;

    // ---- strategy_definitions CRUD (legacy, reads backup table) ----
    bool createStrategyDefinition(const DbStrategyDefinition& def);
    DbStrategyDefinition fetchStrategyDefinition(const QString& defId) const;
    bool updateStrategyDefinition(const DbStrategyDefinition& def);
    QList<DbStrategyDefinition> listStrategyDefinitions(bool includeArchived = false) const;
    bool archiveStrategyDefinition(const QString& defId);

    // ---- strategies CRUD (v3 catalog) ----
    bool createStrategyCatalog(const DbStrategy& strategy);
    DbStrategy fetchStrategyCatalog(const QString& strategyId) const;
    QList<DbStrategy> listStrategyCatalog(bool includeArchived = false) const;
    bool updateStrategyCatalog(const DbStrategy& strategy);
    bool archiveStrategyCatalog(const QString& strategyId);
    int  removeOrphanedCatalogEntries();

    // ---- strategy_versions CRUD ----
    bool createStrategyVersion(const DbStrategyVersion& version);
    DbStrategyVersion fetchStrategyVersion(const QString& versionId) const;
    QList<DbStrategyVersion> listStrategyVersions(const QString& strategyId) const;
    DbStrategyVersion fetchLatestVersion(const QString& strategyId) const;
    bool setVersionPublished(const QString& versionId, bool published);
    int nextVersionNumber(const QString& strategyId) const;

    // ---- live_strategy_bindings CRUD ----
    bool createLiveBinding(const DbLiveStrategyBinding& binding);
    DbLiveStrategyBinding fetchBindingForNode(const QString& nodeUuid) const;
    QList<DbLiveStrategyBinding> listBindingsForDefinition(const QString& defId) const;
    bool removeBindingForNode(const QString& nodeUuid);
    bool updateBindingVersion(const QString& bindingId, const QString& versionId);

    // ---- backtest_run_profiles CRUD ----
    bool createRunProfile(const DbBacktestRunProfile& profile);
    QList<DbBacktestRunProfile> listRunProfiles(const QString& ownerType,
                                                 const QString& ownerRefId) const;

    // ---- Metadata ----
    QString metadata(const QString& key) const;
    bool setMetadata(const QString& key, const QString& value);

private:
    QSqlDatabase db() const;
    ModelNodeRecord recordFromQuery(const class QSqlQuery& query) const;
    DbStrategyDefinition definitionFromQuery(const class QSqlQuery& query) const;
    DbStrategy strategyCatalogFromQuery(const class QSqlQuery& query) const;
    DbStrategyVersion versionFromQuery(const class QSqlQuery& query) const;
    DbLiveStrategyBinding bindingFromQuery(const class QSqlQuery& query) const;
    DbBacktestRunProfile profileFromQuery(const class QSqlQuery& query) const;

    bool migrateV2toV3();
    void repairBindingsTableForeignKey();

    QString m_dbPath;
    QString m_connectionName;
};

#endif // MODELTREEREPOSITORY_H
