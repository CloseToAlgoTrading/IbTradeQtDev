#ifndef IMODELTREEREPOSITORY_H
#define IMODELTREEREPOSITORY_H

#include "ModelNodeRecord.h"
#include "DB/dbdatatypes.h"
#include <QList>
#include <QString>
#include <optional>
#include <QStringList>

/** Persistence boundary for model tree + strategy catalog (ModelStore). SQLite and PostgreSQL implementations. */
class IModelTreeRepository
{
public:
    virtual ~IModelTreeRepository() = default;

    virtual bool initialize() = 0;

    virtual bool insertNode(const ModelNodeRecord& record) = 0;
    virtual bool updateNode(const ModelNodeRecord& record) = 0;
    virtual bool deleteNode(const QString& uuid) = 0;
    virtual std::optional<ModelNodeRecord> fetchNode(const QString& uuid) const = 0;
    virtual QList<ModelNodeRecord> fetchChildren(const QString& parentUuid) const = 0;
    virtual QList<ModelNodeRecord> fetchTopLevel() const = 0;
    virtual QList<ModelNodeRecord> fetchAll() const = 0;

    virtual bool replaceAll(const QList<ModelNodeRecord>& records) = 0;

    virtual int nextSortOrder(const QString& parentUuid) const = 0;

    virtual bool createStrategyDefinition(const DbStrategyDefinition& def) = 0;
    virtual DbStrategyDefinition fetchStrategyDefinition(const QString& defId) const = 0;
    virtual bool updateStrategyDefinition(const DbStrategyDefinition& def) = 0;
    virtual QList<DbStrategyDefinition> listStrategyDefinitions(bool includeArchived = false) const = 0;
    virtual bool archiveStrategyDefinition(const QString& defId) = 0;

    virtual bool createStrategyCatalog(const DbStrategy& strategy) = 0;
    virtual DbStrategy fetchStrategyCatalog(const QString& strategyId) const = 0;
    virtual QList<DbStrategy> listStrategyCatalog(bool includeArchived = false) const = 0;
    virtual bool updateStrategyCatalog(const DbStrategy& strategy) = 0;
    virtual bool archiveStrategyCatalog(const QString& strategyId) = 0;
    /** Removes catalog row, all versions, leftover bindings, and run profiles for the given owner refs. */
    virtual bool deleteStrategyCatalogCascade(const QString& strategyId,
                                              const QStringList& purgeProfileOwnerRefs) = 0;
    virtual int removeOrphanedCatalogEntries() = 0;

    virtual bool createStrategyVersion(const DbStrategyVersion& version) = 0;
    virtual DbStrategyVersion fetchStrategyVersion(const QString& versionId) const = 0;
    virtual QList<DbStrategyVersion> listStrategyVersions(const QString& strategyId) const = 0;
    virtual DbStrategyVersion fetchLatestVersion(const QString& strategyId) const = 0;
    virtual bool deleteStrategyVersion(const QString& versionId) = 0;
    virtual bool setVersionPublished(const QString& versionId, bool published) = 0;
    virtual int nextVersionNumber(const QString& strategyId) const = 0;

    virtual bool createLiveBinding(const DbLiveStrategyBinding& binding) = 0;
    virtual DbLiveStrategyBinding fetchBindingForNode(const QString& nodeUuid) const = 0;
    virtual QList<DbLiveStrategyBinding> listBindingsForDefinition(const QString& defId) const = 0;
    virtual bool removeBindingForNode(const QString& nodeUuid) = 0;
    virtual bool updateBindingVersion(const QString& bindingId, const QString& versionId) = 0;

    virtual bool createRunProfile(const DbBacktestRunProfile& profile) = 0;
    virtual QList<DbBacktestRunProfile> listRunProfiles(const QString& ownerType,
                                                          const QString& ownerRefId) const = 0;

    virtual QString metadata(const QString& key) const = 0;
    virtual bool setMetadata(const QString& key, const QString& value) = 0;
};

#endif
