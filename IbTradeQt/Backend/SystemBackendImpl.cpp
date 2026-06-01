#include "SystemBackendImpl.h"
#include "cbasicroot.h"
#include "cbasicaccount.h"
#include "cbasicportfolio.h"
#include "cstrategyfactory.h"
#include "cpipelinestrategyadapter.h"
#include "ModelTreeMapper.h"
#include "PipelineConstants.h"
#include "NHelper.h"
#include "dbquery.h"
#include <QUuid>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <functional>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSystemBackend, "backend.system")

namespace {

bool deleteBacktestPersistedDataForCatalog(const QString& strategyId)
{
    const QString path = NHelper::getStorageConfig().appDataStore.path;
    if (path.isEmpty() || strategyId.isEmpty())
        return false;

    const QString connName = QStringLiteral("bt_cat_del_")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(connName);
            return false;
        }
        QSqlQuery pragma(db);
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        ok = query_deleteBacktestDataForCatalogStrategy(strategyId, connName);
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
    return ok;
}

} // namespace

SystemBackendImpl::SystemBackendImpl(IModelTreeRepository* repo, QObject* parent)
    : ISystemBackend(parent)
    , m_repo(repo)
{
    m_root = new CBasicRoot();
    m_root->setId(QUuid::createUuid());
}

SystemBackendImpl::~SystemBackendImpl()
{
    delete m_root;
}

bool SystemBackendImpl::isStrategyType(ModelType type)
{
    return type == ModelType::STRATEGY
        || type == ModelType::STRATEGY_BASIC_TEST
        || type == ModelType::STRATEGY_MA
        || type == ModelType::STRATEGY_MOMENTUM
        || type == ModelType::STRATEGY_PIPELINE;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

QString SystemBackendImpl::nowUtcIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QJsonObject SystemBackendImpl::extractCanonicalStrategyConfig(const QJsonObject& raw)
{
    QJsonObject canonical;
    // Whitelist only the truly independent fields that define strategy behaviour.
    // `parameters` is deliberately excluded: it is a runtime-derived view on top of
    // `pipelineConfig` (populated via updateParametersFromConfig).  Versioning
    // `pipelineConfig` alone is sufficient and avoids spurious version bumps caused
    // by default bt_* / execution_mode entries being added when setPipelineConfig is
    // called for the first time after model creation.
    // Excludes: uuid, name, genericInfo, parameters (derived), and deployment-only fields.
    static const QLatin1StringView keys[] = {
        Pipeline::Key::PipelineConfig,
        Pipeline::Key::AssetList,
    };
    for (auto k : keys) {
        if (raw.contains(k))
            canonical[k] = raw[k];
    }
    return canonical;
}

QJsonObject SystemBackendImpl::definitionToJson(const DbStrategyDefinition& def)
{
    QJsonObject obj;
    obj["strategyDefId"]    = def.strategyDefId;
    obj["name"]             = def.name;
    obj["strategyKind"]     = def.strategyKind;
    obj["configJson"]       = def.configJson;
    obj["version"]          = def.version;
    obj["lifecycleState"]   = def.lifecycleState;
    obj["isArchived"]       = def.isArchived;
    obj["createdAt"]        = def.createdAt;
    obj["updatedAt"]        = def.updatedAt;
    if (!def.createdFromDefId.isEmpty())
        obj["createdFromDefId"] = def.createdFromDefId;
    return obj;
}

void SystemBackendImpl::createCatalogEntryAndBinding(const QString& nodeUuid,
                                                       const QString& name,
                                                       int strategyKind,
                                                       const QJsonObject& canonicalConfig)
{
    QString stratId   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString versionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now       = nowUtcIso();

    // 1. Create the catalog strategy entry
    DbStrategy strat;
    strat.strategyId     = stratId;
    strat.name           = name;
    strat.strategyKind   = strategyKind;
    strat.lifecycleState = QStringLiteral("draft");
    strat.isArchived     = false;
    strat.createdAt      = now;
    strat.updatedAt      = now;

    if (!m_repo->createStrategyCatalog(strat)) {
        qCWarning(lcSystemBackend) << "SystemBackendImpl::createCatalogEntryAndBinding: "
                                      "failed to persist strategies row for node"
                                   << nodeUuid;
        return;
    }

    // 2. Create v0 version (published by default)
    DbStrategyVersion ver;
    ver.versionId     = versionId;
    ver.strategyId    = stratId;
    ver.versionNumber = 0;
    ver.configJson    = QString::fromUtf8(
        QJsonDocument(canonicalConfig).toJson(QJsonDocument::Compact));
    ver.isPublished   = true;
    ver.createdAt     = now;

    if (!m_repo->createStrategyVersion(ver)) {
        qCWarning(lcSystemBackend) << "SystemBackendImpl::createCatalogEntryAndBinding: "
                                      "failed to persist strategy_versions row for node"
                                   << nodeUuid;
        return;
    }

    // 3. Also create legacy strategy_definitions row for backward compat
    DbStrategyDefinition def;
    def.strategyDefId  = stratId;
    def.name           = name;
    def.strategyKind   = strategyKind;
    def.configJson     = ver.configJson;
    def.version        = 0;
    def.lifecycleState = QStringLiteral("draft");
    def.isArchived     = false;
    def.createdAt      = now;
    def.updatedAt      = now;
    m_repo->createStrategyDefinition(def);

    // 4. Create binding with version_id
    DbLiveStrategyBinding binding;
    binding.bindingId     = QUuid::createUuid().toString(QUuid::WithoutBraces);
    binding.modelNodeId   = nodeUuid;
    binding.strategyDefId = stratId;
    binding.versionId     = versionId;
    binding.createdAt     = now;
    binding.updatedAt     = now;

    if (!m_repo->createLiveBinding(binding)) {
        qCWarning(lcSystemBackend) << "SystemBackendImpl::createCatalogEntryAndBinding: "
                                      "failed to persist live_strategy_bindings row for node"
                                   << nodeUuid;
        return;
    }

    // Populate runtime field on the in-memory adapter
    CGenericModelApi* node = findNodeByUuid(nodeUuid);
    if (auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node))
        adapter->setStrategyDefinitionId(stratId);
}

void SystemBackendImpl::detectVersionDivergence(const QString& strategyNodeUuid)
{
    CGenericModelApi* node = findNodeByUuid(strategyNodeUuid);
    if (!node) return;

    DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(strategyNodeUuid);
    if (!binding.isValid() || binding.versionId.isEmpty())
        return;

    DbStrategyVersion ver = m_repo->fetchStrategyVersion(binding.versionId);
    if (!ver.isValid())
        return;

    QJsonObject rawConfig = ModelTreeMapper::nodeConfigJson(node);
    QJsonObject canonical = extractCanonicalStrategyConfig(rawConfig);
    QByteArray  newBytes  = QJsonDocument(canonical).toJson(QJsonDocument::Compact);

    QByteArray existingBytes = QJsonDocument::fromJson(ver.configJson.toUtf8())
                                   .toJson(QJsonDocument::Compact);
    if (newBytes != existingBytes)
        emit nodeConfigDiverged(strategyNodeUuid);
}

bool SystemBackendImpl::isDescendantOf(CGenericModelApi* node, CGenericModelApi* potentialAncestor)
{
    CGenericModelApi* current = node->getParentModel();
    while (current) {
        if (current == potentialAncestor) return true;
        current = current->getParentModel();
    }
    return false;
}

CGenericModelApi* SystemBackendImpl::findNodeByUuid(const QString& uuid) const
{
    return m_uuidIndex.value(uuid, nullptr);
}

void SystemBackendImpl::rebuildUuidIndex()
{
    m_uuidIndex.clear();
    if (!m_root) return;

    std::function<void(CGenericModelApi*)> visit = [&](CGenericModelApi* node) {
        QString uuid = node->getId().toString(QUuid::WithoutBraces);
        if (!uuid.isEmpty())
            m_uuidIndex.insert(uuid, node);
        for (auto& child : node->getModels())
            visit(child.data());
    };
    visit(m_root);
}

void SystemBackendImpl::wireAllRuntimeSignals()
{
    if (!m_root) return;
    std::function<void(CGenericModelApi*)> visit = [&](CGenericModelApi* node) {
        wireRuntimeSignals(node);
        for (auto& child : node->getModels())
            visit(child.data());
    };
    visit(m_root);
}

void SystemBackendImpl::wireRuntimeSignals(CGenericModelApi* node)
{
    auto* baseModel = dynamic_cast<CBaseModel*>(node);
    if (!baseModel) return;
    QString uuid = node->getId().toString(QUuid::WithoutBraces);
    connect(baseModel, &CBaseModel::displayStateChanged, this,
        [this, uuid](DisplayState, DisplayState newState) {
            emit strategyStateChanged(uuid, static_cast<int>(newState));
        });
}

void SystemBackendImpl::persistNode(CGenericModelApi* node)
{
    QString uuid = node->getId().toString(QUuid::WithoutBraces);
    auto existing = m_repo->fetchNode(uuid);
    if (!existing) return;

    ModelNodeRecord rec = *existing;
    rec.name = node->getName();
    rec.config = ModelTreeMapper::nodeConfigJson(node);
    rec.updatedAt = QDateTime::currentDateTimeUtc();
    m_repo->updateNode(rec);
}

// ---- Topology CRUD ----

QString SystemBackendImpl::createAccount(const QString& name)
{
    QUuid id = QUuid::createUuid();
    QString uuid = id.toString(QUuid::WithoutBraces);

    ModelNodeRecord rec;
    rec.uuid = uuid;
    rec.parentUuid = "";
    rec.modelType = static_cast<int>(ModelType::ACCOUNT);
    rec.name = name;
    rec.sortOrder = m_repo->nextSortOrder("");
    rec.isActive = true;
    rec.createdAt = QDateTime::currentDateTimeUtc();
    rec.updatedAt = rec.createdAt;

    if (!m_repo->insertNode(rec)) {
        qCWarning(lcSystemBackend) << "SystemBackend: failed to persist account" << uuid;
        return {};
    }

    auto model = QSharedPointer<CBasicAccount>::create();
    model->setId(id);
    model->setName(name);
    model->setParentActivationState(true);
    model->setParentModel(m_root);
    m_root->getModels().append(model);

    m_uuidIndex.insert(uuid, model.data());
    wireRuntimeSignals(model.data());
    emit nodeCreated(uuid, "", rec.modelType);
    return uuid;
}

QString SystemBackendImpl::createPortfolio(const QString& accountId, const QString& name)
{
    CGenericModelApi* parent = findNodeByUuid(accountId);
    if (!parent || parent->modelType() != ModelType::ACCOUNT) return {};

    QUuid id = QUuid::createUuid();
    QString uuid = id.toString(QUuid::WithoutBraces);

    ModelNodeRecord rec;
    rec.uuid = uuid;
    rec.parentUuid = accountId;
    rec.modelType = static_cast<int>(ModelType::PORTFOLIO);
    rec.name = name;
    rec.sortOrder = m_repo->nextSortOrder(accountId);
    rec.isActive = true;
    rec.createdAt = QDateTime::currentDateTimeUtc();
    rec.updatedAt = rec.createdAt;

    if (!m_repo->insertNode(rec)) {
        qCWarning(lcSystemBackend) << "SystemBackend: failed to persist portfolio" << uuid;
        return {};
    }

    auto model = QSharedPointer<CBasicPortfolio>::create();
    model->setId(id);
    model->setName(name);
    model->setParentModel(parent);
    parent->getModels().append(model);

    m_uuidIndex.insert(uuid, model.data());
    wireRuntimeSignals(model.data());
    emit nodeCreated(uuid, accountId, rec.modelType);
    return uuid;
}

QString SystemBackendImpl::createStrategy(const QString& portfolioId, ModelType type)
{
    if (!isStrategyType(type)) return {};

    CGenericModelApi* parent = findNodeByUuid(portfolioId);
    if (!parent || parent->modelType() != ModelType::PORTFOLIO) return {};

    QUuid id = QUuid::createUuid();
    QString uuid = id.toString(QUuid::WithoutBraces);

    auto model = CStrategyFactory::createNewStrategy(type);
    if (!model) return {};

    model->setId(id);
    model->setName("Strategy");

    ModelNodeRecord rec;
    rec.uuid = uuid;
    rec.parentUuid = portfolioId;
    rec.modelType = static_cast<int>(type);
    rec.name = model->getName();
    rec.config = ModelTreeMapper::nodeConfigJson(model.data());
    rec.sortOrder = m_repo->nextSortOrder(portfolioId);
    rec.isActive = true;
    rec.createdAt = QDateTime::currentDateTimeUtc();
    rec.updatedAt = rec.createdAt;

    if (!m_repo->insertNode(rec)) {
        qCWarning(lcSystemBackend) << "SystemBackend: failed to persist strategy" << uuid;
        return {};
    }

    model->setParentModel(parent);
    parent->getModels().append(model);

    m_uuidIndex.insert(uuid, model.data());
    wireRuntimeSignals(model.data());

    // Auto-create catalog entry + v1 + live binding for the new strategy node.
    QJsonObject rawConfig  = ModelTreeMapper::nodeConfigJson(model.data());
    QJsonObject canonical  = extractCanonicalStrategyConfig(rawConfig);
    createCatalogEntryAndBinding(uuid, model->getName(), static_cast<int>(type), canonical);

    emit nodeCreated(uuid, portfolioId, rec.modelType);
    return uuid;
}

bool SystemBackendImpl::removeNode(const QString& uuid)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node || node == m_root) return false;
    CGenericModelApi* parent = node->getParentModel();
    if (!parent) return false;

    // For strategy nodes: remove the live binding but keep the definition intact.
    // Definition is a research artifact and stays in the catalog until explicitly archived.
    if (isStrategyType(static_cast<ModelType>(node->modelType()))) {
        m_repo->removeBindingForNode(uuid);
        // Definition row is intentionally NOT deleted or auto-archived here.
    }

    if (!m_repo->deleteNode(uuid)) {
        qCWarning(lcSystemBackend) << "SystemBackend: failed to delete node" << uuid;
        return false;
    }

    auto& siblings = parent->getModels();
    for (int i = 0; i < siblings.size(); ++i) {
        if (siblings[i]->getId().toString(QUuid::WithoutBraces) == uuid) {
            siblings.removeAt(i);
            break;
        }
    }
    rebuildUuidIndex();

    emit nodeRemoved(uuid);
    return true;
}

bool SystemBackendImpl::renameNode(const QString& uuid, const QString& name)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node || node == m_root) return false;

    auto existing = m_repo->fetchNode(uuid);
    if (!existing) return false;

    ModelNodeRecord rec = *existing;
    rec.name = name;
    rec.updatedAt = QDateTime::currentDateTimeUtc();

    if (!m_repo->updateNode(rec)) {
        qCWarning(lcSystemBackend) << "SystemBackend: failed to rename node" << uuid;
        return false;
    }

    node->setName(name);
    emit nodeRenamed(uuid, name);
    return true;
}

bool SystemBackendImpl::setNodeActive(const QString& uuid, bool active)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node || node == m_root) return false;

    auto existing = m_repo->fetchNode(uuid);
    if (!existing) return false;

    ModelNodeRecord rec = *existing;
    rec.isActive = active;
    rec.updatedAt = QDateTime::currentDateTimeUtc();

    if (!m_repo->updateNode(rec)) return false;

    node->setActivationState(active);
    emit nodeActiveChanged(uuid, active);
    return true;
}

bool SystemBackendImpl::moveNode(const QString& uuid, const QString& newParentUuid, int newSortOrder)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    CGenericModelApi* newParent = newParentUuid.isEmpty() ? m_root : findNodeByUuid(newParentUuid);
    if (!node || !newParent || node == m_root) return false;

    ModelType nodeType = node->modelType();
    ModelType parentType = newParent->modelType();

    if (nodeType == ModelType::ACCOUNT && newParent != m_root) return false;
    if (nodeType == ModelType::PORTFOLIO && parentType != ModelType::ACCOUNT) return false;
    if (isStrategyType(nodeType) && parentType != ModelType::PORTFOLIO) return false;

    if (isDescendantOf(newParent, node)) return false;

    auto existing = m_repo->fetchNode(uuid);
    if (!existing) return false;

    ModelNodeRecord rec = *existing;
    rec.parentUuid = newParentUuid;
    rec.sortOrder = newSortOrder;
    rec.updatedAt = QDateTime::currentDateTimeUtc();

    if (!m_repo->updateNode(rec)) return false;

    CGenericModelApi* oldParent = node->getParentModel();
    if (oldParent) {
        auto& oldSiblings = oldParent->getModels();
        for (int i = 0; i < oldSiblings.size(); ++i) {
            if (oldSiblings[i]->getId().toString(QUuid::WithoutBraces) == uuid) {
                ptrGenericModelType moved = oldSiblings.takeAt(i);
                moved->setParentModel(newParent);
                newParent->getModels().append(moved);
                break;
            }
        }
    }

    emit nodeMoved(uuid, newParentUuid);
    return true;
}

// ---- Embedded Block CRUD ----

QString SystemBackendImpl::categoryToJsonKey(const QString& category, bool& isArray)
{
    QLatin1StringView key = Pipeline::categoryKey(category);
    isArray = Pipeline::categoryIsArray(category);
    return key.isEmpty() ? category : QString(key);
}

bool SystemBackendImpl::addBlock(const QString& strategyId, const QString& category, const QString& blockId, const QJsonObject& defaultConfig)
{
    CGenericModelApi* node = findNodeByUuid(strategyId);
    if (!node) return false;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node);
    if (!adapter) return false;

    QJsonObject config = adapter->pipelineConfig();
    bool isArray = false;
    QString key = categoryToJsonKey(category, isArray);

    if (isArray) {
        QJsonArray arr = config.value(key).toArray();
        QJsonObject block;
        block[Pipeline::Key::BlockId] = blockId;
        block[Pipeline::Key::Config]  = defaultConfig;
        arr.append(block);
        config[key] = arr;
    } else {
        QJsonObject block;
        block[Pipeline::Key::BlockId] = blockId;
        block[Pipeline::Key::Config]  = defaultConfig;
        config[key] = block;
    }

    adapter->setPipelineConfig(config);

    persistNode(node);
    detectVersionDivergence(strategyId);

    emit pipelineConfigChanged(strategyId, config);
    return true;
}

bool SystemBackendImpl::removeBlock(const QString& strategyId, const QString& category, int index)
{
    CGenericModelApi* node = findNodeByUuid(strategyId);
    if (!node) return false;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node);
    if (!adapter) return false;

    QJsonObject config = adapter->pipelineConfig();
    bool isArray = false;
    QString key = categoryToJsonKey(category, isArray);

    if (isArray) {
        QJsonArray arr = config.value(key).toArray();
        if (index < 0 || index >= arr.size()) return false;
        arr.removeAt(index);
        config[key] = arr;
    } else {
        config.remove(key);
    }

    adapter->setPipelineConfig(config);
    persistNode(node);
    detectVersionDivergence(strategyId);

    emit pipelineConfigChanged(strategyId, config);
    return true;
}

// ---- Config ----

QJsonObject SystemBackendImpl::nodeConfig(const QString& uuid) const
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node) return {};
    return ModelTreeMapper::nodeConfigJson(node);
}

bool SystemBackendImpl::updateNodeConfig(const QString& uuid, const QJsonObject& config)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node || node == m_root) return false;

    auto existing = m_repo->fetchNode(uuid);
    if (!existing) return false;

    node->fromJson(config);

    ModelNodeRecord rec = *existing;
    rec.config = ModelTreeMapper::nodeConfigJson(node);
    rec.name = node->getName();
    rec.updatedAt = QDateTime::currentDateTimeUtc();

    if (!m_repo->updateNode(rec)) return false;

    // Detect divergence for strategy nodes (no auto-version creation)
    if (isStrategyType(static_cast<ModelType>(node->modelType())))
        detectVersionDivergence(uuid);

    emit configChanged(uuid);
    return true;
}

QJsonObject SystemBackendImpl::pipelineConfig(const QString& strategyId) const
{
    CGenericModelApi* node = findNodeByUuid(strategyId);
    if (!node) return {};
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node);
    if (!adapter) return {};
    return adapter->pipelineConfig();
}

bool SystemBackendImpl::updatePipelineConfig(const QString& strategyId, const QJsonObject& config)
{
    CGenericModelApi* node = findNodeByUuid(strategyId);
    if (!node) return false;
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node);
    if (!adapter) return false;

    adapter->setPipelineConfig(config);
    persistNode(node);
    detectVersionDivergence(strategyId);

    emit pipelineConfigChanged(strategyId, config);
    return true;
}

// ---- Tree Queries ----

static QJsonObject nodeToSummary(CGenericModelApi* node)
{
    QJsonObject obj;
    obj["uuid"] = node->getId().toString(QUuid::WithoutBraces);
    obj["name"] = node->getName();
    obj["type"] = static_cast<int>(node->modelType());
    obj["active"] = node->getActiveStatus();
    return obj;
}

QJsonObject SystemBackendImpl::fullTreeSnapshot() const
{
    QJsonObject root;
    root["type"] = "root";

    std::function<QJsonArray(CGenericModelApi*)> collectChildren;
    collectChildren = [&](CGenericModelApi* parent) -> QJsonArray {
        QJsonArray arr;
        for (auto& child : parent->getModels()) {
            QJsonObject obj = nodeToSummary(child.data());
            QJsonArray childArr = collectChildren(child.data());
            if (!childArr.isEmpty())
                obj["children"] = childArr;
            arr.append(obj);
        }
        return arr;
    };

    if (m_root)
        root["children"] = collectChildren(m_root);
    return root;
}

QJsonArray SystemBackendImpl::listAccounts() const
{
    QJsonArray arr;
    if (!m_root) return arr;
    for (auto& acct : m_root->getModels())
        arr.append(nodeToSummary(acct.data()));
    return arr;
}

QJsonArray SystemBackendImpl::listPortfolios(const QString& accountId) const
{
    QJsonArray arr;
    CGenericModelApi* acct = findNodeByUuid(accountId);
    if (!acct) return arr;
    for (auto& port : acct->getModels())
        arr.append(nodeToSummary(port.data()));
    return arr;
}

QJsonArray SystemBackendImpl::listStrategies(const QString& portfolioId) const
{
    QJsonArray arr;
    CGenericModelApi* port = findNodeByUuid(portfolioId);
    if (!port) return arr;
    for (auto& strat : port->getModels())
        arr.append(nodeToSummary(strat.data()));
    return arr;
}

QJsonObject SystemBackendImpl::nodeInfo(const QString& uuid) const
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node) return {};

    QJsonObject info;
    info["uuid"] = uuid;
    info["name"] = node->getName();
    info["type"] = static_cast<int>(node->modelType());
    info["active"] = node->getActiveStatus();
    info["parameters"] = QJsonObject::fromVariantMap(node->getParameters());
    info["assetList"] = QJsonObject::fromVariantMap(node->assetList());
    return info;
}

// ---- Runtime Service ----

bool SystemBackendImpl::startStrategy(const QString& uuid)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node) return false;
    return node->start();
}

bool SystemBackendImpl::stopStrategy(const QString& uuid)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node) return false;
    return node->stop();
}

bool SystemBackendImpl::connectBroker()
{
    if (m_brokerConnected)
        return true;
    m_brokerConnected = true;
    emit brokerConnectionChanged(true);
    return true;
}

bool SystemBackendImpl::disconnectBroker()
{
    if (!m_brokerConnected)
        return true;
    m_brokerConnected = false;
    emit brokerConnectionChanged(false);
    return true;
}

QJsonObject SystemBackendImpl::runtimeState(const QString& uuid) const
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node) return {};

    QJsonObject state;
    state["uuid"] = uuid;
    state["genericInfo"] = QJsonObject::fromVariantMap(node->genericInfo());
    return state;
}

bool SystemBackendImpl::isBrokerConnected() const
{
    return m_brokerConnected;
}

// ---- Persistence ----

bool SystemBackendImpl::loadFromDb()
{
    QList<ModelNodeRecord> records = m_repo->fetchAll();

    // An empty DB is valid (fresh install or user deleted all nodes).
    // Build a synthetic empty root rather than signalling failure, which
    // would trigger the JSON migration path and a null-broker crash.
    CBasicRoot* newRoot = records.isEmpty()
                          ? new CBasicRoot()
                          : ModelTreeMapper::toRoot(records);
    if (!newRoot) return false;

    delete m_root;
    m_root = newRoot;
    rebuildUuidIndex();
    wireAllRuntimeSignals();

    // --- Orphan repair + m_strategyDefinitionId population ---
    // For each strategy node, ensure a canonical definition + binding exist.
    for (const ModelNodeRecord& rec : records) {
        if (!isStrategyType(static_cast<ModelType>(rec.modelType)))
            continue;

        const QString& nodeUuid = rec.uuid;
        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(nodeUuid);

        if (!binding.isValid()) {
            qCWarning(lcSystemBackend) << "[repair] strategy node" << nodeUuid
                                       << "has no binding — auto-creating catalog entry";
            QJsonObject canonical = extractCanonicalStrategyConfig(rec.config);
            createCatalogEntryAndBinding(nodeUuid, rec.name,
                                         rec.modelType, canonical);
            continue;
        }

        // Check if v3 catalog entry exists; if not, check legacy definition
        DbStrategy catalogEntry = m_repo->fetchStrategyCatalog(binding.strategyDefId);
        if (!catalogEntry.isValid()) {
            DbStrategyDefinition def = m_repo->fetchStrategyDefinition(binding.strategyDefId);
            if (!def.isValid()) {
                qCWarning(lcSystemBackend) << "[repair] no catalog or definition for binding"
                                           << binding.strategyDefId << "— recreating";
                m_repo->removeBindingForNode(nodeUuid);
                QJsonObject canonical = extractCanonicalStrategyConfig(rec.config);
                createCatalogEntryAndBinding(nodeUuid, rec.name,
                                             rec.modelType, canonical);
                continue;
            }
        }

        // Ensure binding has a version_id
        if (binding.versionId.isEmpty()) {
            DbStrategyVersion latest = m_repo->fetchLatestVersion(binding.strategyDefId);
            if (latest.isValid()) {
                m_repo->updateBindingVersion(binding.bindingId, latest.versionId);
                binding.versionId = latest.versionId;
            }
        }

        // Populate the runtime field
        CGenericModelApi* node = findNodeByUuid(nodeUuid);
        if (auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node))
            adapter->setStrategyDefinitionId(binding.strategyDefId);
    }

    // Remove catalog entries that no live binding references (accumulated orphans)
    int orphansRemoved = m_repo->removeOrphanedCatalogEntries();
    if (orphansRemoved > 0)
        qCWarning(lcSystemBackend) << "[repair] removed" << orphansRemoved << "orphaned catalog entries";

    emit treeLoaded();
    return true;
}

bool SystemBackendImpl::importFromJsonFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject rootJson = doc.object();

    auto tempRoot = QSharedPointer<CBasicRoot>::create();
    tempRoot->fromJson(rootJson);

    auto records = ModelTreeMapper::toRecords(tempRoot.data());
    if (records.isEmpty()) return false;

    if (!m_repo->replaceAll(records)) return false;

    // loadFromDb() will run orphan repair for any imported strategy nodes
    // that lack bindings, creating their canonical definitions automatically.
    return loadFromDb();
}

bool SystemBackendImpl::exportToJsonFile(const QString& path)
{
    if (!m_root) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonObject rootJson = m_root->toJson();
    QJsonDocument doc(rootJson);
    file.write(doc.toJson());
    return true;
}

// ---------------------------------------------------------------------------
// Strategy Catalog (v2: families + versions)
// ---------------------------------------------------------------------------

QJsonObject SystemBackendImpl::strategyToJson(const DbStrategy& s)
{
    QJsonObject obj;
    obj["strategyId"]     = s.strategyId;
    obj["name"]           = s.name;
    obj["strategyKind"]   = s.strategyKind;
    obj["lifecycleState"] = s.lifecycleState;
    obj["description"]    = s.description;
    obj["tags"]           = s.tags;
    obj["isArchived"]     = s.isArchived;
    obj["createdAt"]      = s.createdAt;
    obj["updatedAt"]      = s.updatedAt;
    return obj;
}

QJsonObject SystemBackendImpl::versionToJson(const DbStrategyVersion& v)
{
    QJsonObject obj;
    obj["versionId"]     = v.versionId;
    obj["strategyId"]    = v.strategyId;
    obj["versionNumber"] = v.versionNumber;
    obj["configJson"]    = v.configJson;
    obj["notes"]         = v.notes;
    obj["isPublished"]   = v.isPublished;
    obj["createdAt"]     = v.createdAt;
    if (!v.createdFromVersionId.isEmpty())
        obj["createdFromVersionId"] = v.createdFromVersionId;
    return obj;
}

int SystemBackendImpl::publishedVersionCount(const QString& strategyId) const
{
    int count = 0;
    if (!m_repo || strategyId.isEmpty())
        return count;

    for (const auto& v : m_repo->listStrategyVersions(strategyId)) {
        if (v.isPublished)
            ++count;
    }
    return count;
}

StrategyLifecycle::Summary SystemBackendImpl::lifecycleSummaryForStrategy(const DbStrategy& s) const
{
    if (!s.isValid())
        return {};

    return StrategyLifecycle::summarize(
        s.lifecycleState,
        s.isArchived,
        publishedVersionCount(s.strategyId),
        isCatalogStrategyActiveInLive(s.strategyId));
}

QJsonObject SystemBackendImpl::strategyToJsonWithLifecycle(const DbStrategy& s) const
{
    QJsonObject obj = strategyToJson(s);
    const auto summary = lifecycleSummaryForStrategy(s);
    obj[QStringLiteral("lifecycleState")] =
        StrategyLifecycle::manualToStorage(summary.lifecycle);
    obj[QStringLiteral("publishedVersionCount")] = summary.publishedVersionCount;
    obj[QStringLiteral("liveDeploymentActive")] = summary.liveDeploymentActive;
    obj[QStringLiteral("derivedState")] = StrategyLifecycle::stateKey(summary.state);
    obj[QStringLiteral("derivedStateLabel")] = StrategyLifecycle::stateLabel(summary.state);
    obj[QStringLiteral("derivedStateColor")] = StrategyLifecycle::stateColorHex(summary.state);
    obj[QStringLiteral("lifecycleSummary")] = StrategyLifecycle::toJson(summary);
    return obj;
}

QString SystemBackendImpl::createStrategyCatalogEntry(const QString& name, int kind,
                                                       const QJsonObject& initialConfig,
                                                       const QString& description)
{
    Q_UNUSED(initialConfig);

    QString stratId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now     = nowUtcIso();

    DbStrategy strat;
    strat.strategyId     = stratId;
    strat.name           = name;
    strat.strategyKind   = kind;
    strat.lifecycleState = QStringLiteral("draft");
    strat.description    = description.isNull() ? QStringLiteral("") : description;
    strat.isArchived     = false;
    strat.createdAt      = now;
    strat.updatedAt      = now;

    if (!m_repo->createStrategyCatalog(strat))
        return {};

    emit strategyCatalogChanged(stratId);
    return stratId;
}

bool SystemBackendImpl::updateStrategyCatalogMeta(const QString& strategyId,
                                                    const QString& name,
                                                    const QString& description,
                                                    const QString& tags,
                                                    const QString& lifecycleState)
{
    if (!m_repo || strategyId.isEmpty())
        return false;

    DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid()) return false;

    const auto lifecycle = StrategyLifecycle::manualFromStorage(lifecycleState);
    if (lifecycle == StrategyLifecycle::ManualLifecycle::Retired
        && isCatalogStrategyActiveInLive(strategyId)) {
        return false;
    }

    strat.name           = name;
    strat.description    = description;
    strat.tags           = tags;
    strat.lifecycleState = StrategyLifecycle::manualToStorage(lifecycle);
    strat.isArchived     = lifecycle == StrategyLifecycle::ManualLifecycle::Retired;
    strat.updatedAt      = nowUtcIso();

    if (!m_repo->updateStrategyCatalog(strat))
        return false;

    emit strategyCatalogChanged(strategyId);
    return true;
}

bool SystemBackendImpl::setStrategyLifecycle(const QString& strategyId,
                                             StrategyLifecycle::ManualLifecycle lifecycle)
{
    if (!m_repo || strategyId.isEmpty())
        return false;

    DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid())
        return false;

    if (lifecycle == StrategyLifecycle::ManualLifecycle::Retired
        && isCatalogStrategyActiveInLive(strategyId)) {
        return false;
    }

    strat.lifecycleState = StrategyLifecycle::manualToStorage(lifecycle);
    strat.isArchived = lifecycle == StrategyLifecycle::ManualLifecycle::Retired;
    strat.updatedAt = nowUtcIso();

    if (!m_repo->updateStrategyCatalog(strat))
        return false;

    emit strategyCatalogChanged(strategyId);
    return true;
}

QJsonObject SystemBackendImpl::strategyLifecycleSummary(const QString& strategyId) const
{
    if (!m_repo || strategyId.isEmpty())
        return {};

    const DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid())
        return {};

    return StrategyLifecycle::toJson(lifecycleSummaryForStrategy(strat));
}

QJsonObject SystemBackendImpl::strategyCatalogEntry(const QString& strategyId) const
{
    DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid()) return {};
    return strategyToJsonWithLifecycle(strat);
}

QJsonArray SystemBackendImpl::listStrategyCatalog(bool includeArchived) const
{
    QJsonArray arr;
    for (const auto& s : m_repo->listStrategyCatalog(includeArchived))
        arr.append(strategyToJsonWithLifecycle(s));
    return arr;
}

bool SystemBackendImpl::archiveStrategyCatalogEntry(const QString& strategyId)
{
    return setStrategyLifecycle(strategyId, StrategyLifecycle::ManualLifecycle::Retired);
}

bool SystemBackendImpl::deleteStrategyCatalogCascade(const QString& strategyId)
{
    if (!m_repo || strategyId.isEmpty())
        return false;

    DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid())
        return false;

    QList<DbLiveStrategyBinding> bindings = m_repo->listBindingsForDefinition(strategyId);

    QStringList profileRefs;
    profileRefs.reserve(bindings.size() + 1);
    profileRefs.append(strategyId);
    for (const auto& b : bindings) {
        if (!b.modelNodeId.isEmpty())
            profileRefs.append(b.modelNodeId);
    }

    for (const auto& b : bindings) {
        if (!b.modelNodeId.isEmpty())
            removeNode(b.modelNodeId);
    }

    if (!m_repo->deleteStrategyCatalogCascade(strategyId, profileRefs)) {
        qCWarning(lcSystemBackend) << "deleteStrategyCatalogCascade: model store delete failed for"
                                   << strategyId;
        return false;
    }

    if (!deleteBacktestPersistedDataForCatalog(strategyId)) {
        qCWarning(lcSystemBackend) << "deleteStrategyCatalogCascade: backtest DB cleanup failed for"
                                   << strategyId;
    }

    emit strategyCatalogChanged(strategyId);
    return true;
}

bool SystemBackendImpl::isCatalogStrategyActiveInLive(const QString& strategyId) const
{
    if (!m_repo || strategyId.isEmpty())
        return false;
    for (const auto& b : m_repo->listBindingsForDefinition(strategyId)) {
        CGenericModelApi* node = findNodeByUuid(b.modelNodeId);
        if (node && node->getActiveStatus())
            return true;
    }
    return false;
}

QString SystemBackendImpl::createStrategyVersion(const QString& strategyId,
                                                   const QJsonObject& config,
                                                   const QString& notes,
                                                   const QString& fromVersionId)
{
    DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid()) return {};

    QString versionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now       = nowUtcIso();

    DbStrategyVersion ver;
    ver.versionId            = versionId;
    ver.strategyId           = strategyId;
    ver.versionNumber        = m_repo->nextVersionNumber(strategyId);
    ver.configJson           = QString::fromUtf8(
        QJsonDocument(config).toJson(QJsonDocument::Compact));
    ver.notes                = notes.isNull() ? QStringLiteral("") : notes;
    ver.isPublished          = false;
    ver.createdFromVersionId = fromVersionId.isNull() ? QStringLiteral("") : fromVersionId;
    ver.createdAt            = now;

    if (!m_repo->createStrategyVersion(ver))
        return {};

    // Touch strategy updatedAt
    strat.updatedAt = now;
    m_repo->updateStrategyCatalog(strat);

    emit strategyVersionCreated(strategyId, versionId);
    return versionId;
}

QJsonObject SystemBackendImpl::strategyVersionInfo(const QString& versionId) const
{
    DbStrategyVersion ver = m_repo->fetchStrategyVersion(versionId);
    if (!ver.isValid()) return {};
    return versionToJson(ver);
}

QJsonArray SystemBackendImpl::listStrategyVersions(const QString& strategyId) const
{
    QJsonArray arr;
    for (const auto& v : m_repo->listStrategyVersions(strategyId))
        arr.append(versionToJson(v));
    return arr;
}

bool SystemBackendImpl::deleteStrategyVersion(const QString& versionId)
{
    if (!m_repo || versionId.isEmpty())
        return false;

    const DbStrategyVersion version = m_repo->fetchStrategyVersion(versionId);
    if (!version.isValid())
        return false;

    DbStrategy strategy = m_repo->fetchStrategyCatalog(version.strategyId);
    if (!strategy.isValid())
        return false;

    const QList<DbLiveStrategyBinding> bindings =
        m_repo->listBindingsForDefinition(version.strategyId);
    for (const DbLiveStrategyBinding& binding : bindings) {
        if (binding.versionId == versionId)
            return false;
    }

    if (!m_repo->deleteStrategyVersion(versionId))
        return false;

    strategy.updatedAt = nowUtcIso();
    m_repo->updateStrategyCatalog(strategy);
    emit strategyCatalogChanged(version.strategyId);
    return true;
}

bool SystemBackendImpl::publishVersion(const QString& versionId)
{
    if (!m_repo || versionId.isEmpty())
        return false;

    const DbStrategyVersion version = m_repo->fetchStrategyVersion(versionId);
    if (!version.isValid())
        return false;
    if (version.isPublished)
        return true;

    if (!m_repo->setVersionPublished(versionId, true))
        return false;
    if (!m_repo->fetchStrategyVersion(versionId).isPublished)
        return false;

    DbStrategy strategy = m_repo->fetchStrategyCatalog(version.strategyId);
    if (strategy.isValid()) {
        strategy.updatedAt = nowUtcIso();
        m_repo->updateStrategyCatalog(strategy);
    }
    emit strategyCatalogChanged(version.strategyId);
    return true;
}

bool SystemBackendImpl::unpublishVersion(const QString& versionId)
{
    if (!m_repo || versionId.isEmpty())
        return false;

    const DbStrategyVersion version = m_repo->fetchStrategyVersion(versionId);
    if (!version.isValid())
        return false;
    if (!version.isPublished)
        return true;

    const QList<DbLiveStrategyBinding> bindings =
        m_repo->listBindingsForDefinition(version.strategyId);
    for (const DbLiveStrategyBinding& binding : bindings) {
        if (binding.versionId == versionId)
            return false;
    }

    if (!m_repo->setVersionPublished(versionId, false))
        return false;
    if (m_repo->fetchStrategyVersion(versionId).isPublished)
        return false;

    DbStrategy strategy = m_repo->fetchStrategyCatalog(version.strategyId);
    if (strategy.isValid()) {
        strategy.updatedAt = nowUtcIso();
        m_repo->updateStrategyCatalog(strategy);
    }
    emit strategyCatalogChanged(version.strategyId);
    return true;
}

bool SystemBackendImpl::bindLiveNodeToVersion(const QString& nodeId,
                                                const QString& strategyId,
                                                const QString& versionId)
{
    if (!m_repo || nodeId.isEmpty() || strategyId.isEmpty() || versionId.isEmpty())
        return false;

    const DbStrategyVersion version = m_repo->fetchStrategyVersion(versionId);
    if (!version.isValid() || version.strategyId != strategyId || !version.isPublished)
        return false;
    const DbStrategy strategy = m_repo->fetchStrategyCatalog(strategyId);
    if (!strategy.isValid())
        return false;
    const auto lifecycle = StrategyLifecycle::manualFromStorage(strategy.lifecycleState);
    if (strategy.isArchived || lifecycle == StrategyLifecycle::ManualLifecycle::Retired)
        return false;

    m_repo->removeBindingForNode(nodeId);

    QString now = nowUtcIso();
    DbLiveStrategyBinding binding;
    binding.bindingId     = QUuid::createUuid().toString(QUuid::WithoutBraces);
    binding.modelNodeId   = nodeId;
    binding.strategyDefId = strategyId;
    binding.versionId     = versionId;
    binding.createdAt     = now;
    binding.updatedAt     = now;

    if (!m_repo->createLiveBinding(binding))
        return false;

    CGenericModelApi* node = findNodeByUuid(nodeId);
    if (auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(node))
        adapter->setStrategyDefinitionId(strategyId);

    return true;
}

QJsonObject SystemBackendImpl::bindingForNode(const QString& nodeId) const
{
    DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(nodeId);
    if (!binding.isValid()) return {};

    QJsonObject result;
    result["strategyId"]  = binding.strategyDefId;
    result["versionId"]   = binding.versionId;
    result["bindingId"]   = binding.bindingId;

    DbStrategy strat = m_repo->fetchStrategyCatalog(binding.strategyDefId);
    if (strat.isValid())
        result["strategyName"] = strat.name;

    if (!binding.versionId.isEmpty()) {
        DbStrategyVersion ver = m_repo->fetchStrategyVersion(binding.versionId);
        if (ver.isValid()) {
            result["versionNumber"] = ver.versionNumber;
            result["configJson"]    = ver.configJson;
            result["isPublished"]   = ver.isPublished;
        }
    }

    return result;
}

bool SystemBackendImpl::isNodeDivergedFromVersion(const QString& nodeId) const
{
    CGenericModelApi* node = findNodeByUuid(nodeId);
    if (!node) return false;

    DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(nodeId);
    if (!binding.isValid() || binding.versionId.isEmpty())
        return false;

    DbStrategyVersion ver = m_repo->fetchStrategyVersion(binding.versionId);
    if (!ver.isValid())
        return false;

    QJsonObject rawConfig = ModelTreeMapper::nodeConfigJson(node);
    QJsonObject canonical = extractCanonicalStrategyConfig(rawConfig);
    QByteArray  newBytes  = QJsonDocument(canonical).toJson(QJsonDocument::Compact);

    QByteArray existingBytes = QJsonDocument::fromJson(ver.configJson.toUtf8())
                                   .toJson(QJsonDocument::Compact);
    return newBytes != existingBytes;
}

QString SystemBackendImpl::createLiveNodeForExistingCatalog(
    const QString& portfolioId, ModelType type,
    const QString& strategyId, const QString& versionId)
{
    if (!isStrategyType(type)) return {};

    CGenericModelApi* parent = findNodeByUuid(portfolioId);
    if (!parent || parent->modelType() != ModelType::PORTFOLIO) return {};

    DbStrategy strat = m_repo->fetchStrategyCatalog(strategyId);
    if (!strat.isValid()) return {};
    const auto lifecycle = StrategyLifecycle::manualFromStorage(strat.lifecycleState);
    if (strat.isArchived || lifecycle == StrategyLifecycle::ManualLifecycle::Retired)
        return {};

    DbStrategyVersion ver = m_repo->fetchStrategyVersion(versionId);
    if (!ver.isValid() || ver.strategyId != strategyId || !ver.isPublished) return {};

    QUuid id = QUuid::createUuid();
    QString uuid = id.toString(QUuid::WithoutBraces);

    auto model = CStrategyFactory::createNewStrategy(type);
    if (!model) return {};

    model->setId(id);
    model->setName(strat.name);

    ModelNodeRecord rec;
    rec.uuid       = uuid;
    rec.parentUuid = portfolioId;
    rec.modelType  = static_cast<int>(type);
    rec.name       = model->getName();
    rec.config     = ModelTreeMapper::nodeConfigJson(model.data());
    rec.sortOrder  = m_repo->nextSortOrder(portfolioId);
    rec.isActive   = true;
    rec.createdAt  = QDateTime::currentDateTimeUtc();
    rec.updatedAt  = rec.createdAt;

    if (!m_repo->insertNode(rec)) return {};

    model->setParentModel(parent);
    parent->getModels().append(model);
    m_uuidIndex.insert(uuid, model.data());
    wireRuntimeSignals(model.data());

    DbLiveStrategyBinding binding;
    binding.bindingId      = QUuid::createUuid().toString(QUuid::WithoutBraces);
    binding.modelNodeId    = uuid;
    binding.strategyDefId  = strategyId;
    binding.versionId      = versionId;
    binding.createdAt      = nowUtcIso();
    binding.updatedAt      = binding.createdAt;
    m_repo->createLiveBinding(binding);

    QJsonObject pipeCfg = QJsonDocument::fromJson(ver.configJson.toUtf8()).object();
    if (!pipeCfg.isEmpty())
        updatePipelineConfig(uuid, pipeCfg);

    emit nodeCreated(uuid, portfolioId, rec.modelType);
    return uuid;
}

// ---------------------------------------------------------------------------
// Legacy Strategy Catalog (deprecated — delegates to v2 catalog)
// ---------------------------------------------------------------------------

QString SystemBackendImpl::createStrategyDefinition(const QString& name, int kind,
                                                     const QJsonObject& fullConfig)
{
    QString stratId = createStrategyCatalogEntry(name, kind, fullConfig);
    if (stratId.isEmpty()) return {};

    const QString versionId = createStrategyVersion(stratId, fullConfig);
    if (versionId.isEmpty())
        return {};
    publishVersion(versionId);

    // Also write legacy row for backward compat
    DbStrategyDefinition def;
    def.strategyDefId  = stratId;
    def.name           = name;
    def.strategyKind   = kind;
    def.configJson     = QString::fromUtf8(
        QJsonDocument(fullConfig).toJson(QJsonDocument::Compact));
    def.version        = 1;
    def.lifecycleState = QStringLiteral("draft");
    def.isArchived     = false;
    def.createdAt      = nowUtcIso();
    def.updatedAt      = def.createdAt;
    m_repo->createStrategyDefinition(def);

    emit strategyDefinitionChanged(stratId);
    return stratId;
}

bool SystemBackendImpl::updateStrategyDefinition(const QString& defId,
                                                  const QJsonObject& fullConfig)
{
    // Create a new version in the v3 catalog
    DbStrategy strat = m_repo->fetchStrategyCatalog(defId);
    if (!strat.isValid()) {
        // Fall back to legacy-only update
        DbStrategyDefinition def = m_repo->fetchStrategyDefinition(defId);
        if (!def.isValid()) return false;
        QByteArray newBytes = QJsonDocument(fullConfig).toJson(QJsonDocument::Compact);
        QByteArray existing = QJsonDocument::fromJson(def.configJson.toUtf8())
                                  .toJson(QJsonDocument::Compact);
        if (newBytes == existing) return true;
        def.configJson = QString::fromUtf8(newBytes);
        def.version   += 1;
        def.updatedAt  = nowUtcIso();
        if (!m_repo->updateStrategyDefinition(def)) return false;
        emit strategyDefinitionChanged(defId);
        return true;
    }

    // Compare with latest version
    DbStrategyVersion latest = m_repo->fetchLatestVersion(defId);
    QByteArray newBytes = QJsonDocument(fullConfig).toJson(QJsonDocument::Compact);
    if (latest.isValid()) {
        QByteArray existing = QJsonDocument::fromJson(latest.configJson.toUtf8())
                                  .toJson(QJsonDocument::Compact);
        if (newBytes == existing) return true;
    }

    QString versionId = createStrategyVersion(defId, fullConfig);
    if (versionId.isEmpty()) return false;
    publishVersion(versionId);

    emit strategyDefinitionChanged(defId);
    return true;
}

QJsonObject SystemBackendImpl::strategyDefinition(const QString& defId) const
{
    // Try v3 catalog first
    DbStrategy strat = m_repo->fetchStrategyCatalog(defId);
    if (strat.isValid()) {
        QJsonObject obj = strategyToJson(strat);
        // Add legacy-compatible fields
        obj["strategyDefId"] = strat.strategyId;
        DbStrategyVersion latest = m_repo->fetchLatestVersion(defId);
        if (latest.isValid()) {
            obj["configJson"] = latest.configJson;
            obj["version"]    = latest.versionNumber;
        }
        return obj;
    }

    // Fall back to legacy table
    DbStrategyDefinition def = m_repo->fetchStrategyDefinition(defId);
    if (!def.isValid()) return {};
    return definitionToJson(def);
}

QJsonArray SystemBackendImpl::listStrategyDefinitions(bool includeArchived) const
{
    // Prefer v3 catalog
    auto strategies = m_repo->listStrategyCatalog(includeArchived);
    if (!strategies.isEmpty()) {
        QJsonArray arr;
        for (const auto& s : strategies) {
            QJsonObject obj = strategyToJson(s);
            obj["strategyDefId"] = s.strategyId;
            DbStrategyVersion latest = m_repo->fetchLatestVersion(s.strategyId);
            if (latest.isValid()) {
                obj["configJson"] = latest.configJson;
                obj["version"]    = latest.versionNumber;
            }
            arr.append(obj);
        }
        return arr;
    }

    // Fall back to legacy
    QJsonArray arr;
    for (const auto& def : m_repo->listStrategyDefinitions(includeArchived))
        arr.append(definitionToJson(def));
    return arr;
}

bool SystemBackendImpl::archiveStrategyDefinition(const QString& defId)
{
    bool ok = m_repo->archiveStrategyCatalog(defId);
    // Also archive the legacy row if it exists
    m_repo->archiveStrategyDefinition(defId);
    if (!ok) return false;

    emit strategyDefinitionChanged(defId);
    return true;
}

bool SystemBackendImpl::bindLiveNodeToDefinition(const QString& nodeId, const QString& defId)
{
    // Find latest version and delegate to v2
    DbStrategyVersion latest = m_repo->fetchLatestVersion(defId);
    QString versionId = latest.isValid() ? latest.versionId : QString();

    return bindLiveNodeToVersion(nodeId, defId, versionId);
}

QJsonObject SystemBackendImpl::strategyDefinitionForNode(const QString& nodeId) const
{
    QJsonObject binding = bindingForNode(nodeId);
    if (binding.isEmpty()) return {};

    // Build legacy-compatible result
    QString stratId = binding.value("strategyId").toString();
    QJsonObject result;
    result["strategyDefId"] = stratId;
    result["version"]       = binding.value("versionNumber").toInt(1);
    result["name"]          = binding.value("strategyName").toString();
    result["versionId"]     = binding.value("versionId").toString();
    result["configJson"]    = binding.value("configJson").toString();

    return result;
}

// ---------------------------------------------------------------------------
// Backtest Run Profiles
// ---------------------------------------------------------------------------

QString SystemBackendImpl::createBacktestRunProfile(const QString& ownerType,
                                                     const QString& ownerRefId,
                                                     const QString& name,
                                                     const QJsonObject& runConfig)
{
    QString profileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now       = nowUtcIso();

    DbBacktestRunProfile profile;
    profile.profileId     = profileId;
    profile.ownerType     = ownerType;
    profile.ownerRefId    = ownerRefId;
    profile.name          = name;
    profile.runConfigJson = QString::fromUtf8(
        QJsonDocument(runConfig).toJson(QJsonDocument::Compact));
    profile.createdAt     = now;
    profile.updatedAt     = now;

    if (!m_repo->createRunProfile(profile))
        return {};

    return profileId;
}

QJsonArray SystemBackendImpl::listBacktestRunProfiles(const QString& ownerType,
                                                       const QString& ownerRefId) const
{
    QJsonArray arr;
    for (const auto& p : m_repo->listRunProfiles(ownerType, ownerRefId)) {
        QJsonObject obj;
        obj["profileId"]     = p.profileId;
        obj["ownerType"]     = p.ownerType;
        obj["ownerRefId"]    = p.ownerRefId;
        obj["name"]          = p.name;
        obj["runConfigJson"] = p.runConfigJson;
        obj["createdAt"]     = p.createdAt;
        obj["updatedAt"]     = p.updatedAt;
        arr.append(obj);
    }
    return arr;
}
