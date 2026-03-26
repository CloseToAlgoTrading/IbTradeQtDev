#ifndef MODELTREEREPOSITORY_H
#define MODELTREEREPOSITORY_H

#include "IModelTreeRepository.h"
#include "ModelNodeRecord.h"
#include "DB/dbdatatypes.h"
#include <QList>
#include <QString>
#include <QStringList>
#include <optional>

class QSqlDatabase;

/** SQLite implementation of ModelStore persistence. */
class ModelTreeRepository : public IModelTreeRepository
{
public:
    explicit ModelTreeRepository(const QString& dbPath, const QString& connectionName);
    ~ModelTreeRepository() override;

    bool initialize() override;

    bool insertNode(const ModelNodeRecord& record) override;
    bool updateNode(const ModelNodeRecord& record) override;
    bool deleteNode(const QString& uuid) override;
    std::optional<ModelNodeRecord> fetchNode(const QString& uuid) const override;
    QList<ModelNodeRecord> fetchChildren(const QString& parentUuid) const override;
    QList<ModelNodeRecord> fetchTopLevel() const override;
    QList<ModelNodeRecord> fetchAll() const override;

    bool replaceAll(const QList<ModelNodeRecord>& records) override;

    int nextSortOrder(const QString& parentUuid) const override;

    bool createStrategyDefinition(const DbStrategyDefinition& def) override;
    DbStrategyDefinition fetchStrategyDefinition(const QString& defId) const override;
    bool updateStrategyDefinition(const DbStrategyDefinition& def) override;
    QList<DbStrategyDefinition> listStrategyDefinitions(bool includeArchived = false) const override;
    bool archiveStrategyDefinition(const QString& defId) override;

    bool createStrategyCatalog(const DbStrategy& strategy) override;
    DbStrategy fetchStrategyCatalog(const QString& strategyId) const override;
    QList<DbStrategy> listStrategyCatalog(bool includeArchived = false) const override;
    bool updateStrategyCatalog(const DbStrategy& strategy) override;
    bool archiveStrategyCatalog(const QString& strategyId) override;
    bool deleteStrategyCatalogCascade(const QString& strategyId,
                                      const QStringList& purgeProfileOwnerRefs) override;
    int  removeOrphanedCatalogEntries() override;

    bool createStrategyVersion(const DbStrategyVersion& version) override;
    DbStrategyVersion fetchStrategyVersion(const QString& versionId) const override;
    QList<DbStrategyVersion> listStrategyVersions(const QString& strategyId) const override;
    DbStrategyVersion fetchLatestVersion(const QString& strategyId) const override;
    bool deleteStrategyVersion(const QString& versionId) override;
    bool setVersionPublished(const QString& versionId, bool published) override;
    int nextVersionNumber(const QString& strategyId) const override;

    bool createLiveBinding(const DbLiveStrategyBinding& binding) override;
    DbLiveStrategyBinding fetchBindingForNode(const QString& nodeUuid) const override;
    QList<DbLiveStrategyBinding> listBindingsForDefinition(const QString& defId) const override;
    bool removeBindingForNode(const QString& nodeUuid) override;
    bool updateBindingVersion(const QString& bindingId, const QString& versionId) override;

    bool createRunProfile(const DbBacktestRunProfile& profile) override;
    QList<DbBacktestRunProfile> listRunProfiles(const QString& ownerType,
                                                 const QString& ownerRefId) const override;

    QString metadata(const QString& key) const override;
    bool setMetadata(const QString& key, const QString& value) override;

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
