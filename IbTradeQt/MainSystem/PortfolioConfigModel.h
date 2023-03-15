#ifndef PORTFOLIOCONFIGMODEL_H
#define PORTFOLIOCONFIGMODEL_H

#include "ctreeviewcustommodel.h"
#include "cbasicroot.h"
#include <QList>

class PortfolioConfigModel : public CTreeViewCustomModel
{
    Q_OBJECT
public:
    PortfolioConfigModel(QTreeView *treeView, QObject *parent);

    void setupModelData(TreeItem * rootItem);
    QModelIndex findWorkingNode(QModelIndex index, const QList<quint16> & Ids);
    void addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id);

public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;

private:
    CBasicRoot m_Root;

public slots:
    void slotOnClickAddAccount();
    void slotOnClickAddPortfolio();
    void onClickRemoveNodeButton();


signals:
    void signalUpdateData(const QModelIndex& index);
};

#endif // PORTFOLIOCONFIGMODEL_H
