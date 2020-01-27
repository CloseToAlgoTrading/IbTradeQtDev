#include "cstandartitemsettings.h"
#include <QStandardItem>
#include "MyLogger.h"


CStandartItemSettings::CStandartItemSettings(QObject *parent)
    : QObject(parent)
    , m_pModel(new QStandardItemModel(CROW,CCOL))
    , m_LogItems()
    , m_pServerAddressItem(nullptr)
    , m_pServerPortItem(nullptr)
    , m_logLevel(LOG_LEVEL_COUNT)
{
    generateModel();
    //QObject::connect(m_pModel, &QStandardItemModel::dataChanged, this, &CStandartItemSettings::dataChangeCallback, Qt::AutoConnection);
    //QObject::connect(m_pModel, &QStandardItemModel::itemChanged, this, &CStandartItemSettings::itemChangeCallback, Qt::AutoConnection);
}

void CStandartItemSettings::generateModel()
{
    QStandardItem *infoItem = new QStandardItem("Description");
    QStandardItem *valueItem = new QStandardItem("Value");
    m_pModel->setHorizontalHeaderItem( 0, infoItem );
    m_pModel->setHorizontalHeaderItem( 1, valueItem );

    m_pModel->setItem(0,0,createSubCategoryTextItem("Log Settings"));
    m_pModel->setItem(0,1,createSubCategoryTextItem(""));

    m_pModel->setItem(INDEX_LOG_LEVEL_ALL    ,0,createLogDescriptionTextItem("All", true));
    m_pModel->setItem(INDEX_LOG_LEVEL_FATAL  ,0,createLogDescriptionTextItem("Fatal"));
    m_pModel->setItem(INDEX_LOG_LEVEL_ERROR  ,0,createLogDescriptionTextItem("Error", true));
    m_pModel->setItem(INDEX_LOG_LEVEL_WARNING,0,createLogDescriptionTextItem("Warning"));
    m_pModel->setItem(INDEX_LOG_LEVEL_INFO   ,0,createLogDescriptionTextItem("Info", true));
    m_pModel->setItem(INDEX_LOG_LEVEL_DEBUG  ,0,createLogDescriptionTextItem("Debug"));

    m_pModel->setItem(7,0,createSubCategoryTextItem("Server Settings"));
    m_pModel->setItem(7,1,createSubCategoryTextItem(""));

    m_pModel->setItem(INDEX_SEVER_ADDRESS,0,createServerDescriptionTextItem("Address", true));
    m_pModel->setItem(INDEX_SEVER_PORT,0,createServerDescriptionTextItem("Port"));

    //------ create data item ---------
    m_LogItems.push_back(createLogDataItem(Qt::Unchecked, true));
    m_LogItems.push_back(createLogDataItem(Qt::Unchecked));
    m_LogItems.push_back(createLogDataItem(Qt::Unchecked, true));
    m_LogItems.push_back(createLogDataItem(Qt::Unchecked));
    m_LogItems.push_back(createLogDataItem(Qt::Unchecked, true));
    m_LogItems.push_back(createLogDataItem(Qt::Unchecked));

    for (int var = 1; var < 7; ++var) {
        m_pModel->setItem(var,1,m_LogItems[var-1]);
    }

    m_pServerAddressItem = createTextDataItem("localhost", QBrush(QColor(C_COLOR_L_SERVER)));
    m_pServerPortItem = createNumberDataItem(4002, QBrush(QColor(C_COLOR_N_SERVER)));

    m_pModel->setItem(8, 1, m_pServerAddressItem);
    m_pModel->setItem(9, 1, m_pServerPortItem);
}

QStandardItem *CStandartItemSettings::createSubCategoryTextItem(const QString &_text)
{
    QStandardItem *logTextItem = new QStandardItem(_text);
    logTextItem->setBackground(QBrush(QColor(177, 180, 181)));
    QFont _font = logTextItem->font();
    _font.setBold(true);
    _font.setItalic(true);
    logTextItem->setFont(_font);
    logTextItem->setFlags(logTextItem->flags() & (~Qt::ItemFlag::ItemIsEditable));
    return logTextItem;
}

QStandardItem *CStandartItemSettings::createDecriptionTextItem(const QString &_text, const QBrush & _brush)
{
    QStandardItem *_Item = new QStandardItem(_text);
    _Item->setBackground(_brush);
    _Item->setFlags(_Item->flags() & (~Qt::ItemFlag::ItemIsEditable));
    return _Item;
}

QStandardItem *CStandartItemSettings::createLogDescriptionTextItem(const QString &_text, const bool isLight)
{
    quint32 _color = C_COLOR_N_LOG;
    if(true == isLight)
    {
        _color = C_COLOR_L_LOG;
    }

    return createDecriptionTextItem(_text, QBrush(QColor(_color)));
}

QStandardItem *CStandartItemSettings::createServerDescriptionTextItem(const QString &_text, const bool isLight)
{
    quint32 _color = C_COLOR_N_SERVER;
    if(true == isLight)
    {
        _color = C_COLOR_L_SERVER;
    }

    return createDecriptionTextItem(_text, QBrush(QColor(_color)));
}

QStandardItem *CStandartItemSettings::createCheckableDataItem(const Qt::CheckState _isChecked, const QBrush & _brush)
{
    QStandardItem *_Item = new QStandardItem();
    _Item->setBackground(_brush);
    _Item->setFlags(_Item->flags() & (~Qt::ItemFlag::ItemIsEditable) | (Qt::ItemFlag::ItemIsUserCheckable));
    _Item->setCheckable(true);
    _Item->setCheckState(_isChecked);
    return _Item;
}

QStandardItem *CStandartItemSettings::createLogDataItem(const Qt::CheckState _isChecked, const bool isLight)
{
    quint32 _color = 0xf4f2e1u;
    if(true == isLight)
    {
        _color = 0xfffdefu;
    }

    return createCheckableDataItem(_isChecked, QBrush(QColor(_color)));
}

QStandardItem *CStandartItemSettings::createTextDataItem(const QString &_text, const QBrush &_brush)
{
    QStandardItem *_Item = new QStandardItem(_text);
    _Item->setBackground(_brush);
    _Item->setFlags(_Item->flags() | (Qt::ItemFlag::ItemIsEditable));
    _Item->setEditable(true);
    return _Item;
}

QStandardItem *CStandartItemSettings::createNumberDataItem(const qint32 &_number, const QBrush &_brush)
{
    QStandardItem *_Item = new QStandardItem(_number);
    _Item->setData(_number, Qt::DisplayRole);
    _Item->setBackground(_brush);
    _Item->setFlags(_Item->flags() | (Qt::ItemFlag::ItemIsEditable));
    _Item->setEditable(true);
    return _Item;
}

void CStandartItemSettings::dataChangeCallback(const QModelIndex &topLeft, const
                                               QModelIndex &bottomRight)
{
    Q_UNUSED(bottomRight);
    qint32 _row = topLeft.row();
    if((INDEX_LOG_LEVEL_ALL <= _row) && (INDEX_LOG_LEVEL_DEBUG >= _row))
    {
        m_logLevel[_row - 1] = m_pModel->item(_row, 1)->data(Qt::CheckStateRole).toBool();
        emit signalEditLogSettingsCompleted();
    }
    else if(INDEX_SEVER_ADDRESS == _row)
    {
        QString sadr = m_pModel->item(_row, 1)->data(Qt::DisplayRole).toString();
        signalEditServerAddresCompleted(sadr);
    }
    else if(INDEX_SEVER_PORT == _row)
    {
        qint32 port = m_pModel->item(_row, 1)->data(Qt::DisplayRole).toInt();
        signalEditServerPortCompleted(port);
    }
}



void CStandartItemSettings::setLoggerSettings(const QVector<bool> _levels)
{
    m_logLevel = _levels;
    for (quint8 i = 0; i < LOG_LEVEL_COUNT; ++i) {

        m_LogItems[i]->setData(m_logLevel[i] ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
}

const QVector<bool> CStandartItemSettings::getLoggerSettings()
{
    return m_logLevel;
}

void CStandartItemSettings::setServerSettings(const QString &_addr, const quint16 &_port)
{
    m_pModel->item(INDEX_SEVER_ADDRESS, 1)->setData(_addr, Qt::DisplayRole);
    m_pModel->item(INDEX_SEVER_PORT, 1)->setData(_port, Qt::DisplayRole);
}

QStandardItemModel *CStandartItemSettings::getModel()
{
    return m_pModel;
}


quint8 CStandartItemSettings::getMaskFromLoggerSettings(const QVector<bool> &_levels)
{
    quint8 mask = static_cast<quint8>(MyLogger::LL_NONE);

    if(true == _levels[INDEX_LOG_LEVEL_ALL-1])
    {
        mask = MyLogger::LL_ALL;
    }
    else {
        if(true == _levels[INDEX_LOG_LEVEL_INFO-1])
        {
            mask |= MyLogger::LL_INFO;
        }
        if(true == _levels[INDEX_LOG_LEVEL_DEBUG-1])
        {
            mask |= MyLogger::LL_DEBUG;
        }
        if(true == _levels[INDEX_LOG_LEVEL_ERROR-1])
        {
            mask |= MyLogger::LL_ERROR;
        }
        if(true == _levels[INDEX_LOG_LEVEL_FATAL-1])
        {
            mask |= MyLogger::LL_FATAL;
        }
        if(true == _levels[INDEX_LOG_LEVEL_WARNING-1])
        {
            mask |= MyLogger::LL_WARNING;
        }
    }
    return mask;
}

void CStandartItemSettings::updateLoggerSettingsArray(quint8 _mask, QVector<bool> & _levels)
{
    if (_mask & MyLogger::LL_DEBUG)
    {
        _levels[INDEX_LOG_LEVEL_DEBUG-1] = true;
    }
    if (_mask & MyLogger::LL_INFO)
    {
        _levels[INDEX_LOG_LEVEL_INFO-1] = true;
    }
    if (_mask & MyLogger::LL_WARNING)
    {
        _levels[INDEX_LOG_LEVEL_WARNING-1] = true;
    }
    if (_mask & MyLogger::LL_ERROR)
    {
        _levels[INDEX_LOG_LEVEL_ERROR-1] = true;
    }
    if (_mask & MyLogger::LL_FATAL)
    {
        _levels[INDEX_LOG_LEVEL_FATAL-1] = true;
    }
}
