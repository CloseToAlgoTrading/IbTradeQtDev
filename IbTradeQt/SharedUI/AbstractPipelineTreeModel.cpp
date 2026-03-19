#include "AbstractPipelineTreeModel.h"
#include "PipelineConstants.h"
#include "BlockRegistry.h"
#include "ciconhandler.h"

#include <QColor>
#include <QFont>
#include <QJsonArray>

// Pipeline category definitions (same order used everywhere)
struct CategoryDef {
    QString displayName;
    QLatin1StringView jsonKey;
    bool isArray;
};

static const CategoryDef kCategories[] = {
    { QStringLiteral("Selection"),  Pipeline::Key::Selection, true  },
    { QStringLiteral("Alpha"),      Pipeline::Key::Alphas,    true  },
    { QStringLiteral("Risk"),       Pipeline::Key::Risks,     true  },
    { QStringLiteral("Rebalance"),  Pipeline::Key::Rebalance, false },
    { QStringLiteral("Execution"),  Pipeline::Key::Execution, false },
};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AbstractPipelineTreeModel::AbstractPipelineTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

AbstractPipelineTreeModel::~AbstractPipelineTreeModel()
{
    delete m_rootNode;
}

void AbstractPipelineTreeModel::setRootNode(TreeNode* node)
{
    delete m_rootNode;
    m_rootNode = node;
}

// ---------------------------------------------------------------------------
// QAbstractItemModel boilerplate
// ---------------------------------------------------------------------------

QModelIndex AbstractPipelineTreeModel::index(int row, int column,
                                             const QModelIndex& parent) const
{
    if (!m_rootNode || column < 0 || column >= columnCount(parent))
        return {};

    TreeNode* parentNode = parent.isValid() ? nodeFromIndex(parent) : m_rootNode;
    if (!parentNode || row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex AbstractPipelineTreeModel::parent(const QModelIndex& child) const
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

int AbstractPipelineTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!m_rootNode) return 0;
    TreeNode* node = parent.isValid() ? nodeFromIndex(parent) : m_rootNode;
    return node ? node->children.size() : 0;
}

AbstractPipelineTreeModel::TreeNode*
AbstractPipelineTreeModel::nodeFromIndex(const QModelIndex& idx) const
{
    if (!idx.isValid()) return nullptr;
    return static_cast<TreeNode*>(idx.internalPointer());
}

QModelIndex AbstractPipelineTreeModel::indexForNode(TreeNode* node,
                                                    int column) const
{
    if (!node || node == m_rootNode) return {};
    TreeNode* parent = node->parent;
    if (!parent) return {};
    int row = parent->children.indexOf(node);
    if (row < 0) return {};
    return createIndex(row, column, node);
}

// ---------------------------------------------------------------------------
// Virtual-node queries
// ---------------------------------------------------------------------------

bool AbstractPipelineTreeModel::isVirtualBlock(const QModelIndex& idx) const
{
    auto* n = nodeFromIndex(idx);
    return n && n->isVirtual && !n->isVirtualCat;
}

bool AbstractPipelineTreeModel::isVirtualCategory(const QModelIndex& idx) const
{
    auto* n = nodeFromIndex(idx);
    return n && n->isVirtual && n->isVirtualCat;
}

QString AbstractPipelineTreeModel::virtualBlockId(const QModelIndex& idx) const
{
    auto* n = nodeFromIndex(idx);
    return (n && n->isVirtual && !n->isVirtualCat) ? n->virtualName : QString();
}

QString AbstractPipelineTreeModel::virtualCategory(const QModelIndex& idx) const
{
    auto* n = nodeFromIndex(idx);
    if (!n || !n->isVirtual) return {};
    return n->virtualCategory;
}

// ---------------------------------------------------------------------------
// Pipeline category/block population
// ---------------------------------------------------------------------------

void AbstractPipelineTreeModel::addPipelineCategories(
    TreeNode* strategyNode, const QJsonObject& pipelineConfig)
{
    for (const auto& cat : kCategories) {
        QJsonValue val = pipelineConfig.value(cat.jsonKey);

        auto* catNode = new TreeNode;
        catNode->isVirtual    = true;
        catNode->isVirtualCat = true;
        catNode->virtualCategory = cat.displayName;
        catNode->virtualName     = cat.displayName;
        catNode->parent = strategyNode;
        strategyNode->children.append(catNode);

        auto addBlock = [&](const QJsonObject& obj) {
            QString blockId = obj.value(Pipeline::Key::BlockId).toString();
            if (blockId.isEmpty()) blockId = obj.value("type").toString();
            if (blockId.isEmpty()) return;

            auto* blockNode = new TreeNode;
            blockNode->isVirtual = true;
            blockNode->virtualCategory = cat.displayName;
            blockNode->virtualName = blockId;
            blockNode->parent = catNode;
            catNode->children.append(blockNode);
        };

        if (cat.isArray) {
            for (const auto& entry : val.toArray())
                addBlock(entry.toObject());
        } else if (val.isObject() && !val.toObject().isEmpty()) {
            addBlock(val.toObject());
        }
    }
}

// ---------------------------------------------------------------------------
// Data helpers for virtual nodes
// ---------------------------------------------------------------------------

QVariant AbstractPipelineTreeModel::virtualNodeData(TreeNode* node,
                                                    int column, int role) const
{
    if (!node || !node->isVirtual) return {};

    if (role == IsVirtualBlockRole)
        return !node->isVirtualCat;
    if (role == IsVirtualCatRole)
        return node->isVirtualCat;
    if (role == VirtualBlockIdRole)
        return node->isVirtualCat ? QVariant() : QVariant(node->virtualName);
    if (role == VirtualCategoryRole)
        return node->virtualCategory;

    if (column != 0) {
        if (role == StrategyTreeRoles::ColumnTypeRole)
            return static_cast<int>(ColumnPaintType::PlainText);
        return {};
    }

    if (node->isVirtualCat) {
        if (role == Qt::DisplayRole)
            return node->virtualCategory;
        if (role == Qt::FontRole) {
            QFont f;
            f.setBold(true);
            return f;
        }
        if (role == Qt::ForegroundRole)
            return QColor(160, 180, 210);       // muted blue for category
        if (role == StrategyTreeRoles::ColumnTypeRole)
            return static_cast<int>(ColumnPaintType::NameWithIcon);
        return {};
    }

    // Block leaf
    if (role == Qt::DisplayRole) {
        auto desc = Pipeline::BlockRegistry::instance().descriptor(node->virtualName);
        return desc ? desc.value().name : node->virtualName;
    }
    if (role == Qt::DecorationRole) {
        static CIconHandler ih;
        return ih.loadIconFromResourceTheme("Parameter");
    }
    if (role == Qt::ForegroundRole)
        return QColor(136, 170, 210);           // block leaf color
    if (role == StrategyTreeRoles::ColumnTypeRole)
        return static_cast<int>(ColumnPaintType::NameWithIcon);

    return {};
}
