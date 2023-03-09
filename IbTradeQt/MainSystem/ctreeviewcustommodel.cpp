#include "ctreeviewcustommodel.h"
#include <QFont>
#include <QDebug>
#include <QBrush>

//#define ROW_INDEX_LOG_TEXT 0
//#define ROW_INDEX_LAST_LOG S_LOG_LEVEL_COUNT
//#define ROW_INDEX_SERVER_TEXT S_INDEX_SERVER_TEXT+1
//#define ROW_INDEX_SERVER_ADDRESS S_INDEX_SERVER_ADDRESS+1
//#define ROW_INDEX_SERVER_PORT S_INDEX_SERVER_PORT+1

CTreeViewCustomModel::CTreeViewCustomModel(QObject *parent, CTreeViewDataModel* _dataModel)
    : QAbstractItemModel(parent)
    , m_data(_dataModel)
{
    if(NULL != m_data)
    {
        rootItem = new TreeItem(QList<pItemDataType>());
        m_data->setupModelData(rootItem);
        QObject::connect(this, &CTreeViewCustomModel::dataChanged,
                         m_data, &CTreeViewDataModel::dataChangeCallback, Qt::AutoConnection);
    }
}

CTreeViewCustomModel::~CTreeViewCustomModel()
{
    delete rootItem;
}

int CTreeViewCustomModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() > 0)
        return 0;

    const TreeItem *parentItem = getItem(parent);

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

    TreeItem *item = getItem(index);
    const auto itemData = item->data(index.column());

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::CheckStateRole && !Qt::DecorationRole)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if((itemData.vType & EVT_TEXT) == EVT_TEXT)
            return itemData.value;
        break;
    case Qt::CheckStateRole:
        if(itemData.vType == EVT_CECK_BOX)
            return itemData.value;
        break;
    case Qt::DecorationRole:
        if (0u != (0xF000 & itemData.id))
            return QIcon(":/IBTradeSystem/x_resources/Account_1.png");
    default:
        break;
    }

    return QVariant();

//    int row = index.row();
//    int col = index.column();

//    switch (role) {
//    case Qt::DisplayRole:
//        if(0 == col)
//        {
//            return m_settngsName[row];
//        }
//        else if((1 == col)
//                && (ROW_INDEX_SERVER_PORT == row))
//        {
//            return m_serverPort;
//        }
//        else if((1 == col)
//                && (ROW_INDEX_SERVER_ADDRESS == row))
//        {
//            //return m_serverAddress;
//            return m_severAddress2.toString();
//        }
//        break;

//    case Qt::FontRole:
//        if ((0 == row && 0 == col)
//            || (ROW_INDEX_SERVER_TEXT == row && 0 == col))
//        {
//            QFont boldFont;
//            boldFont.setBold(true);
//            return boldFont;
//        }
//        break;

//    case Qt::BackgroundRole:
//        if ((0 == row) || (ROW_INDEX_SERVER_TEXT == row))
//            return QBrush(QColor(177, 180, 181));

//        break;

//    case Qt::TextAlignmentRole:
//        if (row == 1 && col == 1) //change text alignment only for cell(1,1)
//            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
//        break;

//    case Qt::CheckStateRole:
//        if ((col == 1) && (row >= 1) && (row <= ROW_INDEX_LAST_LOG))
//        {
//            return m_logLevel[row-1] ? Qt::Checked : Qt::Unchecked;

//        }
//        break;
//    }
//    return QVariant();
}

QVariant CTreeViewCustomModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section).value;

    return QVariant();

//    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
//        switch (section) {
//        case 0:
//            return QString("Porperty");
//        case 1:
//            return QString("Value");
//        }
//    }
//    return QVariant();
}

bool CTreeViewCustomModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ((role != Qt::EditRole) && (role != Qt::CheckStateRole))
        return false;

    TreeItem *item = getItem(index);
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

//    bool ret = false;
//    if (!index.isValid() /*|| role != Qt::EditRole*/)
//        return false;

//    if (role == Qt::EditRole) {
//        if(ROW_INDEX_SERVER_ADDRESS == index.row())
//        {
//            QHostAddress address(value.toString());
//            if (QAbstractSocket::IPv4Protocol == address.protocol())
//            {
//               m_severAddress2.setAddress(value.toString());
//            }

//        }
//        else if(ROW_INDEX_SERVER_PORT == index.row())
//        {
//            m_serverPort = value.toUInt();
//        }
//        ret = true;
//    }

//    if (role == Qt::CheckStateRole)
//    {
//        m_logLevel[index.row()-1] = value.toBool();
//        emit signalEditCompleted();
//        ret = true;
//    }

//    return ret;
}

Qt::ItemFlags CTreeViewCustomModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    TreeItem *item = getItem(index);
    const auto itemData = item->data(index.column());
    if(EVT_READ_ONLY == (itemData.vType & EVT_READ_ONLY))
        return QAbstractItemModel::flags(index);

    return Qt::ItemIsEditable | Qt::ItemIsUserCheckable | QAbstractItemModel::flags(index);

//    if(index.column() == 0)
//    {
//       return QStandardItemModel::flags(index);
//    }
//    else if((index.column() == 1) && (index.row() >= 1) && (index.row() <= ROW_INDEX_LAST_LOG))
//    {
//       return Qt::ItemIsUserCheckable | QStandardItemModel::flags(index);
//    }
//    else if((index.column() == 1) && (index.row() > ROW_INDEX_SERVER_TEXT))
//    {
//       return Qt::ItemIsEditable | QStandardItemModel::flags(index);
//    }

    //    return QStandardItemModel::flags(index);
}

QModelIndex CTreeViewCustomModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex CTreeViewCustomModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

CTreeViewDataModel *CTreeViewCustomModel::getDataObject()
{
    return this->m_data;
}

void CTreeViewCustomModel::setupModelData(TreeItem *parent)
{
    QList<TreeItem *> parents;
    parents << parent;

//    QStandardItem *logTextItem = new QStandardItem("_text");
//    logTextItem->setBackground(QBrush(QColor(177, 180, 181)));
//    QFont _font = logTextItem->font();
//    _font.setBold(true);
//    _font.setItalic(true);
//    logTextItem->setFont(_font);
//    logTextItem->setFlags(logTextItem->flags() & (~Qt::ItemFlag::ItemIsEditable));
//    QVariant tmp;
//    tmp.setValue(logTextItem);

//    QList<ptrTextDataType> _columnData;
//    _columnData.reserve(2u);
//    _columnData.append(ptrTextDataType(new stItemData("Test", EVT_TEXT)));
//    _columnData.append(ptrTextDataType(new stItemData(QVariant(), EVT_TEXT)));


//    TreeItem *_parent = parents.last();
//    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
//    for (int column = 0; column < _columnData.size(); ++column)
//        _parent->child(_parent->childCount() - 1)->addData(column, _columnData[column]);

//    QList<ptrTextDataType> columnData;
//    columnData.reserve(2u);
//    columnData.append(ptrTextDataType( new stItemData("Test", EVT_TEXT)));
//    columnData.append(ptrTextDataType(new stItemData(Qt::Checked, EVT_CECK_BOX)));

//    TreeItem *_parent1 = _parent->child(_parent->childCount() - 1);
//    _parent1->insertChildren(_parent1->childCount(), 1, rootItem->columnCount());
//    for (int column = 0; column < columnData.size(); ++column)
//        _parent1->child(_parent1->childCount() - 1)->addData(column, columnData[column]);
//    _parent1->insertChildren(_parent1->childCount(), 1, rootItem->columnCount());
//    for (int column = 0; column < columnData.size(); ++column)
//        _parent1->child(_parent1->childCount() - 1)->addData(column, columnData[column]);
}

//void SettingsModel::setLoggerSettings(const bool _levels[])
//{
//    memcpy(m_logLevel, _levels, S_LOG_LEVEL_COUNT);
//}

//const bool *SettingsModel::getLoggerSettings()
//{
//    return m_logLevel;
//}

//void SettingsModel::setServerSettings(const QString &_addr, const quint16 &_port)
//{
//    m_serverAddress = _addr;
//    m_severAddress2.setAddress(_addr);
//    m_serverPort = _port;
//}

TreeItem *CTreeViewCustomModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}
