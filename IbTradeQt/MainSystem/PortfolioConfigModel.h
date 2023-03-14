#ifndef PORTFOLIOCONFIGMODEL_H
#define PORTFOLIOCONFIGMODEL_H

#include "ctreeviewcustommodel.h"
#include "cbasicroot.h"

class PortfolioConfigModel : public CTreeViewCustomModel
{
    Q_OBJECT
public:
    PortfolioConfigModel(QTreeView *treeView, QObject *parent);

    void setupModelData(TreeItem * rootItem);

public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;

private:
    CBasicRoot m_Root;

public slots:
    void onClickAddNodeButton();
    void onClickRemoveNodeButton();

signals:
    void signalUpdateData(const QModelIndex& index);
};

#endif // PORTFOLIOCONFIGMODEL_H
