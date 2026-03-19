#ifndef SYSTEMTREEMODEL_H
#define SYSTEMTREEMODEL_H

#include "AbstractPipelineTreeModel.h"
#include <QHash>
#include <QTimer>
#include "cmodelstate.h"
#include "cgenericmodelApi.h"

class CBasicRoot;
class ISystemBackend;

class SystemTreeModel : public AbstractPipelineTreeModel
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
    void setBackend(ISystemBackend* backend) { m_backend = backend; }
    CBasicRoot* root() const { return m_root; }

    CGenericModelApi* modelAt(const QModelIndex& index) const;
    CGenericModelApi* parentModelAt(const QModelIndex& index) const;
    CGenericModelApi* parentStrategyOf(const QModelIndex& index) const;

    // QAbstractItemModel interface
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

public slots:
    void rebuildFromRoot();

private:
    static CGenericModelApi* modelFromNode(TreeNode* n);

    void buildSubtree(TreeNode* parentNode, CGenericModelApi* model);
    void connectModelSignals(CGenericModelApi* model, TreeNode* node);
    void disconnectModelSignals(TreeNode* node);

    DisplayState resolveAggregatedStatus(TreeNode* node) const;
    double resolveAggregatedPnL(TreeNode* node) const;
    QString buildModelPath(CGenericModelApi* model) const;

    ISystemBackend* m_backend = nullptr;
    CBasicRoot* m_root = nullptr;
    QTimer m_pollTimer;

    QHash<QString, TreeNode*> m_pathIndex;
};

#endif // SYSTEMTREEMODEL_H
