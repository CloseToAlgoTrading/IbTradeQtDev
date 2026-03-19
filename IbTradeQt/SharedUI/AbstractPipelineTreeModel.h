#ifndef ABSTRACTPIPELINETREEMODEL_H
#define ABSTRACTPIPELINETREEMODEL_H

#include <QAbstractItemModel>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "StrategyTreeDelegate.h"          // ColumnPaintType, StrategyTreeRoles

// Abstract base for all pipeline-aware tree models.
//
// Provides the common TreeNode structure, QAbstractItemModel boilerplate
// (index/parent/rowCount), and pipeline category+block population helpers.
// Subclasses implement rebuildTree(), data(), columnCount(), headerData().

class AbstractPipelineTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    // Custom roles shared by every subclass
    enum SharedRole {
        VirtualBlockIdRole  = Qt::UserRole + 800,
        VirtualCategoryRole = Qt::UserRole + 801,
        IsVirtualBlockRole  = Qt::UserRole + 802,
        IsVirtualCatRole    = Qt::UserRole + 803,
    };

    explicit AbstractPipelineTreeModel(QObject* parent = nullptr);
    ~AbstractPipelineTreeModel() override;

    // QAbstractItemModel interface (common boilerplate)
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;

    // Virtual-node queries
    bool isVirtualBlock(const QModelIndex& idx) const;
    bool isVirtualCategory(const QModelIndex& idx) const;
    QString virtualBlockId(const QModelIndex& idx) const;
    QString virtualCategory(const QModelIndex& idx) const;

protected:
    struct TreeNode {
        TreeNode* parent = nullptr;
        QList<TreeNode*> children;

        // Pipeline virtual nodes (category headers + block leaves)
        bool    isVirtual    = false;
        bool    isVirtualCat = false;
        QString virtualName;
        QString virtualCategory;

        // Generic fields used by subclasses
        void*   modelPtr = nullptr;     // typed via subclass (e.g. CGenericModelApi*)
        void  (*deleteModelPtr)(void*) = nullptr; // if non-null, frees modelPtr
        QString path;

        ~TreeNode() {
            qDeleteAll(children);
            if (deleteModelPtr && modelPtr)
                deleteModelPtr(modelPtr);
        }
    };

    // Tree management
    TreeNode* rootNode() const { return m_rootNode; }
    void setRootNode(TreeNode* node);
    TreeNode* nodeFromIndex(const QModelIndex& idx) const;
    QModelIndex indexForNode(TreeNode* node, int column = 0) const;

    // Pipeline helpers: add Selection/Alpha/Risk/Rebalance/Execution
    // sub-tree under a strategy node.
    void addPipelineCategories(TreeNode* strategyNode,
                               const QJsonObject& pipelineConfig);

    // Data helpers for virtual nodes (call from subclass data())
    QVariant virtualNodeData(TreeNode* node, int column, int role) const;

private:
    TreeNode* m_rootNode = nullptr;
};

#endif // ABSTRACTPIPELINETREEMODEL_H
