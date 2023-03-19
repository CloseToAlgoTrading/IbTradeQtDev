#ifndef CTREEVIEWDATAMODEL_H
#define CTREEVIEWDATAMODEL_H

#include <QObject>
#include <QModelIndex>
#include <QList>
#include <QAbstractItemModel>
#include "treeitem.h"

class CTreeViewDataModel : public QObject
{
    Q_OBJECT
public:
    virtual void setupModelData(TreeItem * rootItem) = 0;
public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) = 0;
};

#endif // CTREEVIEWDATAMODEL_H
