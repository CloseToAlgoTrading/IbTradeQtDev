#include "csettinsmodeldata.h"
#include "MyLogger.h"
#include "NHelper.h"

CSettinsModelData::CSettinsModelData(QTreeView *treeView, QObject *parent)
    : CTreeViewCustomModel(treeView, parent),
    m_pServerAddress(),
    m_pServerPort(),
    m_plogLevel()
{
    Q_UNUSED(parent);
    setupModelData(rootItem);

    QObject::connect(this, &CSettinsModelData::signalEditLogSettingsCompleted, this, &CSettinsModelData::slotEditSettingLogSettingsComlited, Qt::QueuedConnection);
    QObject::connect(this, &CSettinsModelData::signalEditServerPortCompleted, NHelper::writeServerPort);
    QObject::connect(this, &CSettinsModelData::signalEditServerAddresCompleted, NHelper::writeServerAddress);

    NHelper::initSettings();

    createSettingsView();

    MyLogger::setDebugLevelMask(NHelper::getLoggerMask());

}

void CSettinsModelData::setupModelData(TreeItem * rootItem)
{

    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Parameter", EVT_TEXT, S_DATA_ID_UNSET)));
    rootItem->addData(1, pItemDataType(new stItemData("Value", EVT_TEXT, S_DATA_ID_UNSET)));

    QList<TreeItem *> parents;
    parents << rootItem;

    TreeItem * parent = parents.last();

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Servers", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QVariant(), EVT_READ_ONLY, S_DATA_ID_UNSET)));

    TreeItem * _parent = parent->child(parent->childCount() - 1);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    m_pServerAddress = pItemDataType(new stItemData("127.0.0.1", EVT_TEXT, S_DATA_ID_SERVER_ADDRESS));
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Address", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    m_pServerPort = pItemDataType(new stItemData("4448", EVT_TEXT, S_DATA_ID_SERVER_PORT));
    _parent->child(_parent->childCount() - 1)->addData(1, m_pServerAddress);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Port", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_pServerPort);

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Log Setting", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QVariant(), EVT_READ_ONLY, S_DATA_ID_UNSET)));

    m_plogLevel.reserve(S_LOG_LEVEL_COUNT);
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Checked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_ALL)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_FATAL)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_ERROR)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_WARNING)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_INFO)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_DEBUG)));


    _parent = parent->child(parent->childCount() - 1);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("ALL", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_ALL));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Fatal", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_FATAL));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Error", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_ERROR));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Warning", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_WARNING));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Info", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_INFO));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Debug", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_DEBUG));
}

void CSettinsModelData::dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(param);

    if (topLeft.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(topLeft.internalPointer());
        if(item)
        {
            const auto itemData = item->data(topLeft.column());
            if((S_DATA_ID_LOG_LEVEL_ALL <= itemData.id) && (S_DATA_ID_LOG_LEVEL_DEBUG >= itemData.id))
            {
                emit signalEditLogSettingsCompleted();
            }
            else if(S_DATA_ID_SERVER_ADDRESS == itemData.id)
            {
                signalEditServerAddresCompleted(itemData.value.toString());
            }
            else if(S_DATA_ID_SERVER_PORT == itemData.id)
            {
                signalEditServerPortCompleted(itemData.value.toInt());
            }

        }
    }
}

void CSettinsModelData::slotEditSettingLogSettingsComlited()
{
    quint8 mask = CSettinsModelData::getMaskFromLoggerSettings(this->getLoggerSettings());//CSettinsModelData::getMaskFromLoggerSettings(m_SettingsModel.getLoggerSettings());
    NHelper::writeLoggerMask(mask);
    MyLogger::setDebugLevelMask(mask);
}

void CSettinsModelData::setLoggerSettings(const QVector<bool> _levels)
{
    if(S_LOG_LEVEL_COUNT == _levels.length())
    {
        for (quint8 i = 0; i < _levels.length(); ++i)
        {
            m_plogLevel.at(i)->value = _levels.at(i) ? Qt::Checked : Qt::Unchecked;
        }
    }
}

const QVector<bool> CSettinsModelData::getLoggerSettings()
{
    QVector<bool> _levels;
    _levels.reserve(m_plogLevel.length());
    for (const auto& level : m_plogLevel)
    {
        _levels.append(level->value == Qt::Checked ? true : false);
    }

    return _levels;
}

void CSettinsModelData::setServerSettings(const QString &_addr, const quint16 &_port)
{
    m_pServerAddress->value = _addr;
    m_pServerPort->value = _port;
//    m_pModel->item(INDEX_SEVER_ADDRESS, 1)->setData(_addr, Qt::DisplayRole);
//    m_pModel->item(INDEX_SEVER_PORT, 1)->setData(_port, Qt::DisplayRole);
}


quint8 CSettinsModelData::getMaskFromLoggerSettings(const QVector<bool> &_levels)
{
    quint8 mask = static_cast<quint8>(MyLogger::LL_NONE);

    if(true == _levels.at(S_INDEX_LOG_LEVEL_ALL))
    {
        mask = MyLogger::LL_ALL;
    }
    else {
        if(true == _levels.at(S_INDEX_LOG_LEVEL_INFO))
        {
            mask |= MyLogger::LL_INFO;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_DEBUG))
        {
            mask |= MyLogger::LL_DEBUG;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_ERROR))
        {
            mask |= MyLogger::LL_ERROR;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_FATAL))
        {
            mask |= MyLogger::LL_FATAL;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_WARNING))
        {
            mask |= MyLogger::LL_WARNING;
        }
    }
    return mask;
}

void CSettinsModelData::updateLoggerSettingsArray(quint8 _mask, QVector<bool> & _levels)
{
    if (_mask & MyLogger::LL_DEBUG)
    {
        _levels[S_INDEX_LOG_LEVEL_DEBUG] = true;
    }
    if (_mask & MyLogger::LL_INFO)
    {
        _levels[S_INDEX_LOG_LEVEL_INFO] = true;
    }
    if (_mask & MyLogger::LL_WARNING)
    {
        _levels[S_INDEX_LOG_LEVEL_WARNING] = true;
    }
    if (_mask & MyLogger::LL_ERROR)
    {
        _levels[S_INDEX_LOG_LEVEL_ERROR] = true;
    }
    if (_mask & MyLogger::LL_FATAL)
    {
        _levels[S_INDEX_LOG_LEVEL_FATAL] = true;
    }
}

void CSettinsModelData::createSettingsView()
{
    quint8 _mask = NHelper::getLoggerMask();
    QVector<bool> levelArr(S_LOG_LEVEL_COUNT);

    CSettinsModelData::updateLoggerSettingsArray(_mask, levelArr);

    setLoggerSettings(levelArr);
    setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());
}
