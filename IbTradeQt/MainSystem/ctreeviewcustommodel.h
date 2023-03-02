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



class CTreeViewCustomModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    //SettingsModel(QObject *parent = nullptr);
    CTreeViewCustomModel(QObject *parent, CTreeViewDataModel* _dataModel);
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

    CTreeViewDataModel* getDataObject();

private:
    void setupModelData(TreeItem *parent);
    TreeItem *getItem(const QModelIndex &index) const;

private:
    CTreeViewDataModel* m_data;
    TreeItem *rootItem;
signals:
    void signalEditCompleted();
    void siganlSataChangeCallback(const pItemDataType pDataItem);

public slots:
};

#endif // CTREEVIEWCUSTOMMODEL_H
