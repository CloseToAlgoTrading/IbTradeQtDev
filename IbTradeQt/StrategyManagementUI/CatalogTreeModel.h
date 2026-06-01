#ifndef CATALOGTREEMODEL_H
#define CATALOGTREEMODEL_H

#include "AbstractPipelineTreeModel.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>

namespace StrategyMgmt {

class CatalogTreeModel : public AbstractPipelineTreeModel
{
    Q_OBJECT
public:
    enum Column {
        ColName   = 0,
        ColKind,
        ColStatus,
        ColumnCount
    };

    enum Roles {
        StrategyIdRole = Qt::UserRole + 50,
        IsStrategyRole,
        StatusRole,
        // Block-level roles (from PipelineTreeUtils equivalents)
        BlockCategoryRole  = Qt::UserRole + 100,
        BlockJsonKeyRole,
        BlockIsArrayRole,
        BlockArrayIndexRole,
        BlockIsBlockRole,
    };

    explicit CatalogTreeModel(QObject* parent = nullptr);

    void populate(const QJsonArray& catalogEntries,
                  const QMap<QString, int>& versionCounts,
                  const QMap<QString, QJsonObject>& latestConfigs);

    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Block-level query helpers
    bool isBlockLeaf(const QModelIndex& index) const;
    QString blockCategory(const QModelIndex& index) const;
    QString blockJsonKey(const QModelIndex& index) const;
    bool blockIsArray(const QModelIndex& index) const;
    int blockArrayIndex(const QModelIndex& index) const;
    QString strategyIdFor(const QModelIndex& index) const;

    QModelIndex findStrategyIndex(const QString& strategyId) const;

private:
    struct CatalogEntry {
        QString     strategyId;
        QString     name;
        int         strategyKind = 0;
        QString     lifecycleState;
        int         versionCount = 0;
        QString     derivedState;
        QString     derivedStateLabel;
        QString     derivedStateColor;
        QString     updatedAt;
        QJsonObject pipelineConfig;
    };

    struct NodeData {
        bool    isStrategy = false;
        QString strategyId;
        QString name;
        int     strategyKind = 0;
        QString lifecycleState;
        QString derivedState;
        QString derivedStateLabel;
        QString derivedStateColor;
    };

    void rebuild();
    void addBlockRoles(TreeNode* strategyNode, const QJsonObject& pipelineConfig);
    static NodeData* dataOf(TreeNode* n);
    static TreeNode* makeOwnedNode(NodeData* nd, TreeNode* parent);

    QList<CatalogEntry> m_entries;
};

} // namespace StrategyMgmt

#endif // CATALOGTREEMODEL_H
