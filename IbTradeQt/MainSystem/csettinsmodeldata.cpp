#include "csettinsmodeldata.h"

CSettinsModelData::CSettinsModelData()
{

}

void CSettinsModelData::setupModelData(TreeItem *rootItem)
{
    QList<TreeItem *> parents;
    parents << rootItem;

    TreeItem *parent = parents.last();
    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Servers", EVET_RO_TEXT)));
    parent->child(parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(QVariant(), EVT_READ_ONLY)));

    TreeItem *_parent = parent->child(parent->childCount() - 1);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    m_pServerAddress = ptrTextDataType(new stItemData("127.0.0.1", EVT_TEXT));
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Address", EVET_RO_TEXT)));
    m_pServerPort = ptrTextDataType(new stItemData("4448", EVT_TEXT));
    _parent->child(_parent->childCount() - 1)->addData(1, m_pServerAddress);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Port", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_pServerPort);

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Log Setting", EVET_RO_TEXT)));
    parent->child(parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(QVariant(), EVT_READ_ONLY)));

    _parent = parent->child(parent->childCount() - 1);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("ALL", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(Qt::Checked, EVT_CECK_BOX)));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Fatal", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX)));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Error", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX)));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Warning", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX)));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Info", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX)));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, ptrTextDataType(new stItemData("Debug", EVET_RO_TEXT)));
    _parent->child(_parent->childCount() - 1)->addData(1, ptrTextDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX)));
}
