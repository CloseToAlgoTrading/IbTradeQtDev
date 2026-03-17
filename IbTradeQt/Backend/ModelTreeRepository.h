#ifndef MODELTREEREPOSITORY_H
#define MODELTREEREPOSITORY_H

#include "ModelNodeRecord.h"
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

    // CRUD
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

    // Metadata
    QString metadata(const QString& key) const;
    bool setMetadata(const QString& key, const QString& value);

private:
    QSqlDatabase db() const;
    ModelNodeRecord recordFromQuery(const class QSqlQuery& query) const;

    QString m_dbPath;
    QString m_connectionName;
};

#endif // MODELTREEREPOSITORY_H
