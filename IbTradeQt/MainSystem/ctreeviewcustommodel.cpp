#include "ctreeviewcustommodel.h"
#include <QFont>
#include <QDebug>
#include <QBrush>
#include "PortfolioModelDefines.h"


//CTreeViewCustomModel::CTreeViewCustomModel(QObject *parent, CTreeViewDataModel* _dataModel)
//    : QAbstractItemModel(parent)
//    , m_data(_dataModel)
//{
//    if(NULL != m_data)
//    {
//        rootItem = new TreeItem(QList<pItemDataType>());
//        m_data->setupModelData(rootItem);
//        QObject::connect(this, &CTreeViewCustomModel::dataChanged,
//                         m_data, &CTreeViewDataModel::dataChangeCallback, Qt::AutoConnection);
//    }
//}

CTreeViewCustomModel::CTreeViewCustomModel(QTreeView *treeView, QObject *parent)
    : QAbstractItemModel(parent)
    , rootItem(new TreeItem(QList<pItemDataType>()))
    , m_treeView(treeView)
    , m_ih()
{
    QObject::connect(this, &CTreeViewCustomModel::dataChanged,
                     this, &CTreeViewCustomModel::dataChangeCallback, Qt::AutoConnection);

}

CTreeViewCustomModel::~CTreeViewCustomModel()
{
    delete rootItem;
}

int CTreeViewCustomModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() > 0)
        return 0;

    const TreeItem * parentItem = getItem(parent);

    return parentItem ? parentItem->childCount() : 0;
}

int CTreeViewCustomModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return rootItem->columnCount();
}

QVariant CTreeViewCustomModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid())
        return QVariant();

    TreeItem * item = getItem(index);
    const auto itemData = item->data(index.column());

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::CheckStateRole && !Qt::DecorationRole)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if(0u != (itemData.vType & EVT_TEXT ))
            return itemData.value;
        break;

    case Qt::CheckStateRole:
        if(itemData.vType == EVT_CECK_BOX)
            return itemData.value;
        break;
    case Qt::DecorationRole:
        switch ((PM_ITEM_ID_MASK & itemData.id)) {
        case PM_ITEM_ACCOUNTS:
            return m_ih.loadIconFromResourceTheme("Accounts_1");
            break;
        case PM_ITEM_ACCOUNT:
            return m_ih.loadIconFromResourceTheme("Account");
            break;
        case PM_ITEM_PARAMETER:
            return m_ih.loadIconFromResourceTheme("Parameter");
            break;
        case PM_ITEM_PARAMETERS:
            return m_ih.loadIconFromResourceTheme("Parameters");
            break;
        case PM_ITEM_PORTFOLIO:
            return m_ih.loadIconFromResourceTheme("Portfolio");
            break;
        case PM_ITEM_STRATEGY:
            return m_ih.loadIconFromResourceTheme("Strategy");
            break;
        default:
            break;
        }
    default:
        break;
    }

    return QVariant();
}

QVariant CTreeViewCustomModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section).value;

    return QVariant();
}

bool CTreeViewCustomModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ((role != Qt::EditRole) && (role != Qt::CheckStateRole))
        return false;

    TreeItem * item = getItem(index);
    const auto itemData = item->data(index.column());
    bool result = false;
    if(0u == (itemData.vType & EVT_READ_ONLY))
    {
        result = item->setData(index.column(), value);
        if(result){
           switch (itemData.vType & EVT_VALUE_MASK) {
           case EVT_TEXT:
               emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
               break;
           case EVT_CECK_BOX:
               emit dataChanged(index, index, {Qt::DisplayRole, Qt::CheckStateRole});
               break;
           default:
               break;
           }

       }
    }
    return result;

}

Qt::ItemFlags CTreeViewCustomModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    TreeItem * item = getItem(index);
    const auto itemData = item->data(index.column());
    if(EVT_READ_ONLY == (itemData.vType & EVT_READ_ONLY))
        return QAbstractItemModel::flags(index);

    if(EVT_CECK_BOX == (itemData.vType & EVT_CECK_BOX))
        return Qt::ItemIsUserCheckable | QAbstractItemModel::flags(index);

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

QModelIndex CTreeViewCustomModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem * parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem * childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}
//QModelIndex CTreeViewCustomModel::index(int row, int column, const QModelIndex &parent) const
//{
//    // Check for valid column
//    if (column != 0)
//        return QModelIndex();

//    // Get the parent item
//    TreeItem *parentItem = getItem(parent);

//    // Check if parentItem is valid and if the row is within bounds
//    if (!parentItem || row < 0 || row >= parentItem->childCount())
//        return QModelIndex();

//    // Get the child item at the specified row
//    TreeItem *childItem = parentItem->child(row);

//    // Check if the child item is valid
//    if (childItem)
//    {
//        // Create and return a valid QModelIndex
//        return createIndex(row, column, childItem);
//    }

//    // Return an invalid QModelIndex if the child item is not found
//    return QModelIndex();
//}


QModelIndex CTreeViewCustomModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem * childItem = getItem(index);
    TreeItem * parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

//CTreeViewDataModel *CTreeViewCustomModel::getDataObject()
//{
//    return this->m_data;
//}

void CTreeViewCustomModel::setupModelData(TreeItem * parent)
{
//    QList<TreeItem *> parents;
//    parents << parent;
}

TreeItem * CTreeViewCustomModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

void CTreeViewCustomModel::setTreeView(QTreeView *newTreeView)
{
    m_treeView = newTreeView;
}

void CTreeViewCustomModel::slotDataUpdated()
{

}
