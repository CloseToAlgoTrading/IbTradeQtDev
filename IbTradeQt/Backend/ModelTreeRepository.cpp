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

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS strategy_definitions ("
        "  strategy_def_id     TEXT PRIMARY KEY,"
        "  name                TEXT NOT NULL DEFAULT '',"
        "  strategy_kind       INTEGER NOT NULL DEFAULT 0,"
        "  config_json         TEXT NOT NULL DEFAULT '{}',"
        "  version             INTEGER NOT NULL DEFAULT 1,"
        "  lifecycle_state     TEXT NOT NULL DEFAULT 'draft',"
        "  is_archived         INTEGER NOT NULL DEFAULT 0,"
        "  created_at          TEXT NOT NULL,"
        "  updated_at          TEXT NOT NULL,"
        "  created_from_def_id TEXT"
        ")");
    if (!ok) {
        qWarning("ModelTreeRepository: create strategy_definitions failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS live_strategy_bindings ("
        "  binding_id      TEXT PRIMARY KEY,"
        "  model_node_id   TEXT NOT NULL REFERENCES model_nodes(uuid) ON DELETE CASCADE,"
        "  strategy_def_id TEXT NOT NULL REFERENCES strategy_definitions(strategy_def_id),"
        "  created_at      TEXT NOT NULL,"
        "  updated_at      TEXT NOT NULL,"
        "  UNIQUE(model_node_id)"
        ")");
    if (!ok) {
        qWarning("ModelTreeRepository: create live_strategy_bindings failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_bindings_def ON live_strategy_bindings(strategy_def_id)");

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS backtest_run_profiles ("
        "  profile_id      TEXT PRIMARY KEY,"
        "  owner_type      TEXT NOT NULL,"
        "  owner_ref_id    TEXT NOT NULL,"
        "  name            TEXT NOT NULL DEFAULT '',"
        "  run_config_json TEXT NOT NULL DEFAULT '{}',"
        "  created_at      TEXT NOT NULL,"
        "  updated_at      TEXT NOT NULL"
        ")");
    if (!ok) {
        qWarning("ModelTreeRepository: create backtest_run_profiles failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_profiles_owner ON backtest_run_profiles(owner_type, owner_ref_id)");

    QString ver = metadata("schema_version");
    if (ver.isEmpty()) {
        setMetadata("schema_version", "2");
    } else if (ver == "1") {
        setMetadata("schema_version", "2");
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

// ---------------------------------------------------------------------------
// strategy_definitions CRUD
// ---------------------------------------------------------------------------

DbStrategyDefinition ModelTreeRepository::definitionFromQuery(const QSqlQuery& q) const
{
    DbStrategyDefinition def;
    def.strategyDefId    = q.value("strategy_def_id").toString();
    def.name             = q.value("name").toString();
    def.strategyKind     = q.value("strategy_kind").toInt();
    def.configJson       = q.value("config_json").toString();
    def.version          = q.value("version").toInt();
    def.lifecycleState   = q.value("lifecycle_state").toString();
    def.isArchived       = q.value("is_archived").toInt() != 0;
    def.createdAt        = q.value("created_at").toString();
    def.updatedAt        = q.value("updated_at").toString();
    def.createdFromDefId = q.value("created_from_def_id").toString();
    return def;
}

bool ModelTreeRepository::createStrategyDefinition(const DbStrategyDefinition& def)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO strategy_definitions "
        "(strategy_def_id, name, strategy_kind, config_json, version, lifecycle_state, "
        " is_archived, created_at, updated_at, created_from_def_id) "
        "VALUES (:id, :name, :kind, :cfg, :ver, :state, :archived, :created, :updated, :from_id)");
    q.bindValue(":id",       def.strategyDefId);
    q.bindValue(":name",     def.name);
    q.bindValue(":kind",     def.strategyKind);
    q.bindValue(":cfg",      def.configJson);
    q.bindValue(":ver",      def.version);
    q.bindValue(":state",    def.lifecycleState);
    q.bindValue(":archived", def.isArchived ? 1 : 0);
    q.bindValue(":created",  def.createdAt);
    q.bindValue(":updated",  def.updatedAt);
    q.bindValue(":from_id",  def.createdFromDefId.isEmpty() ? QVariant() : def.createdFromDefId);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::createStrategyDefinition failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return true;
}

DbStrategyDefinition ModelTreeRepository::fetchStrategyDefinition(const QString& defId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT * FROM strategy_definitions WHERE strategy_def_id = :id");
    q.bindValue(":id", defId);

    if (q.exec() && q.next())
        return definitionFromQuery(q);
    return {};
}

bool ModelTreeRepository::updateStrategyDefinition(const DbStrategyDefinition& def)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE strategy_definitions SET "
        "name = :name, strategy_kind = :kind, config_json = :cfg, version = :ver, "
        "lifecycle_state = :state, is_archived = :archived, updated_at = :updated, "
        "created_from_def_id = :from_id "
        "WHERE strategy_def_id = :id");
    q.bindValue(":id",       def.strategyDefId);
    q.bindValue(":name",     def.name);
    q.bindValue(":kind",     def.strategyKind);
    q.bindValue(":cfg",      def.configJson);
    q.bindValue(":ver",      def.version);
    q.bindValue(":state",    def.lifecycleState);
    q.bindValue(":archived", def.isArchived ? 1 : 0);
    q.bindValue(":updated",  def.updatedAt);
    q.bindValue(":from_id",  def.createdFromDefId.isEmpty() ? QVariant() : def.createdFromDefId);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::updateStrategyDefinition failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return q.numRowsAffected() > 0;
}

QList<DbStrategyDefinition> ModelTreeRepository::listStrategyDefinitions(bool includeArchived) const
{
    QList<DbStrategyDefinition> results;
    QSqlQuery q(db());

    if (includeArchived) {
        q.prepare("SELECT * FROM strategy_definitions ORDER BY updated_at DESC");
    } else {
        q.prepare("SELECT * FROM strategy_definitions WHERE is_archived = 0 ORDER BY updated_at DESC");
    }

    if (q.exec()) {
        while (q.next())
            results.append(definitionFromQuery(q));
    }
    return results;
}

bool ModelTreeRepository::archiveStrategyDefinition(const QString& defId)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE strategy_definitions "
        "SET is_archived = 1, updated_at = :now "
        "WHERE strategy_def_id = :id");
    q.bindValue(":now", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.bindValue(":id", defId);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::archiveStrategyDefinition failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return q.numRowsAffected() > 0;
}

// ---------------------------------------------------------------------------
// live_strategy_bindings CRUD
// ---------------------------------------------------------------------------

DbLiveStrategyBinding ModelTreeRepository::bindingFromQuery(const QSqlQuery& q) const
{
    DbLiveStrategyBinding b;
    b.bindingId     = q.value("binding_id").toString();
    b.modelNodeId   = q.value("model_node_id").toString();
    b.strategyDefId = q.value("strategy_def_id").toString();
    b.createdAt     = q.value("created_at").toString();
    b.updatedAt     = q.value("updated_at").toString();
    return b;
}

bool ModelTreeRepository::createLiveBinding(const DbLiveStrategyBinding& binding)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO live_strategy_bindings "
        "(binding_id, model_node_id, strategy_def_id, created_at, updated_at) "
        "VALUES (:bid, :nid, :did, :created, :updated)");
    q.bindValue(":bid",     binding.bindingId);
    q.bindValue(":nid",     binding.modelNodeId);
    q.bindValue(":did",     binding.strategyDefId);
    q.bindValue(":created", binding.createdAt);
    q.bindValue(":updated", binding.updatedAt);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::createLiveBinding failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return true;
}

DbLiveStrategyBinding ModelTreeRepository::fetchBindingForNode(const QString& nodeUuid) const
{
    QSqlQuery q(db());
    q.prepare("SELECT * FROM live_strategy_bindings WHERE model_node_id = :nid");
    q.bindValue(":nid", nodeUuid);

    if (q.exec() && q.next())
        return bindingFromQuery(q);
    return {};
}

QList<DbLiveStrategyBinding> ModelTreeRepository::listBindingsForDefinition(const QString& defId) const
{
    QList<DbLiveStrategyBinding> results;
    QSqlQuery q(db());
    q.prepare("SELECT * FROM live_strategy_bindings WHERE strategy_def_id = :did");
    q.bindValue(":did", defId);

    if (q.exec()) {
        while (q.next())
            results.append(bindingFromQuery(q));
    }
    return results;
}

bool ModelTreeRepository::removeBindingForNode(const QString& nodeUuid)
{
    QSqlQuery q(db());
    q.prepare("DELETE FROM live_strategy_bindings WHERE model_node_id = :nid");
    q.bindValue(":nid", nodeUuid);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::removeBindingForNode failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return q.numRowsAffected() > 0;
}

// ---------------------------------------------------------------------------
// backtest_run_profiles CRUD
// ---------------------------------------------------------------------------

DbBacktestRunProfile ModelTreeRepository::profileFromQuery(const QSqlQuery& q) const
{
    DbBacktestRunProfile p;
    p.profileId     = q.value("profile_id").toString();
    p.ownerType     = q.value("owner_type").toString();
    p.ownerRefId    = q.value("owner_ref_id").toString();
    p.name          = q.value("name").toString();
    p.runConfigJson = q.value("run_config_json").toString();
    p.createdAt     = q.value("created_at").toString();
    p.updatedAt     = q.value("updated_at").toString();
    return p;
}

bool ModelTreeRepository::createRunProfile(const DbBacktestRunProfile& profile)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO backtest_run_profiles "
        "(profile_id, owner_type, owner_ref_id, name, run_config_json, created_at, updated_at) "
        "VALUES (:pid, :otype, :oref, :name, :cfg, :created, :updated)");
    q.bindValue(":pid",     profile.profileId);
    q.bindValue(":otype",   profile.ownerType);
    q.bindValue(":oref",    profile.ownerRefId);
    q.bindValue(":name",    profile.name);
    q.bindValue(":cfg",     profile.runConfigJson);
    q.bindValue(":created", profile.createdAt);
    q.bindValue(":updated", profile.updatedAt);

    if (!q.exec()) {
        qWarning("ModelTreeRepository::createRunProfile failed: %s",
                 qPrintable(q.lastError().text()));
        return false;
    }
    return true;
}

QList<DbBacktestRunProfile> ModelTreeRepository::listRunProfiles(const QString& ownerType,
                                                                   const QString& ownerRefId) const
{
    QList<DbBacktestRunProfile> results;
    QSqlQuery q(db());
    q.prepare(
        "SELECT * FROM backtest_run_profiles "
        "WHERE owner_type = :otype AND owner_ref_id = :oref "
        "ORDER BY updated_at DESC");
    q.bindValue(":otype", ownerType);
    q.bindValue(":oref",  ownerRefId);

    if (q.exec()) {
        while (q.next())
            results.append(profileFromQuery(q));
    }
    return results;
}
