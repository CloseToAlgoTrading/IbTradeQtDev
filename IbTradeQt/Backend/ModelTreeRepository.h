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

    // ---- strategy_definitions CRUD (record-only, no business logic) ----
    bool createStrategyDefinition(const DbStrategyDefinition& def);
    DbStrategyDefinition fetchStrategyDefinition(const QString& defId) const;
    bool updateStrategyDefinition(const DbStrategyDefinition& def);
    QList<DbStrategyDefinition> listStrategyDefinitions(bool includeArchived = false) const;
    bool archiveStrategyDefinition(const QString& defId);

    // ---- live_strategy_bindings CRUD ----
    bool createLiveBinding(const DbLiveStrategyBinding& binding);
    DbLiveStrategyBinding fetchBindingForNode(const QString& nodeUuid) const;
    QList<DbLiveStrategyBinding> listBindingsForDefinition(const QString& defId) const;
    bool removeBindingForNode(const QString& nodeUuid);

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
    DbLiveStrategyBinding bindingFromQuery(const class QSqlQuery& query) const;
    DbBacktestRunProfile profileFromQuery(const class QSqlQuery& query) const;

    QString m_dbPath;
    QString m_connectionName;
};

#endif // MODELTREEREPOSITORY_H
