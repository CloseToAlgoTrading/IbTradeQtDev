#ifndef SYSTEMTREEMODEL_H
#define SYSTEMTREEMODEL_H

#include <QAbstractItemModel>
#include <QHash>
#include <QTimer>
#include "cmodelstate.h"
#include "cgenericmodelApi.h"

class CBasicRoot;

class SystemTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Column {
        ColName    = 0,
        ColEnabled = 1,
        ColStatus  = 2,
        ColPnL     = 3,
        ColumnCount
    };

    enum Roles {
        ModelPtrRole   = Qt::UserRole + 1,
        ModelTypeRole,
        DisplayStateRole,
        ModelPathRole
    };

    explicit SystemTreeModel(QObject* parent = nullptr);
    ~SystemTreeModel() override;

    void setRoot(CBasicRoot* root);
    CBasicRoot* root() const { return m_root; }

    CGenericModelApi* modelAt(const QModelIndex& index) const;
    CGenericModelApi* parentModelAt(const QModelIndex& index) const;

    bool isVirtualBlock(const QModelIndex& index) const;
    QString virtualBlockId(const QModelIndex& index) const;
    QString virtualCategory(const QModelIndex& index) const;
    CGenericModelApi* parentStrategyOf(const QModelIndex& index) const;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

public slots:
    void rebuildFromRoot();

private:
    struct TreeNode {
        CGenericModelApi* model = nullptr;
        TreeNode* parent = nullptr;
        QList<TreeNode*> children;
        QString path;

        QString virtualName;
        QString virtualCategory;
        bool isVirtual = false;

        ~TreeNode() { qDeleteAll(children); }
    };

    TreeNode* nodeFromIndex(const QModelIndex& index) const;
    QModelIndex indexForNode(TreeNode* node, int column = 0) const;
    void buildSubtree(TreeNode* parentNode, CGenericModelApi* model);
    void connectModelSignals(CGenericModelApi* model, TreeNode* node);

    DisplayState resolveAggregatedStatus(TreeNode* node) const;
    double resolveAggregatedPnL(TreeNode* node) const;
    QString buildModelPath(CGenericModelApi* model) const;

    CBasicRoot* m_root = nullptr;
    TreeNode* m_rootNode = nullptr;
    QTimer m_pollTimer;

    QHash<QString, TreeNode*> m_pathIndex;
};

#endif // SYSTEMTREEMODEL_H
