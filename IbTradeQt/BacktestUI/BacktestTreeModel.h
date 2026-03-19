#ifndef BACKTESTTREEMODEL_H
#define BACKTESTTREEMODEL_H

#include "AbstractPipelineTreeModel.h"
#include <QJsonObject>

namespace BacktestUI {

struct StrategyListItem {
    QString     strategyId;
    QString     strategyDefId;
    QString     catalogVersionId;
    int         version    = 1;
    QString     name;
    QString     accountName;
    QString     portfolioName;
    QJsonObject pipelineConfig;
};

struct CatalogVersionItem {
    QString strategyId;
    QString strategyName;
    QString versionId;
    int     versionNumber = 1;
    QString configJson;
    bool    isPublished   = false;
};

class BacktestTreeModel : public AbstractPipelineTreeModel
{
    Q_OBJECT
public:
    enum Column {
        ColName = 0,
        ColVersion,
        ColumnCount
    };

    enum Roles {
        StrategyIdRole      = Qt::UserRole + 1,
        DefIdRole,
        VersionRole,
        DisplayNameRole,
        PortfolioPathRole,
        PipelineJsonRole,
        IsStrategyRole,
        IsCatalogEntryRole,
        CatalogStratIdRole,
        CatalogVerIdRole,
    };

    explicit BacktestTreeModel(QObject* parent = nullptr);

    void populate(const QList<StrategyListItem>& items);
    void populateCatalog(const QList<CatalogVersionItem>& catalogItems);

    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    enum class NodeKind { Account, Portfolio, Strategy, CatalogSection, CatalogStrategy, CatalogVersion };

    struct NodeData {
        NodeKind kind = NodeKind::Account;
        QString  strategyId;
        QString  defId;
        QString  catalogVersionId;
        int      version = 0;
        QString  displayName;
        QString  portfolioPath;
        QJsonObject pipelineConfig;
        bool     isCatalogEntry = false;
        bool     isPublished    = false;
    };

    void rebuild();
    static NodeData* dataOf(TreeNode* n);
    static TreeNode* makeOwnedNode(NodeData* nd, TreeNode* parent);

    QList<StrategyListItem>   m_items;
    QList<CatalogVersionItem> m_catalogItems;
};

} // namespace BacktestUI

#endif // BACKTESTTREEMODEL_H
