#include "SystemTreeModel.h"
#include "cbasicroot.h"
#include "cpipelinestrategyadapter.h"
#include "ModelStateUtils.h"
#include "mandatoryFieldKeys.h"
#include "ciconhandler.h"
#include <QColor>
#include <QJsonArray>

static bool isStrategyType(ModelType t)
{
    switch (t) {
    case ModelType::STRATEGY:
    case ModelType::STRATEGY_BASIC_TEST:
    case ModelType::STRATEGY_MA:
    case ModelType::STRATEGY_MOMENTUM:
    case ModelType::STRATEGY_PIPELINE:
        return true;
    default:
        return false;
    }
}

// ---- construction / destruction ----

SystemTreeModel::SystemTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
        if (!m_rootNode) return;
        // Refresh metric columns (PnL) for every visible row
        if (m_rootNode->children.isEmpty()) return;
        emit dataChanged(index(0, ColPnL),
                         index(rowCount() - 1, ColPnL),
                         {Qt::DisplayRole, Qt::ForegroundRole});
    });
    m_pollTimer.start(5000);
}

SystemTreeModel::~SystemTreeModel()
{
    delete m_rootNode;
}

// ---- public interface ----

void SystemTreeModel::setRoot(CBasicRoot* root)
{
    m_root = root;
    rebuildFromRoot();
}

CGenericModelApi* SystemTreeModel::modelAt(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    return n ? n->model : nullptr;
}

CGenericModelApi* SystemTreeModel::parentModelAt(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    if (n && n->parent && n->parent != m_rootNode)
        return n->parent->model;
    return nullptr;
}

bool SystemTreeModel::isVirtualBlock(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    return n && n->isVirtual;
}

QString SystemTreeModel::virtualBlockId(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    return (n && n->isVirtual) ? n->virtualName : QString();
}

QString SystemTreeModel::virtualCategory(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    return (n && n->isVirtual) ? n->virtualCategory : QString();
}

CGenericModelApi* SystemTreeModel::parentStrategyOf(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    if (n && n->isVirtual && n->parent)
        return n->parent->model;
    return nullptr;
}

// ---- model rebuild ----

void SystemTreeModel::rebuildFromRoot()
{
    beginResetModel();
    delete m_rootNode;
    m_rootNode = nullptr;
    m_pathIndex.clear();

    if (m_root) {
        m_rootNode = new TreeNode;
        m_rootNode->model = m_root;
        m_rootNode->path = "root";

        for (auto& account : m_root->getModels()) {
            buildSubtree(m_rootNode, account.data());
        }
    }
    endResetModel();
}

void SystemTreeModel::buildSubtree(TreeNode* parentNode, CGenericModelApi* model)
{
    if (!model) return;

    auto* node = new TreeNode;
    node->model = model;
    node->parent = parentNode;
    node->path = buildModelPath(model);
    parentNode->children.append(node);
    m_pathIndex.insert(node->path, node);

    connectModelSignals(model, node);

    ModelType mt = model->modelType();
    if (mt == ModelType::ACCOUNT || mt == ModelType::PORTFOLIO) {
        for (auto& child : model->getModels()) {
            buildSubtree(node, child.data());
        }
    }

    if (mt == ModelType::STRATEGY_PIPELINE) {
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(model);
        if (adapter) {
            const QJsonObject& cfg = adapter->pipelineConfig();

            auto addFromValue = [&](const QJsonValue& val, const QString& category) {
                if (val.isArray()) {
                    for (const auto& entry : val.toArray()) {
                        QJsonObject obj = entry.toObject();
                        QString blockId = obj.value("blockId").toString();
                        if (blockId.isEmpty()) blockId = obj.value("type").toString();
                        if (blockId.isEmpty()) continue;
                        auto* vnode = new TreeNode;
                        vnode->isVirtual = true;
                        vnode->virtualCategory = category;
                        vnode->virtualName = blockId;
                        vnode->parent = node;
                        vnode->path = node->path + "/" + category + "/" + blockId;
                        node->children.append(vnode);
                    }
                } else if (val.isObject() && !val.toObject().isEmpty()) {
                    QJsonObject obj = val.toObject();
                    QString blockId = obj.value("blockId").toString();
                    if (blockId.isEmpty()) blockId = obj.value("type").toString();
                    if (blockId.isEmpty()) return;
                    auto* vnode = new TreeNode;
                    vnode->isVirtual = true;
                    vnode->virtualCategory = category;
                    vnode->virtualName = blockId;
                    vnode->parent = node;
                    vnode->path = node->path + "/" + category + "/" + blockId;
                    node->children.append(vnode);
                }
            };

            addFromValue(cfg.value("selection"),  "Selection");
            addFromValue(cfg.value("alphas"),     "Alpha");
            addFromValue(cfg.value("risks"),      "Risk");
            addFromValue(cfg.value("rebalance"),  "Rebalance");
            addFromValue(cfg.value("execution"),  "Execution");
        }
    }
}

void SystemTreeModel::connectModelSignals(CGenericModelApi* model, TreeNode* node)
{
    auto* baseModel = dynamic_cast<CBaseModel*>(model);
    if (!baseModel) return;

    connect(baseModel, &CBaseModel::displayStateChanged, this,
            [this, node](DisplayState, DisplayState) {
        QModelIndex idx = indexForNode(node, ColStatus);
        if (idx.isValid())
            emit dataChanged(idx, idx, {Qt::DisplayRole, Qt::ForegroundRole, DisplayStateRole});
        // Also update parent aggregated status
        if (node->parent && node->parent != m_rootNode) {
            QModelIndex parentIdx = indexForNode(node->parent, ColStatus);
            if (parentIdx.isValid())
                emit dataChanged(parentIdx, parentIdx, {Qt::DisplayRole, Qt::ForegroundRole, DisplayStateRole});
        }
    });

    connect(baseModel, &QObject::destroyed, this, [this, node]() {
        m_pathIndex.remove(node->path);
        node->model = nullptr;
        QModelIndex idx = indexForNode(node, 0);
        if (idx.isValid())
            emit dataChanged(idx, idx.sibling(idx.row(), ColumnCount - 1));
    });
}

QString SystemTreeModel::buildModelPath(CGenericModelApi* model) const
{
    if (!model) return {};
    QStringList parts;
    CGenericModelApi* current = model;
    while (current && current != m_root) {
        QString name = current->getName();
        if (name.isEmpty())
            name = current->getId().toString(QUuid::WithoutBraces);
        parts.prepend(name);
        current = current->getParentModel();
    }
    return parts.join('/');
}

// ---- QAbstractItemModel interface ----

QModelIndex SystemTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!m_rootNode || column < 0 || column >= ColumnCount)
        return {};

    TreeNode* parentNode = parent.isValid() ? nodeFromIndex(parent) : m_rootNode;
    if (!parentNode || row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex SystemTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) return {};
    auto* node = nodeFromIndex(child);
    if (!node || !node->parent || node->parent == m_rootNode)
        return {};

    TreeNode* grandparent = node->parent->parent;
    if (!grandparent) return {};

    int row = grandparent->children.indexOf(node->parent);
    return createIndex(row, 0, node->parent);
}

int SystemTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!m_rootNode) return 0;
    TreeNode* node = parent.isValid() ? nodeFromIndex(parent) : m_rootNode;
    return node ? node->children.size() : 0;
}

int SystemTreeModel::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

QVariant SystemTreeModel::data(const QModelIndex& index, int role) const
{
    auto* node = nodeFromIndex(index);
    if (!node) return {};

    if (node->isVirtual) {
        int col = index.column();
        if (col == ColName && role == Qt::DisplayRole)
            return QStringLiteral("[%1] %2").arg(node->virtualCategory, node->virtualName);
        if (col == ColName && role == Qt::DecorationRole) {
            static CIconHandler ih;
            return ih.loadIconFromResourceTheme("Parameter");
        }
        if (col == ColName && role == Qt::ForegroundRole)
            return QColor(136, 170, 210);
        if (col == ColStatus && role == Qt::DisplayRole)
            return node->virtualCategory;
        if (col == ColStatus && role == Qt::ForegroundRole)
            return QColor(136, 136, 136);
        return {};
    }

    if (!node->model) return {};

    CGenericModelApi* model = node->model;
    int col = index.column();

    if (role == ModelPtrRole)
        return QVariant::fromValue(reinterpret_cast<quintptr>(model));
    if (role == ModelTypeRole)
        return static_cast<int>(model->modelType());
    if (role == ModelPathRole)
        return node->path;

    if (role == DisplayStateRole) {
        auto* baseModel = dynamic_cast<CBaseModel*>(model);
        if (baseModel)
            return static_cast<int>(baseModel->resolveDisplayState());
        return static_cast<int>(resolveAggregatedStatus(node));
    }

    // --- Column-specific data ---

    if (col == ColName) {
        if (role == Qt::DisplayRole)
            return model->getName();
        if (role == Qt::DecorationRole) {
            static CIconHandler ih;
            ModelType mt = model->modelType();
            if (mt == ModelType::ACCOUNT)   return ih.loadIconFromResourceTheme("Account");
            if (mt == ModelType::PORTFOLIO)  return ih.loadIconFromResourceTheme("Portfolio");
            if (isStrategyType(mt))         return ih.loadIconFromResourceTheme("Strategy");
        }
    }

    if (col == ColEnabled) {
        if (role == Qt::CheckStateRole)
            return model->getActiveStatus() ? Qt::Checked : Qt::Unchecked;
    }

    if (col == ColStatus) {
        DisplayState ds;
        auto* baseModel = dynamic_cast<CBaseModel*>(model);
        if (baseModel)
            ds = baseModel->resolveDisplayState();
        else
            ds = resolveAggregatedStatus(node);

        auto info = ModelStateUtils::stateDisplay(ds);
        if (role == Qt::DisplayRole)
            return info.indicator + " " + info.label;
        if (role == Qt::ForegroundRole)
            return info.color;
    }

    if (col == ColPnL) {
        if (role == Qt::DisplayRole) {
            double pnl = resolveAggregatedPnL(node);
            return QString::number(pnl, 'f', 2);
        }
        if (role == Qt::ForegroundRole) {
            double pnl = resolveAggregatedPnL(node);
            if (pnl > 0.001)  return QColor(76, 175, 80);
            if (pnl < -0.001) return QColor(229, 57, 53);
            return QColor(136, 136, 136);
        }
        if (role == Qt::TextAlignmentRole)
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    }

    return {};
}

bool SystemTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.column() != ColEnabled || role != Qt::CheckStateRole)
        return false;

    auto* node = nodeFromIndex(index);
    if (!node || !node->model) return false;

    bool newState = (value.toInt() == Qt::Checked);
    node->model->setActivationState(newState);
    emit dataChanged(index, index, {Qt::CheckStateRole});
    return true;
}

QVariant SystemTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColName:    return QStringLiteral("Name");
    case ColEnabled: return QStringLiteral("On");
    case ColStatus:  return QStringLiteral("Status");
    case ColPnL:     return QStringLiteral("PnL");
    }
    return {};
}

Qt::ItemFlags SystemTreeModel::flags(const QModelIndex& index) const
{
    auto* node = nodeFromIndex(index);
    if (node && node->isVirtual)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.column() == ColEnabled)
        f |= Qt::ItemIsUserCheckable;
    return f;
}

// ---- helpers ----

SystemTreeModel::TreeNode* SystemTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) return nullptr;
    return static_cast<TreeNode*>(index.internalPointer());
}

QModelIndex SystemTreeModel::indexForNode(TreeNode* node, int column) const
{
    if (!node || node == m_rootNode) return {};
    TreeNode* parent = node->parent;
    if (!parent) return {};
    int row = parent->children.indexOf(node);
    if (row < 0) return {};
    return createIndex(row, column, node);
}

// ---- D9: Aggregation ----

DisplayState SystemTreeModel::resolveAggregatedStatus(TreeNode* node) const
{
    auto* baseModel = dynamic_cast<CBaseModel*>(node->model);
    if (baseModel && node->children.isEmpty())
        return baseModel->resolveDisplayState();

    if (baseModel) {
        DisplayState own = baseModel->resolveDisplayState();
        if (own == DisplayState::Disconnected || own == DisplayState::Error)
            return own;
    }

    bool anyError = false, anyDisconnected = false, anyWarning = false;
    bool anyRunning = false, anyReady = false, anyPaused = false;
    bool allDisabled = true, allIdle = true;

    for (auto* child : node->children) {
        DisplayState cs;
        auto* childBase = dynamic_cast<CBaseModel*>(child->model);
        if (childBase)
            cs = childBase->resolveDisplayState();
        else
            cs = resolveAggregatedStatus(child);

        if (cs != DisplayState::Disabled) allDisabled = false;
        if (cs != DisplayState::Idle)     allIdle = false;

        switch (cs) {
        case DisplayState::Error:        anyError = true; break;
        case DisplayState::Disconnected: anyDisconnected = true; break;
        case DisplayState::Warning:      anyWarning = true; break;
        case DisplayState::Running:      anyRunning = true; break;
        case DisplayState::Ready:        anyReady = true; break;
        case DisplayState::Paused:       anyPaused = true; break;
        default: break;
        }
    }

    if (anyError)        return DisplayState::Error;
    if (anyDisconnected) return DisplayState::Disconnected;
    if (anyWarning)      return DisplayState::Warning;
    if (allDisabled)     return DisplayState::Disabled;
    if (allIdle)         return DisplayState::Idle;
    if (anyRunning)      return DisplayState::Running;
    if (anyReady || anyPaused) return DisplayState::Ready;

    return DisplayState::Idle;
}

double SystemTreeModel::resolveAggregatedPnL(TreeNode* node) const
{
    if (!node->model) return 0.0;

    if (node->children.isEmpty()) {
        QVariantMap info = node->model->genericInfo();
        if (info.contains(MandatoryInfo::Strategy::DailyPnL))
            return info[MandatoryInfo::Strategy::DailyPnL].toDouble();
        if (info.contains(MandatoryInfo::Portfolio::DailyPnL))
            return info[MandatoryInfo::Portfolio::DailyPnL].toDouble();
        if (info.contains("pnl"))
            return info["pnl"].toDouble();
        return 0.0;
    }

    double total = 0.0;
    for (auto* child : node->children)
        total += resolveAggregatedPnL(child);
    return total;
}
