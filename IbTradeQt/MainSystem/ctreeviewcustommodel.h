#ifndef CTREEVIEWCUSTOMMODEL_H
#define CTREEVIEWCUSTOMMODEL_H

#include "treeitem.h"
#include "TreeItemDataTypesDef.h"
#include <QStandardItemModel>
#include <QObject>
#include <QAbstractItemModel>
#include <QString>
#include <QHostAddress>
//#include "csettinsmodeldata.h"
#include "ctreeviewdatamodel.h"
#include <QTreeView>


class CTreeViewCustomModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    //CTreeViewCustomModel(QObject *parent, CTreeViewDataModel* _dataModel);
    CTreeViewCustomModel(QTreeView *treeView, QObject *parent);
    ~CTreeViewCustomModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    //CTreeViewDataModel* getDataObject();

    void setTreeView(QTreeView *newTreeView);

protected:
    void setupModelData(TreeItem * parent);
    TreeItem * getItem(const QModelIndex &index) const;

protected:
    //CTreeViewDataModel* m_data;
    TreeItem * rootItem;
    QTreeView *m_treeView;
signals:
    void signalEditCompleted();
    void siganlSataChangeCallback(const pItemDataType pDataItem);

public slots:
    void slotDataUpdated();
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) = 0;
};

#endif // CTREEVIEWCUSTOMMODEL_H
