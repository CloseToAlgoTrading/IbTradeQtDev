#ifndef CPORTFOLIOCONFIGMODEL_H
#define CPORTFOLIOCONFIGMODEL_H

#include "ctreeviewcustommodel.h"
#include "cbasicroot.h"
#include <QList>

class CPortfolioConfigModel : public CTreeViewCustomModel
{
    Q_OBJECT
public:
    CPortfolioConfigModel(QTreeView *treeView, QObject *parent);

    void setupModelData(TreeItem * rootItem);
    QModelIndex findWorkingNode(QModelIndex index, const QList<quint16> & Ids);
    void addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id);

    void addModel(const QModelIndex& index, const QList<quint16>& ids, ptrGenericModelType model, quint16 itemType);
    void removeModel(QModelIndex index);

public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;

private:
    CBasicRoot m_Root;

public slots:
    void slotOnClickAddAccount();
    void slotOnClickAddPortfolio();
    void slotOnClickAddStrategy();
    void onClickRemoveNodeButton();


signals:
    void signalUpdateData(const QModelIndex& index);
};

#endif // CPORTFOLIOCONFIGMODEL_H
