#include "BacktestTreeModel.h"
#include "ciconhandler.h"

#include <QColor>
#include <QFont>
#include <QJsonDocument>
#include <QMap>

namespace BacktestUI {

BacktestTreeModel::BacktestTreeModel(QObject* parent)
    : AbstractPipelineTreeModel(parent)
{
}

BacktestTreeModel::NodeData* BacktestTreeModel::dataOf(TreeNode* n)
{
    return n ? static_cast<NodeData*>(n->modelPtr) : nullptr;
}

BacktestTreeModel::TreeNode* BacktestTreeModel::makeOwnedNode(
    NodeData* nd, TreeNode* parent)
{
    auto* n = new TreeNode;
    n->modelPtr = nd;
    n->deleteModelPtr = [](void* p) { delete static_cast<NodeData*>(p); };
    n->parent = parent;
    if (parent) parent->children.append(n);
    return n;
}

void BacktestTreeModel::populate(const QList<StrategyListItem>& items)
{
    m_items = items;
    rebuild();
}

void BacktestTreeModel::populateCatalog(const QList<CatalogVersionItem>& catalogItems)
{
    m_catalogItems = catalogItems;
    rebuild();
}

void BacktestTreeModel::rebuild()
{
    beginResetModel();

    auto* root = new TreeNode;
    setRootNode(root);

    QMap<QString, TreeNode*> accountNodes;
    QMap<QString, TreeNode*> portfolioNodes;

    for (const auto& item : m_items) {
        // Account node
        if (!accountNodes.contains(item.accountName)) {
            auto* nd = new NodeData;
            nd->kind = NodeKind::Account;
            nd->displayName = item.accountName;
            accountNodes[item.accountName] = makeOwnedNode(nd, root);
        }
        TreeNode* acctNode = accountNodes[item.accountName];

        // Portfolio node
        const QString portKey = item.accountName + '/' + item.portfolioName;
        if (!portfolioNodes.contains(portKey)) {
            auto* nd = new NodeData;
            nd->kind = NodeKind::Portfolio;
            nd->displayName = item.portfolioName;
            portfolioNodes[portKey] = makeOwnedNode(nd, acctNode);
        }
        TreeNode* portNode = portfolioNodes[portKey];

        // Strategy leaf
        auto* sd = new NodeData;
        sd->kind           = NodeKind::Strategy;
        sd->strategyId     = item.strategyId;
        sd->defId          = item.strategyDefId;
        sd->catalogVersionId = item.catalogVersionId;
        sd->version        = item.version;
        sd->displayName    = item.name;
        sd->portfolioPath  = item.accountName + " / " + item.portfolioName;
        sd->pipelineConfig = item.pipelineConfig;

        auto* sn = makeOwnedNode(sd, portNode);

        if (!item.pipelineConfig.isEmpty())
            addPipelineCategories(sn, item.pipelineConfig);
    }

    // Catalog section
    QMap<QString, TreeNode*> catalogStratNodes;
    TreeNode* catalogSection = nullptr;

    for (const auto& ci : m_catalogItems) {
        if (!catalogSection) {
            auto* secData = new NodeData;
            secData->kind = NodeKind::CatalogSection;
            secData->displayName = QStringLiteral("Catalog Strategies");
            catalogSection = makeOwnedNode(secData, root);
        }

        if (!catalogStratNodes.contains(ci.strategyId)) {
            auto* csData = new NodeData;
            csData->kind = NodeKind::CatalogStrategy;
            csData->displayName = ci.strategyName;
            csData->strategyId = ci.strategyId;
            catalogStratNodes[ci.strategyId] = makeOwnedNode(csData, catalogSection);
        }

        TreeNode* parentStrat = catalogStratNodes[ci.strategyId];

        auto* vd = new NodeData;
        vd->kind            = NodeKind::CatalogVersion;
        vd->isCatalogEntry  = true;
        vd->strategyId      = ci.strategyId;
        vd->catalogVersionId = ci.versionId;
        vd->version         = ci.versionNumber;
        vd->isPublished     = ci.isPublished;
        vd->displayName     = ci.strategyName + " v" + QString::number(ci.versionNumber);

        auto* vn = makeOwnedNode(vd, parentStrat);

        if (!ci.configJson.isEmpty()) {
            QJsonObject pipeCfg = QJsonDocument::fromJson(ci.configJson.toUtf8()).object();
            if (!pipeCfg.isEmpty()) {
                vd->pipelineConfig = pipeCfg;
                addPipelineCategories(vn, pipeCfg);
            }
        }
    }

    endResetModel();
}

int BacktestTreeModel::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

QVariant BacktestTreeModel::data(const QModelIndex& index, int role) const
{
    auto* node = nodeFromIndex(index);
    if (!node) return {};

    // Virtual pipeline nodes
    if (node->isVirtual)
        return virtualNodeData(node, index.column(), role);

    auto* nd = dataOf(node);
    if (!nd) return {};

    int col = index.column();

    // ColumnTypeRole
    if (role == StrategyTreeRoles::ColumnTypeRole) {
        if (col == ColName)    return static_cast<int>(ColumnPaintType::NameWithIcon);
        if (col == ColVersion) return static_cast<int>(ColumnPaintType::Badge);
        return static_cast<int>(ColumnPaintType::PlainText);
    }

    // Custom roles
    if (role == IsStrategyRole) {
        return nd->kind == NodeKind::Strategy || nd->kind == NodeKind::CatalogVersion;
    }
    if (role == IsCatalogEntryRole) return nd->isCatalogEntry;
    if (role == StrategyIdRole)     return nd->strategyId;
    if (role == DefIdRole)          return nd->defId;
    if (role == VersionRole)        return nd->version;
    if (role == DisplayNameRole)    return nd->displayName;
    if (role == PortfolioPathRole)  return nd->portfolioPath;
    if (role == PipelineJsonRole)   return nd->pipelineConfig;
    if (role == CatalogStratIdRole) return nd->strategyId;
    if (role == CatalogVerIdRole)   return nd->catalogVersionId;

    // Display data
    if (col == ColName) {
        if (role == Qt::DisplayRole) {
            if (nd->kind == NodeKind::CatalogVersion) {
                QString label = QStringLiteral("v%1").arg(nd->version);
                if (nd->isPublished) label += QStringLiteral(" (published)");
                return label;
            }
            return nd->displayName;
        }
        if (role == Qt::DecorationRole) {
            static CIconHandler ih;
            switch (nd->kind) {
            case NodeKind::Account:        return ih.loadIconFromResourceTheme("Account");
            case NodeKind::Portfolio:       return ih.loadIconFromResourceTheme("Portfolio");
            case NodeKind::Strategy:       return ih.loadIconFromResourceTheme("Strategy");
            case NodeKind::CatalogVersion: return ih.loadIconFromResourceTheme("Strategy");
            default: break;
            }
        }
        if (role == Qt::FontRole) {
            if (nd->kind == NodeKind::Account || nd->kind == NodeKind::CatalogSection
                || nd->kind == NodeKind::CatalogStrategy) {
                QFont f;
                f.setBold(true);
                if (nd->kind == NodeKind::CatalogSection) f.setItalic(true);
                return f;
            }
        }
        if (role == Qt::ForegroundRole) {
            switch (nd->kind) {
            case NodeKind::Account:        return QColor(138, 175, 212);    // #8aafd4
            case NodeKind::Portfolio:      return QColor(159, 170, 191);    // #9faabf
            case NodeKind::CatalogSection: return QColor(212, 160, 74);     // #d4a04a
            case NodeKind::CatalogStrategy:return QColor(200, 176, 96);     // #c8b060
            default:                       return QColor(224, 230, 240);    // #e0e6f0
            }
        }
    }

    if (col == ColVersion) {
        if (nd->kind == NodeKind::Strategy && !nd->defId.isEmpty()) {
            if (role == Qt::DisplayRole)
                return QStringLiteral("v%1").arg(nd->version);
            if (role == Qt::ForegroundRole)
                return QColor(74, 144, 217);    // #4a90d9
        }
        if (nd->kind == NodeKind::CatalogVersion) {
            if (role == Qt::DisplayRole)
                return QStringLiteral("v%1").arg(nd->version);
            if (role == Qt::ForegroundRole)
                return QColor(212, 160, 74);    // #d4a04a
        }
    }

    return {};
}

QVariant BacktestTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColName:    return QStringLiteral("Strategy");
    case ColVersion: return QStringLiteral("Ver");
    }
    return {};
}

Qt::ItemFlags BacktestTreeModel::flags(const QModelIndex& index) const
{
    auto* node = nodeFromIndex(index);
    if (!node) return Qt::NoItemFlags;

    if (node->isVirtual) {
        if (node->isVirtualCat) return Qt::ItemIsEnabled;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    auto* nd = dataOf(node);
    if (!nd) return Qt::ItemIsEnabled;

    switch (nd->kind) {
    case NodeKind::Account:
    case NodeKind::Portfolio:
    case NodeKind::CatalogSection:
    case NodeKind::CatalogStrategy:
        return Qt::ItemIsEnabled;
    default:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
}

} // namespace BacktestUI
