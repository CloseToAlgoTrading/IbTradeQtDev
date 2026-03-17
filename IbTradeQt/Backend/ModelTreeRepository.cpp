#include "ModelTreeRepository.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QVariant>

ModelTreeRepository::ModelTreeRepository(const QString& dbPath, const QString& connectionName)
    : m_dbPath(dbPath)
    , m_connectionName(connectionName)
{
}

ModelTreeRepository::~ModelTreeRepository()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

QSqlDatabase ModelTreeRepository::db() const
{
    return QSqlDatabase::database(m_connectionName);
}

bool ModelTreeRepository::initialize()
{
    QSqlDatabase database;
    if (QSqlDatabase::contains(m_connectionName)) {
        database = QSqlDatabase::database(m_connectionName);
    } else {
        database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        database.setDatabaseName(m_dbPath);
    }

    if (!database.isOpen() && !database.open()) {
        qWarning("ModelTreeRepository: cannot open DB: %s",
                 qPrintable(database.lastError().text()));
        return false;
    }

    QSqlQuery q(database);

    q.exec("PRAGMA foreign_keys = ON");

    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS model_nodes ("
        "  uuid TEXT PRIMARY KEY,"
        "  parent_uuid TEXT REFERENCES model_nodes(uuid) ON DELETE CASCADE,"
        "  model_type INTEGER NOT NULL,"
        "  name TEXT NOT NULL DEFAULT '',"
        "  config_json TEXT NOT NULL DEFAULT '{}',"
        "  sort_order INTEGER NOT NULL DEFAULT 0,"
        "  is_active INTEGER NOT NULL DEFAULT 1,"
        "  created_at TEXT NOT NULL,"
        "  updated_at TEXT NOT NULL"
        ")");
    if (!ok) {
        qWarning("ModelTreeRepository: create model_nodes failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_model_nodes_parent ON model_nodes(parent_uuid)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_model_nodes_type ON model_nodes(model_type)");

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS app_metadata ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT"
        ")");
    if (!ok) {
        qWarning("ModelTreeRepository: create app_metadata failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }

    if (metadata("schema_version").isEmpty()) {
        setMetadata("schema_version", "1");
    }

    return true;
}

ModelNodeRecord ModelTreeRepository::recordFromQuery(const QSqlQuery& query) const
{
    ModelNodeRecord rec;
    rec.uuid       = query.value("uuid").toString();
    rec.parentUuid = query.value("parent_uuid").toString();
    rec.modelType  = query.value("model_type").toInt();
    rec.name       = query.value("name").toString();
    rec.sortOrder  = query.value("sort_order").toInt();
    rec.isActive   = query.value("is_active").toBool();
    rec.createdAt  = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    rec.updatedAt  = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);

    QString configStr = query.value("config_json").toString();
    rec.config = QJsonDocument::fromJson(configStr.toUtf8()).object();

    return rec;
}

bool ModelTreeRepository::insertNode(const ModelNodeRecord& record)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO model_nodes "
        "(uuid, parent_uuid, model_type, name, config_json, sort_order, is_active, created_at, updated_at) "
        "VALUES (:uuid, :parent_uuid, :model_type, :name, :config_json, :sort_order, :is_active, :created_at, :updated_at)");

    q.bindValue(":uuid", record.uuid);
    q.bindValue(":parent_uuid", record.parentUuid.isEmpty() ? QVariant() : record.parentUuid);
    q.bindValue(":model_type", record.modelType);
    q.bindValue(":name", record.name);
    q.bindValue(":config_json", QString::fromUtf8(QJsonDocument(record.config).toJson(QJsonDocument::Compact)));
    q.bindValue(":sort_order", record.sortOrder);
    q.bindValue(":is_active", record.isActive ? 1 : 0);
    q.bindValue(":created_at", record.createdAt.toString(Qt::ISODate));
    q.bindValue(":updated_at", record.updatedAt.toString(Qt::ISODate));

    if (!q.exec()) {
        qWarning("ModelTreeRepository::insertNode failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return true;
}

bool ModelTreeRepository::updateNode(const ModelNodeRecord& record)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE model_nodes SET "
        "parent_uuid = :parent_uuid, model_type = :model_type, name = :name, "
        "config_json = :config_json, sort_order = :sort_order, is_active = :is_active, "
        "updated_at = :updated_at "
        "WHERE uuid = :uuid");

    q.bindValue(":uuid", record.uuid);
    q.bindValue(":parent_uuid", record.parentUuid.isEmpty() ? QVariant() : record.parentUuid);
    q.bindValue(":model_type", record.modelType);
    q.bindValue(":name", record.name);
    q.bindValue(":config_json", QString::fromUtf8(QJsonDocument(record.config).toJson(QJsonDocument::Compact)));
    q.bindValue(":sort_order", record.sortOrder);
    q.bindValue(":is_active", record.isActive ? 1 : 0);
    q.bindValue(":updated_at", record.updatedAt.toString(Qt::ISODate));

    if (!q.exec()) {
        qWarning("ModelTreeRepository::updateNode failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ModelTreeRepository::deleteNode(const QString& uuid)
{
    QSqlDatabase database = db();

    QSqlQuery pragma(database);
    pragma.exec("PRAGMA foreign_keys = ON");

    QSqlQuery q(database);
    q.prepare("DELETE FROM model_nodes WHERE uuid = :uuid");
    q.bindValue(":uuid", uuid);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::deleteNode failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return q.numRowsAffected() > 0;
}

std::optional<ModelNodeRecord> ModelTreeRepository::fetchNode(const QString& uuid) const
{
    QSqlQuery q(db());
    q.prepare("SELECT * FROM model_nodes WHERE uuid = :uuid");
    q.bindValue(":uuid", uuid);

    if (!q.exec() || !q.next())
        return std::nullopt;

    return recordFromQuery(q);
}

QList<ModelNodeRecord> ModelTreeRepository::fetchChildren(const QString& parentUuid) const
{
    QList<ModelNodeRecord> results;
    QSqlQuery q(db());

    if (parentUuid.isEmpty()) {
        q.prepare("SELECT * FROM model_nodes WHERE parent_uuid IS NULL ORDER BY sort_order ASC");
    } else {
        q.prepare("SELECT * FROM model_nodes WHERE parent_uuid = :parent_uuid ORDER BY sort_order ASC");
        q.bindValue(":parent_uuid", parentUuid);
    }

    if (q.exec()) {
        while (q.next())
            results.append(recordFromQuery(q));
    }
    return results;
}

QList<ModelNodeRecord> ModelTreeRepository::fetchTopLevel() const
{
    return fetchChildren(QString());
}

QList<ModelNodeRecord> ModelTreeRepository::fetchAll() const
{
    QList<ModelNodeRecord> results;
    QSqlQuery q(db());
    q.prepare("SELECT * FROM model_nodes ORDER BY sort_order ASC");

    if (q.exec()) {
        while (q.next())
            results.append(recordFromQuery(q));
    }
    return results;
}

bool ModelTreeRepository::replaceAll(const QList<ModelNodeRecord>& records)
{
    QSqlDatabase database = db();
    if (!database.transaction()) {
        qWarning("ModelTreeRepository::replaceAll: failed to start transaction");
        return false;
    }

    QSqlQuery q(database);
    q.exec("PRAGMA foreign_keys = OFF");
    q.exec("DELETE FROM model_nodes");

    for (const auto& rec : records) {
        if (!insertNode(rec)) {
            database.rollback();
            q.exec("PRAGMA foreign_keys = ON");
            return false;
        }
    }

    q.exec("PRAGMA foreign_keys = ON");

    if (!database.commit()) {
        database.rollback();
        return false;
    }
    return true;
}

int ModelTreeRepository::nextSortOrder(const QString& parentUuid) const
{
    QSqlQuery q(db());
    if (parentUuid.isEmpty()) {
        q.prepare("SELECT COALESCE(MAX(sort_order), -1) + 1 FROM model_nodes WHERE parent_uuid IS NULL");
    } else {
        q.prepare("SELECT COALESCE(MAX(sort_order), -1) + 1 FROM model_nodes WHERE parent_uuid = :parent_uuid");
        q.bindValue(":parent_uuid", parentUuid);
    }

    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

QString ModelTreeRepository::metadata(const QString& key) const
{
    QSqlQuery q(db());
    q.prepare("SELECT value FROM app_metadata WHERE key = :key");
    q.bindValue(":key", key);

    if (q.exec() && q.next())
        return q.value(0).toString();
    return {};
}

bool ModelTreeRepository::setMetadata(const QString& key, const QString& value)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT OR REPLACE INTO app_metadata (key, value) VALUES (:key, :value)");
    q.bindValue(":key", key);
    q.bindValue(":value", value);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::setMetadata failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return true;
}
