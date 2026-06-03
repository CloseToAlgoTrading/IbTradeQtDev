#include "SystemTreeModel.h"
#include "ISystemBackend.h"
#include "cbasicroot.h"
#include "cpipelinestrategyadapter.h"
#include "ModelStateUtils.h"
#include "mandatoryFieldKeys.h"
#include "PipelineConstants.h"
#include "ciconhandler.h"
#include <QColor>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>

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

static bool hasPipelineBlocks(const QJsonObject& config)
{
    const auto hasArrayBlock = [&config](QLatin1StringView key) {
        return !config.value(key).toArray().isEmpty();
    };
    const auto hasObjectBlock = [&config](QLatin1StringView key) {
        const QJsonObject obj = config.value(key).toObject();
        return !obj.isEmpty();
    };

    return hasArrayBlock(Pipeline::Key::Selection)
        || hasArrayBlock(Pipeline::Key::Alphas)
        || hasArrayBlock(Pipeline::Key::Risks)
        || hasObjectBlock(Pipeline::Key::Rebalance)
        || hasObjectBlock(Pipeline::Key::Execution);
}

// ---- construction / destruction ----

SystemTreeModel::SystemTreeModel(QObject* parent)
    : AbstractPipelineTreeModel(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
        if (!rootNode()) return;
        if (rootNode()->children.isEmpty()) return;
        emit dataChanged(index(0, ColPnL),
                         index(rowCount() - 1, ColPnL),
                         {Qt::DisplayRole, Qt::ForegroundRole});
    });
    m_pollTimer.start(5000);
}

SystemTreeModel::~SystemTreeModel()
{
    disconnectModelSignals(rootNode());
}

CGenericModelApi* SystemTreeModel::modelFromNode(TreeNode* n)
{
    return n ? static_cast<CGenericModelApi*>(n->modelPtr) : nullptr;
}

// ---- public interface ----

void SystemTreeModel::setRoot(CBasicRoot* root)
{
    m_root = root;
    rebuildFromRoot();
}

CGenericModelApi* SystemTreeModel::modelAt(const QModelIndex& index) const
{
    return modelFromNode(nodeFromIndex(index));
}

CGenericModelApi* SystemTreeModel::parentModelAt(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    if (n && n->parent && n->parent != rootNode())
        return modelFromNode(n->parent);
    return nullptr;
}

CGenericModelApi* SystemTreeModel::parentStrategyOf(const QModelIndex& index) const
{
    auto* n = nodeFromIndex(index);
    if (!n || !n->isVirtual) return nullptr;
    if (!n->isVirtualCat && n->parent && n->parent->parent)
        return modelFromNode(n->parent->parent);
    if (n->isVirtualCat && n->parent)
        return modelFromNode(n->parent);
    return nullptr;
}

// ---- model rebuild ----

void SystemTreeModel::disconnectModelSignals(TreeNode* node)
{
    if (!node) return;
    auto* model = modelFromNode(node);
    if (model && !node->isVirtual) {
        auto* baseModel = dynamic_cast<CBaseModel*>(model);
        if (baseModel)
            QObject::disconnect(baseModel, nullptr, this, nullptr);
    }
    for (auto* child : node->children)
        disconnectModelSignals(child);
}

void SystemTreeModel::rebuildFromRoot()
{
    beginResetModel();
    disconnectModelSignals(rootNode());
    m_pathIndex.clear();

    auto* root = new TreeNode;
    root->modelPtr = m_root;
    root->path = "root";
    setRootNode(root);

    if (m_root) {
        for (auto& account : m_root->getModels())
            buildSubtree(root, account.data());
    }
    endResetModel();
}

void SystemTreeModel::buildSubtree(TreeNode* parentNode, CGenericModelApi* model)
{
    if (!model) return;

    auto* node = new TreeNode;
    node->modelPtr = model;
    node->parent = parentNode;
    node->path = buildModelPath(model);
    parentNode->children.append(node);
    m_pathIndex.insert(node->path, node);

    connectModelSignals(model, node);

    ModelType mt = model->modelType();
    if (mt == ModelType::ACCOUNT || mt == ModelType::PORTFOLIO) {
        for (auto& child : model->getModels())
            buildSubtree(node, child.data());
    }

    if (mt == ModelType::STRATEGY_PIPELINE) {
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(model);
        if (adapter) {
            QJsonObject pipelineConfig = adapter->pipelineConfig();
            if (!hasPipelineBlocks(pipelineConfig) && m_backend) {
                const QString nodeId = model->getId().toString(QUuid::WithoutBraces);
                const QJsonObject binding = m_backend->bindingForNode(nodeId);
                const QString configJson =
                    binding.value(QStringLiteral("configJson")).toString();
                const QJsonObject boundConfig =
                    QJsonDocument::fromJson(configJson.toUtf8()).object();
                if (hasPipelineBlocks(boundConfig))
                    pipelineConfig = boundConfig;
            }
            addPipelineCategories(node, pipelineConfig);
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
        if (node->parent && node->parent != rootNode()) {
            QModelIndex parentIdx = indexForNode(node->parent, ColStatus);
            if (parentIdx.isValid())
                emit dataChanged(parentIdx, parentIdx, {Qt::DisplayRole, Qt::ForegroundRole, DisplayStateRole});
        }
    });

    connect(baseModel, &QObject::destroyed, this, [this, node]() {
        m_pathIndex.remove(node->path);
        node->modelPtr = nullptr;
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

int SystemTreeModel::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

QVariant SystemTreeModel::data(const QModelIndex& index, int role) const
{
    auto* node = nodeFromIndex(index);
    if (!node) return {};

    // Virtual pipeline nodes — delegate to base helper
    if (node->isVirtual) {
        // Supplement: show category label in the Status column for block leaves
        int col = index.column();
        if (!node->isVirtualCat && col == ColStatus) {
            if (role == Qt::DisplayRole) return node->virtualCategory;
            if (role == Qt::ForegroundRole) return QColor(136, 136, 136);
            if (role == StrategyTreeRoles::ColumnTypeRole)
                return static_cast<int>(ColumnPaintType::PlainText);
            return {};
        }
        // ColumnTypeRole for non-name columns
        if (col != 0 && role == StrategyTreeRoles::ColumnTypeRole)
            return static_cast<int>(ColumnPaintType::PlainText);
        return virtualNodeData(node, col, role);
    }

    auto* model = modelFromNode(node);
    if (!model) return {};

    int col = index.column();

    // ColumnTypeRole — tells the delegate how to paint this cell
    if (role == StrategyTreeRoles::ColumnTypeRole) {
        switch (col) {
        case ColName:    return static_cast<int>(ColumnPaintType::NameWithIcon);
        case ColEnabled: return static_cast<int>(ColumnPaintType::Checkbox);
        case ColStatus:  return static_cast<int>(ColumnPaintType::StatusText);
        case ColPnL:     return static_cast<int>(ColumnPaintType::NumericValue);
        }
        return static_cast<int>(ColumnPaintType::PlainText);
    }

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
    auto* model = modelFromNode(node);
    if (!model) return false;

    bool newState = (value.toInt() == Qt::Checked);
    QString uuid = model->getId().toString(QUuid::WithoutBraces);

    if (m_backend)
        m_backend->setNodeActive(uuid, newState);
    else
        model->setActivationState(newState);

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
    if (node && node->isVirtual) {
        if (node->isVirtualCat)
            return Qt::ItemIsEnabled;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.column() == ColEnabled)
        f |= Qt::ItemIsUserCheckable;
    return f;
}

// ---- Aggregation ----

DisplayState SystemTreeModel::resolveAggregatedStatus(TreeNode* node) const
{
    auto* model = modelFromNode(node);
    auto* baseModel = dynamic_cast<CBaseModel*>(model);
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
        auto* childModel = modelFromNode(child);
        auto* childBase = dynamic_cast<CBaseModel*>(childModel);
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
    auto* model = modelFromNode(node);
    if (!model) return 0.0;

    if (node->children.isEmpty()) {
        QVariantMap info = model->genericInfo();
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
