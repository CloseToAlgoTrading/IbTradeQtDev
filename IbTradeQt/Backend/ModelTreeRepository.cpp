#include "ModelTreeRepository.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QVariant>
#include <QUuid>
#include <QStringList>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcModelTree, "backend.modelTree")

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
        qCWarning(lcModelTree) << "ModelTreeRepository: cannot open DB:" << database.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository: create model_nodes failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository: create app_metadata failed:" << q.lastError().text();

        return false;
    }

    // v3 tables: strategies + strategy_versions
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS strategies ("
        "  strategy_id      TEXT PRIMARY KEY,"
        "  name             TEXT NOT NULL DEFAULT '',"
        "  strategy_kind    INTEGER NOT NULL DEFAULT 0,"
        "  lifecycle_state  TEXT NOT NULL DEFAULT 'draft',"
        "  description      TEXT NOT NULL DEFAULT '',"
        "  tags             TEXT NOT NULL DEFAULT '',"
        "  is_archived      INTEGER NOT NULL DEFAULT 0,"
        "  created_at       TEXT NOT NULL,"
        "  updated_at       TEXT NOT NULL"
        ")");
    if (!ok) {
        qCWarning(lcModelTree) << "ModelTreeRepository: create strategies failed:" << q.lastError().text();

        return false;
    }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS strategy_versions ("
        "  version_id              TEXT PRIMARY KEY,"
        "  strategy_id             TEXT NOT NULL REFERENCES strategies(strategy_id),"
        "  version_number          INTEGER NOT NULL DEFAULT 1,"
        "  config_json             TEXT NOT NULL DEFAULT '{}',"
        "  notes                   TEXT NOT NULL DEFAULT '',"
        "  is_published            INTEGER NOT NULL DEFAULT 0,"
        "  created_from_version_id TEXT,"
        "  created_at              TEXT NOT NULL,"
        "  UNIQUE(strategy_id, version_number)"
        ")");
    if (!ok) {
        qCWarning(lcModelTree) << "ModelTreeRepository: create strategy_versions failed:" << q.lastError().text();

        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_versions_strategy ON strategy_versions(strategy_id)");

    // legacy strategy_definitions — still created for fresh DBs so existing
    // code that references it during migration keeps working.
    q.exec(
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

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS live_strategy_bindings ("
        "  binding_id      TEXT PRIMARY KEY,"
        "  model_node_id   TEXT NOT NULL REFERENCES model_nodes(uuid) ON DELETE CASCADE,"
        "  strategy_def_id TEXT NOT NULL,"
        "  version_id      TEXT NOT NULL DEFAULT '',"
        "  created_at      TEXT NOT NULL,"
        "  updated_at      TEXT NOT NULL,"
        "  UNIQUE(model_node_id)"
        ")");
    if (!ok) {
        qCWarning(lcModelTree) << "ModelTreeRepository: create live_strategy_bindings failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository: create backtest_run_profiles failed:" << q.lastError().text();

        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_profiles_owner ON backtest_run_profiles(owner_type, owner_ref_id)");

    // Ensure version_id column exists on live_strategy_bindings (idempotent ALTER).
    // Must run BEFORE migration so migrateV2toV3() can UPDATE version_id.
    q.exec("ALTER TABLE live_strategy_bindings ADD COLUMN version_id TEXT DEFAULT ''");

    // --- Schema version management ---
    QString ver = metadata("schema_version");
    if (ver.isEmpty()) {
        setMetadata("schema_version", "3");
    } else if (ver == "1" || ver == "2") {
        if (!migrateV2toV3())
            qCWarning(lcModelTree) << "ModelTreeRepository: v2->v3 migration failed";
    }

    // --- Repair broken FK on live_strategy_bindings ---
    // The v2→v3 migration renames strategy_definitions to strategy_definitions_backup.
    // SQLite silently rewrites the FK in live_strategy_bindings to reference the
    // backup table, making all subsequent binding INSERTs fail the FK check.
    // Detect and fix by recreating the table without the stale FK.
    repairBindingsTableForeignKey();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::insertNode failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::updateNode failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteNode failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::replaceAll: failed to start transaction";
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
        qCWarning(lcModelTree) << "ModelTreeRepository::setMetadata failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::createStrategyDefinition failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::updateStrategyDefinition failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::archiveStrategyDefinition failed:" << q.lastError().text();

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
    b.versionId     = q.value("version_id").toString();
    b.createdAt     = q.value("created_at").toString();
    b.updatedAt     = q.value("updated_at").toString();
    return b;
}

bool ModelTreeRepository::createLiveBinding(const DbLiveStrategyBinding& binding)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO live_strategy_bindings "
        "(binding_id, model_node_id, strategy_def_id, version_id, created_at, updated_at) "
        "VALUES (:bid, :nid, :did, :vid, :created, :updated)");
    q.bindValue(":bid",     binding.bindingId);
    q.bindValue(":nid",     binding.modelNodeId);
    q.bindValue(":did",     binding.strategyDefId);
    q.bindValue(":vid",     binding.versionId);
    q.bindValue(":created", binding.createdAt);
    q.bindValue(":updated", binding.updatedAt);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::createLiveBinding failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::removeBindingForNode failed:" << q.lastError().text();

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
        qCWarning(lcModelTree) << "ModelTreeRepository::createRunProfile failed:" << q.lastError().text();

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

// ---------------------------------------------------------------------------
// strategies CRUD (v3 catalog)
// ---------------------------------------------------------------------------

DbStrategy ModelTreeRepository::strategyCatalogFromQuery(const QSqlQuery& q) const
{
    DbStrategy s;
    s.strategyId     = q.value("strategy_id").toString();
    s.name           = q.value("name").toString();
    s.strategyKind   = q.value("strategy_kind").toInt();
    s.lifecycleState = q.value("lifecycle_state").toString();
    s.description    = q.value("description").toString();
    s.tags           = q.value("tags").toString();
    s.isArchived     = q.value("is_archived").toInt() != 0;
    s.createdAt      = q.value("created_at").toString();
    s.updatedAt      = q.value("updated_at").toString();
    return s;
}

bool ModelTreeRepository::createStrategyCatalog(const DbStrategy& strategy)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO strategies "
        "(strategy_id, name, strategy_kind, lifecycle_state, description, tags, "
        " is_archived, created_at, updated_at) "
        "VALUES (:id, :name, :kind, :state, :desc, :tags, :archived, :created, :updated)");
    q.bindValue(":id",       strategy.strategyId);
    q.bindValue(":name",     strategy.name);
    q.bindValue(":kind",     strategy.strategyKind);
    q.bindValue(":state",    strategy.lifecycleState);
    q.bindValue(":desc",     strategy.description);
    q.bindValue(":tags",     strategy.tags);
    q.bindValue(":archived", strategy.isArchived ? 1 : 0);
    q.bindValue(":created",  strategy.createdAt);
    q.bindValue(":updated",  strategy.updatedAt);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::createStrategyCatalog failed:" << q.lastError().text();

        return false;
    }
    return true;
}

DbStrategy ModelTreeRepository::fetchStrategyCatalog(const QString& strategyId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT * FROM strategies WHERE strategy_id = :id");
    q.bindValue(":id", strategyId);

    if (q.exec() && q.next())
        return strategyCatalogFromQuery(q);
    return {};
}

QList<DbStrategy> ModelTreeRepository::listStrategyCatalog(bool includeArchived) const
{
    QList<DbStrategy> results;
    QSqlQuery q(db());

    if (includeArchived)
        q.prepare("SELECT * FROM strategies ORDER BY updated_at DESC");
    else
        q.prepare("SELECT * FROM strategies WHERE is_archived = 0 ORDER BY updated_at DESC");

    if (q.exec()) {
        while (q.next())
            results.append(strategyCatalogFromQuery(q));
    }
    return results;
}

bool ModelTreeRepository::updateStrategyCatalog(const DbStrategy& strategy)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE strategies SET "
        "name = :name, strategy_kind = :kind, lifecycle_state = :state, "
        "description = :desc, tags = :tags, is_archived = :archived, "
        "updated_at = :updated "
        "WHERE strategy_id = :id");
    q.bindValue(":id",       strategy.strategyId);
    q.bindValue(":name",     strategy.name);
    q.bindValue(":kind",     strategy.strategyKind);
    q.bindValue(":state",    strategy.lifecycleState);
    q.bindValue(":desc",     strategy.description);
    q.bindValue(":tags",     strategy.tags);
    q.bindValue(":archived", strategy.isArchived ? 1 : 0);
    q.bindValue(":updated",  strategy.updatedAt);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::updateStrategyCatalog failed:" << q.lastError().text();

        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ModelTreeRepository::archiveStrategyCatalog(const QString& strategyId)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE strategies "
        "SET is_archived = 1, updated_at = :now "
        "WHERE strategy_id = :id");
    q.bindValue(":now", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.bindValue(":id", strategyId);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::archiveStrategyCatalog failed:" << q.lastError().text();

        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ModelTreeRepository::deleteStrategyCatalogCascade(const QString& strategyId,
                                                       const QStringList& purgeProfileOwnerRefs)
{
    QSqlDatabase database = db();
    if (!database.transaction()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyCatalogCascade: begin failed:"
                               << database.lastError().text();
        return false;
    }

    for (const QString& ref : purgeProfileOwnerRefs) {
        if (ref.isEmpty())
            continue;
        QSqlQuery qp(database);
        qp.prepare(QStringLiteral("DELETE FROM backtest_run_profiles WHERE owner_ref_id = :ref"));
        qp.bindValue(QStringLiteral(":ref"), ref);
        if (!qp.exec()) {
            qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyCatalogCascade: delete profiles failed:"
                                   << qp.lastError().text();
            database.rollback();
            return false;
        }
    }

    QSqlQuery qv(database);
    qv.prepare(QStringLiteral("DELETE FROM strategy_versions WHERE strategy_id = :sid"));
    qv.bindValue(QStringLiteral(":sid"), strategyId);
    if (!qv.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyCatalogCascade: delete versions failed:"
                               << qv.lastError().text();
        database.rollback();
        return false;
    }

    QSqlQuery qb(database);
    qb.prepare(QStringLiteral("DELETE FROM live_strategy_bindings WHERE strategy_def_id = :sid"));
    qb.bindValue(QStringLiteral(":sid"), strategyId);
    if (!qb.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyCatalogCascade: delete bindings failed:"
                               << qb.lastError().text();
        database.rollback();
        return false;
    }

    QSqlQuery qs(database);
    qs.prepare(QStringLiteral("DELETE FROM strategies WHERE strategy_id = :sid"));
    qs.bindValue(QStringLiteral(":sid"), strategyId);
    if (!qs.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyCatalogCascade: delete strategy failed:"
                               << qs.lastError().text();
        database.rollback();
        return false;
    }

    if (qs.numRowsAffected() <= 0) {
        database.rollback();
        return false;
    }

    if (!database.commit()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyCatalogCascade: commit failed:"
                               << database.lastError().text();
        database.rollback();
        return false;
    }
    return true;
}

int ModelTreeRepository::removeOrphanedCatalogEntries()
{
    QSqlQuery q(db());
    // Delete catalog entries that no live binding references
    bool ok = q.exec(
        "DELETE FROM strategies "
        "WHERE strategy_id NOT IN ("
        "  SELECT DISTINCT strategy_def_id FROM live_strategy_bindings"
        ")");
    if (!ok) {
        qCWarning(lcModelTree) << "ModelTreeRepository::removeOrphanedCatalogEntries failed:" << q.lastError().text();

        return 0;
    }
    int removed = q.numRowsAffected();

    // Also clean up orphaned versions whose strategy_id no longer exists
    q.exec(
        "DELETE FROM strategy_versions "
        "WHERE strategy_id NOT IN (SELECT strategy_id FROM strategies)");

    // And orphaned legacy definitions
    q.exec(
        "DELETE FROM strategy_definitions "
        "WHERE strategy_def_id NOT IN ("
        "  SELECT DISTINCT strategy_def_id FROM live_strategy_bindings"
        ") AND strategy_def_id NOT IN ("
        "  SELECT strategy_id FROM strategies"
        ")");

    return removed;
}

// ---------------------------------------------------------------------------
// strategy_versions CRUD
// ---------------------------------------------------------------------------

DbStrategyVersion ModelTreeRepository::versionFromQuery(const QSqlQuery& q) const
{
    DbStrategyVersion v;
    v.versionId            = q.value("version_id").toString();
    v.strategyId           = q.value("strategy_id").toString();
    v.versionNumber        = q.value("version_number").toInt();
    v.configJson           = q.value("config_json").toString();
    v.notes                = q.value("notes").toString();
    v.isPublished          = q.value("is_published").toInt() != 0;
    v.createdFromVersionId = q.value("created_from_version_id").toString();
    v.createdAt            = q.value("created_at").toString();
    return v;
}

bool ModelTreeRepository::createStrategyVersion(const DbStrategyVersion& version)
{
    QSqlQuery q(db());
    q.prepare(
        "INSERT INTO strategy_versions "
        "(version_id, strategy_id, version_number, config_json, notes, "
        " is_published, created_from_version_id, created_at) "
        "VALUES (:vid, :sid, :vnum, :cfg, :notes, :pub, :from_vid, :created)");
    q.bindValue(":vid",      version.versionId);
    q.bindValue(":sid",      version.strategyId);
    q.bindValue(":vnum",     version.versionNumber);
    q.bindValue(":cfg",      version.configJson);
    q.bindValue(":notes",    version.notes);
    q.bindValue(":pub",      version.isPublished ? 1 : 0);
    q.bindValue(":from_vid", version.createdFromVersionId.isEmpty()
                                 ? QVariant() : version.createdFromVersionId);
    q.bindValue(":created",  version.createdAt);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::createStrategyVersion failed:" << q.lastError().text();

        return false;
    }
    return true;
}

DbStrategyVersion ModelTreeRepository::fetchStrategyVersion(const QString& versionId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT * FROM strategy_versions WHERE version_id = :vid");
    q.bindValue(":vid", versionId);

    if (q.exec() && q.next())
        return versionFromQuery(q);
    return {};
}

QList<DbStrategyVersion> ModelTreeRepository::listStrategyVersions(const QString& strategyId) const
{
    QList<DbStrategyVersion> results;
    QSqlQuery q(db());
    q.prepare("SELECT * FROM strategy_versions WHERE strategy_id = :sid ORDER BY version_number ASC");
    q.bindValue(":sid", strategyId);

    if (q.exec()) {
        while (q.next())
            results.append(versionFromQuery(q));
    }
    return results;
}

DbStrategyVersion ModelTreeRepository::fetchLatestVersion(const QString& strategyId) const
{
    QSqlQuery q(db());
    q.prepare(
        "SELECT * FROM strategy_versions "
        "WHERE strategy_id = :sid "
        "ORDER BY version_number DESC LIMIT 1");
    q.bindValue(":sid", strategyId);

    if (q.exec() && q.next())
        return versionFromQuery(q);
    return {};
}

bool ModelTreeRepository::deleteStrategyVersion(const QString& versionId)
{
    QSqlQuery q(db());
    q.prepare("DELETE FROM strategy_versions WHERE version_id = :vid");
    q.bindValue(":vid", versionId);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::deleteStrategyVersion failed:"
                               << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ModelTreeRepository::setVersionPublished(const QString& versionId, bool published)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE strategy_versions "
        "SET is_published = :pub "
        "WHERE version_id = :vid");
    q.bindValue(":pub", published ? 1 : 0);
    q.bindValue(":vid", versionId);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::setVersionPublished failed:" << q.lastError().text();

        return false;
    }
    return q.numRowsAffected() > 0;
}

int ModelTreeRepository::nextVersionNumber(const QString& strategyId) const
{
    QSqlQuery q(db());
    q.prepare(
        "SELECT COALESCE(MAX(version_number), -1) + 1 "
        "FROM strategy_versions WHERE strategy_id = :sid");
    q.bindValue(":sid", strategyId);

    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

bool ModelTreeRepository::updateBindingVersion(const QString& bindingId, const QString& versionId)
{
    QSqlQuery q(db());
    q.prepare(
        "UPDATE live_strategy_bindings "
        "SET version_id = :vid, updated_at = :now "
        "WHERE binding_id = :bid");
    q.bindValue(":vid", versionId);
    q.bindValue(":now", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.bindValue(":bid", bindingId);

    if (!q.exec()) {
        qCWarning(lcModelTree) << "ModelTreeRepository::updateBindingVersion failed:" << q.lastError().text();

        return false;
    }
    return q.numRowsAffected() > 0;
}

// ---------------------------------------------------------------------------
// v2 → v3 migration
// ---------------------------------------------------------------------------

bool ModelTreeRepository::migrateV2toV3()
{
    QSqlDatabase database = db();

    // Check if strategy_definitions table exists and has data to migrate
    QSqlQuery check(database);
    check.exec("SELECT COUNT(*) FROM strategy_definitions");
    if (!check.next() || check.value(0).toInt() == 0) {
        setMetadata("schema_version", "3");
        return true;
    }

    if (!database.transaction()) {
        qCWarning(lcModelTree) << "migrateV2toV3: failed to start transaction";
        return false;
    }

    QSqlQuery q(database);

    // Migrate each strategy_definitions row into strategies + strategy_versions
    q.exec("SELECT * FROM strategy_definitions");
    while (q.next()) {
        DbStrategyDefinition def = definitionFromQuery(q);
        QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        // Insert into strategies (reuse the same defId as strategyId)
        QSqlQuery ins(database);
        ins.prepare(
            "INSERT OR IGNORE INTO strategies "
            "(strategy_id, name, strategy_kind, lifecycle_state, description, tags, "
            " is_archived, created_at, updated_at) "
            "VALUES (:id, :name, :kind, :state, '', '', :archived, :created, :updated)");
        ins.bindValue(":id",       def.strategyDefId);
        ins.bindValue(":name",     def.name);
        ins.bindValue(":kind",     def.strategyKind);
        ins.bindValue(":state",    def.lifecycleState);
        ins.bindValue(":archived", def.isArchived ? 1 : 0);
        ins.bindValue(":created",  def.createdAt);
        ins.bindValue(":updated",  def.updatedAt);
        if (!ins.exec()) {
            qCWarning(lcModelTree) << "migrateV2toV3: insert into strategies failed:" << ins.lastError().text();

        }

        // Create a single version row for the current config
        QString versionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSqlQuery insV(database);
        insV.prepare(
            "INSERT OR IGNORE INTO strategy_versions "
            "(version_id, strategy_id, version_number, config_json, notes, "
            " is_published, created_from_version_id, created_at) "
            "VALUES (:vid, :sid, :vnum, :cfg, :notes, 1, NULL, :created)");
        insV.bindValue(":vid",     versionId);
        insV.bindValue(":sid",     def.strategyDefId);
        insV.bindValue(":vnum",    def.version);
        insV.bindValue(":cfg",     def.configJson);
        insV.bindValue(":notes",   QStringLiteral("Migrated from v2 strategy_definitions"));
        insV.bindValue(":created", def.createdAt);
        if (!insV.exec()) {
            qCWarning(lcModelTree) << "migrateV2toV3: insert into strategy_versions failed:" << insV.lastError().text();

        }

        // Update matching live_strategy_bindings to set version_id
        QSqlQuery updB(database);
        updB.prepare(
            "UPDATE live_strategy_bindings "
            "SET version_id = :vid "
            "WHERE strategy_def_id = :did AND (version_id IS NULL OR version_id = '')");
        updB.bindValue(":vid", versionId);
        updB.bindValue(":did", def.strategyDefId);
        updB.exec();
    }

    // Rename old table to backup.
    // IMPORTANT: SQLite automatically rewrites FK references in other tables
    // when a table is renamed, so live_strategy_bindings.strategy_def_id will
    // now point to strategy_definitions_backup. The repairBindingsTableForeignKey()
    // method (called after migration) corrects this.
    q.exec("ALTER TABLE strategy_definitions RENAME TO strategy_definitions_backup");

    // Recreate strategy_definitions (empty) so fresh code referencing it doesn't fail
    q.exec(
        "CREATE TABLE IF NOT EXISTS strategy_definitions ("
        "  strategy_def_id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL DEFAULT '',"
        "  strategy_kind INTEGER NOT NULL DEFAULT 0,"
        "  config_json TEXT NOT NULL DEFAULT '{}',"
        "  version INTEGER NOT NULL DEFAULT 1,"
        "  lifecycle_state TEXT NOT NULL DEFAULT 'draft',"
        "  is_archived INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT NOT NULL,"
        "  updated_at TEXT NOT NULL,"
        "  created_from_def_id TEXT"
        ")");

    if (!database.commit()) {
        database.rollback();
        qCWarning(lcModelTree) << "migrateV2toV3: commit failed";
        return false;
    }

    setMetadata("schema_version", "3");
    return true;
}

void ModelTreeRepository::repairBindingsTableForeignKey()
{
    QSqlDatabase database = db();

    // Check if live_strategy_bindings has a stale FK to strategy_definitions_backup.
    // SQLite's ALTER TABLE RENAME propagates to FK references automatically,
    // so after migrateV2toV3() renames strategy_definitions → strategy_definitions_backup,
    // the FK in live_strategy_bindings silently changes to reference the backup table.
    QSqlQuery check(database);
    check.exec("SELECT sql FROM sqlite_master WHERE name='live_strategy_bindings'");
    if (!check.next()) return;

    QString createSql = check.value(0).toString();
    if (!createSql.contains("strategy_definitions_backup"))
        return;

    qCWarning(lcModelTree) << "[schema-repair] live_strategy_bindings has stale FK to "
                              "strategy_definitions_backup — recreating table";

    if (!database.transaction()) return;

    QSqlQuery q(database);
    q.exec("PRAGMA foreign_keys = OFF");

    // Copy existing data (if any)
    q.exec("ALTER TABLE live_strategy_bindings RENAME TO _bindings_old");
    q.exec(
        "CREATE TABLE live_strategy_bindings ("
        "  binding_id      TEXT PRIMARY KEY,"
        "  model_node_id   TEXT NOT NULL REFERENCES model_nodes(uuid) ON DELETE CASCADE,"
        "  strategy_def_id TEXT NOT NULL,"
        "  version_id      TEXT NOT NULL DEFAULT '',"
        "  created_at      TEXT NOT NULL,"
        "  updated_at      TEXT NOT NULL,"
        "  UNIQUE(model_node_id)"
        ")");
    q.exec("INSERT INTO live_strategy_bindings "
           "SELECT binding_id, model_node_id, strategy_def_id, "
           "       COALESCE(version_id,''), created_at, updated_at "
           "FROM _bindings_old");
    q.exec("DROP TABLE _bindings_old");
    q.exec("CREATE INDEX IF NOT EXISTS idx_bindings_def "
           "ON live_strategy_bindings(strategy_def_id)");

    q.exec("PRAGMA foreign_keys = ON");

    if (!database.commit()) {
        database.rollback();
        qCWarning(lcModelTree) << "[schema-repair] commit failed, rolling back";
    }
}
