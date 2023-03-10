#ifndef PORTFOLIOCONFIGMODEL_H
#define PORTFOLIOCONFIGMODEL_H

#include "ctreeviewdatamodel.h"
#include "cbasicroot.h"

class PortfolioConfigModel : public CTreeViewDataModel
{
    Q_OBJECT
public:
    PortfolioConfigModel(QObject *parent);

    void setupModelData(TreeItem *rootItem) final;

public slots:
    void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;

private:
    pItemDataType m_Item1;
    pItemDataType m_Item2;

    CBasicRoot m_Root;


};

#endif // PORTFOLIOCONFIGMODEL_H
