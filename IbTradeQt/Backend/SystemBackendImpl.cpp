#include "SystemBackendImpl.h"
#include "cbasicroot.h"
#include "cbasicaccount.h"
#include "cbasicportfolio.h"
#include "cstrategyfactory.h"
#include "cpipelinestrategyadapter.h"
#include "ModelTreeMapper.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <functional>

SystemBackendImpl::SystemBackendImpl(ModelTreeRepository* repo, QObject* parent)
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
        qWarning("SystemBackend: failed to persist account %s", qPrintable(uuid));
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
        qWarning("SystemBackend: failed to persist portfolio %s", qPrintable(uuid));
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
        qWarning("SystemBackend: failed to persist strategy %s", qPrintable(uuid));
        return {};
    }

    model->setParentModel(parent);
    parent->getModels().append(model);

    m_uuidIndex.insert(uuid, model.data());
    wireRuntimeSignals(model.data());
    emit nodeCreated(uuid, portfolioId, rec.modelType);
    return uuid;
}

bool SystemBackendImpl::removeNode(const QString& uuid)
{
    CGenericModelApi* node = findNodeByUuid(uuid);
    if (!node || node == m_root) return false;
    CGenericModelApi* parent = node->getParentModel();
    if (!parent) return false;

    if (!m_repo->deleteNode(uuid)) {
        qWarning("SystemBackend: failed to delete node %s", qPrintable(uuid));
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
        qWarning("SystemBackend: failed to rename node %s", qPrintable(uuid));
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
    isArray = false;
    if (category == "Alpha" || category == "alpha" || category == "alphas") {
        isArray = true; return "alphas";
    }
    if (category == "Risk" || category == "risk" || category == "risks") {
        isArray = true; return "risks";
    }
    if (category == "Selection" || category == "selection" || category == "selections") {
        isArray = true; return "selections";
    }
    if (category == "Rebalance" || category == "rebalance") {
        isArray = false; return "rebalance";
    }
    if (category == "Execution" || category == "execution") {
        isArray = false; return "execution";
    }
    return category;
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
        block["blockId"] = blockId;
        block["config"] = defaultConfig;
        arr.append(block);
        config[key] = arr;
    } else {
        QJsonObject block;
        block["blockId"] = blockId;
        block["config"] = defaultConfig;
        config[key] = block;
    }

    adapter->setPipelineConfig(config);

    persistNode(node);

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
    m_brokerConnected = true;
    emit brokerConnectionChanged(true);
    return true;
}

bool SystemBackendImpl::disconnectBroker()
{
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
    if (records.isEmpty()) return false;

    CBasicRoot* newRoot = ModelTreeMapper::toRoot(records);
    if (!newRoot) return false;

    delete m_root;
    m_root = newRoot;
    rebuildUuidIndex();
    wireAllRuntimeSignals();
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
