#include "PortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"

PortfolioConfigModel::PortfolioConfigModel(QObject *parent)
    : m_Item1()
    , m_Item2()
{
}

void PortfolioConfigModel::setupModelData(TreeItem *rootItem)
{
    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Item_1", EVT_TEXT, TVM_UNUSED_ID)));
    rootItem->addData(1, pItemDataType(new stItemData("Item_2", EVT_TEXT, TVM_UNUSED_ID)));

    QList<TreeItem *> parents;
    parents << rootItem;

    TreeItem *parent = parents.last();

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Data1", EVET_RO_TEXT, TVM_UNUSED_ID)));
    parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData("Data2", EVET_RO_TEXT, TVM_UNUSED_ID)));

}

void PortfolioConfigModel::dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &param)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(param);

    if (topLeft.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(topLeft.internalPointer());
        if(item)
        {
            const auto itemData = item->data(topLeft.column());
//            if((S_DATA_ID_LOG_LEVEL_ALL <= itemData.id) && (S_DATA_ID_LOG_LEVEL_DEBUG >= itemData.id))
//            {
//                emit signalEditLogSettingsCompleted();
//            }
//            else if(S_DATA_ID_SERVER_ADDRESS == itemData.id)
//            {
//                signalEditServerAddresCompleted(itemData.value.toString());
//            }
//            else if(S_DATA_ID_SERVER_PORT == itemData.id)
//            {
//                signalEditServerPortCompleted(itemData.value.toInt());
//            }

        }
    }
}
