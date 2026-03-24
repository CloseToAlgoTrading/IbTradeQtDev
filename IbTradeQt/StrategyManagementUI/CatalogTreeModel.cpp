#include "CatalogTreeModel.h"
#include "PipelineConstants.h"
#include "BlockRegistry.h"
#include "ciconhandler.h"

#include <QColor>
#include <QFont>
#include <QJsonArray>

namespace StrategyMgmt {

static QString kindLabel(int k) {
    switch (k) {
    case 0:  return QStringLiteral("pipeline");
    case 1:  return QStringLiteral("classic");
    default: return QStringLiteral("unknown");
    }
}

CatalogTreeModel::CatalogTreeModel(QObject* parent)
    : AbstractPipelineTreeModel(parent)
{
}

CatalogTreeModel::NodeData* CatalogTreeModel::dataOf(TreeNode* n)
{
    return n ? static_cast<NodeData*>(n->modelPtr) : nullptr;
}

CatalogTreeModel::TreeNode* CatalogTreeModel::makeOwnedNode(
    NodeData* nd, TreeNode* parent)
{
    auto* n = new TreeNode;
    n->modelPtr = nd;
    n->deleteModelPtr = [](void* p) { delete static_cast<NodeData*>(p); };
    n->parent = parent;
    if (parent) parent->children.append(n);
    return n;
}

void CatalogTreeModel::populate(const QJsonArray& catalogEntries,
                                 const QMap<QString, int>& versionCounts,
                                 const QMap<QString, QJsonObject>& latestConfigs)
{
    m_entries.clear();
    for (const auto& val : catalogEntries) {
        QJsonObject obj = val.toObject();
        CatalogEntry e;
        e.strategyId     = obj.value(QStringLiteral("strategyId")).toString();
        e.name           = obj.value(QStringLiteral("name")).toString();
        e.strategyKind   = obj.value(QStringLiteral("strategyKind")).toInt();
        e.lifecycleState = obj.value(QStringLiteral("lifecycleState")).toString();
        e.versionCount   = versionCounts.value(e.strategyId, 0);
        e.updatedAt      = obj.value(QStringLiteral("updatedAt")).toString();
        e.pipelineConfig = latestConfigs.value(e.strategyId);
        m_entries.append(e);
    }
    rebuild();
}

void CatalogTreeModel::rebuild()
{
    beginResetModel();

    auto* root = new TreeNode;
    setRootNode(root);

    for (const auto& e : m_entries) {
        auto* nd = new NodeData;
        nd->isStrategy     = true;
        nd->strategyId     = e.strategyId;
        nd->name           = e.name;
        nd->strategyKind   = e.strategyKind;
        nd->lifecycleState = e.lifecycleState;

        auto* sn = makeOwnedNode(nd, root);

        if (!e.pipelineConfig.isEmpty())
            addBlockRoles(sn, e.pipelineConfig);
    }

    endResetModel();
}

// Adds pipeline categories+blocks and stores extra block metadata on each
// leaf node so that the panel can query it via roles.
void CatalogTreeModel::addBlockRoles(TreeNode* strategyNode,
                                     const QJsonObject& pipelineConfig)
{
    addPipelineCategories(strategyNode, pipelineConfig);

    // Walk the newly added children and store JSON key / array-index metadata
    // on block leaf nodes.
    struct CategoryMeta {
        QString displayName;
        QLatin1StringView jsonKey;
        bool isArray;
    };
    static const CategoryMeta cats[] = {
        { QStringLiteral("Selection"),  Pipeline::Key::Selection, true  },
        { QStringLiteral("Alpha"),      Pipeline::Key::Alphas,    true  },
        { QStringLiteral("Risk"),       Pipeline::Key::Risks,     true  },
        { QStringLiteral("Rebalance"),  Pipeline::Key::Rebalance, false },
        { QStringLiteral("Execution"),  Pipeline::Key::Execution, false },
    };

    int catIdx = 0;
    for (auto* catNode : strategyNode->children) {
        if (!catNode->isVirtualCat) continue;
        if (catIdx >= 5) break;
        const auto& cm = cats[catIdx++];

        int blockIdx = 0;
        for (auto* blockNode : catNode->children) {
            if (!blockNode->isVirtual || blockNode->isVirtualCat) continue;
            blockNode->path = QString("%1|%2|%3|%4")
                .arg(cm.displayName)
                .arg(cm.jsonKey)
                .arg(cm.isArray ? 1 : 0)
                .arg(blockIdx);
            ++blockIdx;
        }
    }
}

int CatalogTreeModel::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

QVariant CatalogTreeModel::data(const QModelIndex& index, int role) const
{
    auto* node = nodeFromIndex(index);
    if (!node) return {};

    int col = index.column();

    // Virtual pipeline nodes
    if (node->isVirtual) {
        // Block-level roles
        if (!node->isVirtualCat && !node->path.isEmpty()) {
            QStringList parts = node->path.split('|');
            if (parts.size() == 4) {
                if (role == BlockIsBlockRole)    return true;
                if (role == BlockCategoryRole)   return parts[0];
                if (role == BlockJsonKeyRole)    return parts[1];
                if (role == BlockIsArrayRole)    return parts[2].toInt() != 0;
                if (role == BlockArrayIndexRole) return parts[3].toInt();
            }
        }
        if (role == BlockIsBlockRole)
            return !node->isVirtualCat && !node->path.isEmpty();
        return virtualNodeData(node, col, role);
    }

    auto* nd = dataOf(node);
    if (!nd) return {};

    // ColumnTypeRole
    if (role == StrategyTreeRoles::ColumnTypeRole) {
        if (col == ColName)   return static_cast<int>(ColumnPaintType::NameWithIcon);
        if (col == ColKind)   return static_cast<int>(ColumnPaintType::PlainText);
        if (col == ColStatus) return static_cast<int>(ColumnPaintType::StatusText);
        return static_cast<int>(ColumnPaintType::PlainText);
    }

    // Custom roles
    if (role == StrategyIdRole) return nd->strategyId;
    if (role == IsStrategyRole) return nd->isStrategy;
    if (role == StatusRole)     return nd->lifecycleState;
    if (role == BlockIsBlockRole) return false;

    // Display data
    if (col == ColName) {
        if (role == Qt::DisplayRole) return nd->name;
        if (role == Qt::DecorationRole) {
            static CIconHandler ih;
            return ih.loadIconFromResourceTheme("Strategy");
        }
        if (role == Qt::FontRole) {
            QFont f;
            f.setBold(true);
            return f;
        }
    }

    if (col == ColKind) {
        if (role == Qt::DisplayRole) return kindLabel(nd->strategyKind);
    }

    if (col == ColStatus) {
        if (role == Qt::DisplayRole)    return nd->lifecycleState;
        if (role == Qt::ForegroundRole) {
            if (nd->lifecycleState == QStringLiteral("active"))
                return QColor(76, 175, 80);
            if (nd->lifecycleState == QStringLiteral("archived"))
                return QColor(136, 136, 136);
            return QColor(212, 160, 74);        // draft = amber
        }
    }

    return {};
}

QVariant CatalogTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case ColName:   return QStringLiteral("Name");
    case ColKind:   return QStringLiteral("Kind");
    case ColStatus: return QStringLiteral("Status");
    }
    return {};
}

Qt::ItemFlags CatalogTreeModel::flags(const QModelIndex& index) const
{
    auto* node = nodeFromIndex(index);
    if (!node) return Qt::NoItemFlags;

    if (node->isVirtual) {
        if (node->isVirtualCat) return Qt::ItemIsEnabled;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// Block-level query helpers

bool CatalogTreeModel::isBlockLeaf(const QModelIndex& index) const
{
    return index.data(BlockIsBlockRole).toBool();
}

QString CatalogTreeModel::blockCategory(const QModelIndex& index) const
{
    return index.data(BlockCategoryRole).toString();
}

QString CatalogTreeModel::blockJsonKey(const QModelIndex& index) const
{
    return index.data(BlockJsonKeyRole).toString();
}

bool CatalogTreeModel::blockIsArray(const QModelIndex& index) const
{
    return index.data(BlockIsArrayRole).toBool();
}

int CatalogTreeModel::blockArrayIndex(const QModelIndex& index) const
{
    return index.data(BlockArrayIndexRole).toInt();
}

QString CatalogTreeModel::strategyIdFor(const QModelIndex& index) const
{
    if (!index.isValid()) return {};

    // If it's a strategy node itself
    auto* node = nodeFromIndex(index);
    auto* nd = dataOf(node);
    if (nd && nd->isStrategy) return nd->strategyId;

    // Walk up to find strategy ancestor
    QModelIndex ancestor = index.parent();
    while (ancestor.isValid()) {
        auto* an = nodeFromIndex(ancestor);
        auto* ad = dataOf(an);
        if (ad && ad->isStrategy) return ad->strategyId;
        // Check if it's a non-virtual node
        if (an && !an->isVirtual) break;
        ancestor = ancestor.parent();
    }
    return {};
}

QModelIndex CatalogTreeModel::findStrategyIndex(const QString& strategyId) const
{
    if (strategyId.isEmpty())
        return {};
    const QModelIndex rootParent;
    const int n = rowCount(rootParent);
    for (int r = 0; r < n; ++r) {
        QModelIndex idx = index(r, 0, rootParent);
        if (idx.data(StrategyIdRole).toString() == strategyId)
            return idx;
    }
    return {};
}

} // namespace StrategyMgmt
