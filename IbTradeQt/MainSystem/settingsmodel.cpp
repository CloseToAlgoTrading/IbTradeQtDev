#include "settingsmodel.h"
#include <QFont>
#include <QDebug>
#include <QBrush>

#define ROW_INDEX_LOG_TEXT 0
#define ROW_INDEX_LAST_LOG S_LOG_LEVEL_COUNT
#define ROW_INDEX_SERVER_TEXT S_INDEX_SERVER_TEXT+1
#define ROW_INDEX_SERVER_ADDRESS S_INDEX_SERVER_ADDRESS+1
#define ROW_INDEX_SERVER_PORT S_INDEX_SERVER_PORT+1

SettingsModel::SettingsModel(QObject *parent) : QAbstractTableModel(parent)
{
    m_settngsName[0] = "Trace Level";
    m_settngsName[S_INDEX_LOG_LEVEL_ALL+1] = "ALL";
    m_settngsName[S_INDEX_LOG_LEVEL_DEBUG+1] = "Debug";
    m_settngsName[S_INDEX_LOG_LEVEL_INFO+1] = "Info";
    m_settngsName[S_INDEX_LOG_LEVEL_WARNING+1] = "Warning";
    m_settngsName[S_INDEX_LOG_LEVEL_FATAL+1] = "Fatal";
    m_settngsName[S_INDEX_LOG_LEVEL_ERROR+1] = "Error";
    m_settngsName[ROW_INDEX_SERVER_TEXT] = "Broker Server";
    m_settngsName[ROW_INDEX_SERVER_ADDRESS] = "Address";
    m_settngsName[ROW_INDEX_SERVER_PORT] = "Port";

}

int SettingsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ROWS;
}

int SettingsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return COLS;
}

QVariant SettingsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    switch (role) {
    case Qt::DisplayRole:
        if(0 == col)
        {
            return m_settngsName[row];
        }
        else if((1 == col)
                && (ROW_INDEX_SERVER_PORT == row))
        {
            return m_serverPort;
        }
        else if((1 == col)
                && (ROW_INDEX_SERVER_ADDRESS == row))
        {
            //return m_serverAddress;
            return m_severAddress2.toString();
        }
        break;

    case Qt::FontRole:
        if ((0 == row && 0 == col)
            || (ROW_INDEX_SERVER_TEXT == row && 0 == col))
        {
            QFont boldFont;
            boldFont.setBold(true);
            return boldFont;
        }
        break;

    case Qt::BackgroundRole:
        if ((0 == row) || (ROW_INDEX_SERVER_TEXT == row))
            return QBrush(QColor(177, 180, 181));

        break;

    case Qt::TextAlignmentRole:
        if (row == 1 && col == 1) //change text alignment only for cell(1,1)
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        break;

    case Qt::CheckStateRole:
        if ((col == 1) && (row >= 1) && (row <= ROW_INDEX_LAST_LOG))
        {
            return m_logLevel[row-1] ? Qt::Checked : Qt::Unchecked;

        }
        break;
    }
    return QVariant();
}

QVariant SettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("Porperty");
        case 1:
            return QString("Value");
        }
    }
    return QVariant();
}

bool SettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool ret = false;
    if (!index.isValid() /*|| role != Qt::EditRole*/)
        return false;

    if (role == Qt::EditRole) {
        if(ROW_INDEX_SERVER_ADDRESS == index.row())
        {
            QHostAddress address(value.toString());
            if (QAbstractSocket::IPv4Protocol == address.protocol())
            {
               m_severAddress2.setAddress(value.toString());
            }

        }
        else if(ROW_INDEX_SERVER_PORT == index.row())
        {
            m_serverPort = value.toUInt();
        }
        ret = true;
    }

    if (role == Qt::CheckStateRole)
    {
        m_logLevel[index.row()-1] = value.toBool();
        emit signalEditCompleted();
        ret = true;
    }

    return ret;
}

Qt::ItemFlags SettingsModel::flags(const QModelIndex &index) const
{
    if(index.column() == 0)
    {
       return QAbstractTableModel::flags(index);
    }
    else if((index.column() == 1) && (index.row() >= 1) && (index.row() <= ROW_INDEX_LAST_LOG))
    {
       return Qt::ItemIsUserCheckable | QAbstractTableModel::flags(index);
    }
    else if((index.column() == 1) && (index.row() > ROW_INDEX_SERVER_TEXT))
    {
       return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    }

    return QAbstractTableModel::flags(index);
}

void SettingsModel::setLoggerSettings(const bool _levels[])
{
    memcpy(m_logLevel, _levels, S_LOG_LEVEL_COUNT);
}

const bool *SettingsModel::getLoggerSettings()
{
    return m_logLevel;
}

void SettingsModel::setServerSettings(const QString &_addr, const quint16 &_port)
{
    m_serverAddress = _addr;
    m_severAddress2.setAddress(_addr);
    m_serverPort = _port;
}
