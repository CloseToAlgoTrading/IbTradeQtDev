#include "NHelper.h"
#include <QSettings>
#include <QFile>


quint64 NHelper::convertTimeStringToTimestamp(QString date, bool isFormat)
{
    QDateTime l_dt;
    quint64 retVal = 0;

    if (isFormat)
    {
        l_dt = QDateTime::fromString(date, "yyyyMMdd  hh:mm:ss");
    }
    else
    {
        l_dt = QDateTime::fromString(date);
    }

    if (l_dt.isValid())
    {
        retVal = l_dt.toMSecsSinceEpoch();
    }
    else
    {
        if (!isFormat)
        {
            l_dt = QDateTime::fromString(date, "yyyyMMdd  hh:mm:ss");
        }
        else
        {
            l_dt = QDateTime::fromString(date);
        }

        if (l_dt.isValid())
        {
            retVal = l_dt.toMSecsSinceEpoch();
        }

    }

    return retVal;

}

qint32 NHelper::getDBPort()
{
    qint32 ret_port = 5432;
    if (QFile(SETTINGS_FILE_NAME).exists())
    {
        QSettings dbsettings(SETTINGS_FILE_NAME, QSettings::IniFormat);
        ret_port = dbsettings.value("DBSettings/dbport", ret_port).toInt();
    }
    else
    {
        QSettings dbsettings(SETTINGS_FILE_NAME, QSettings::IniFormat);
        dbsettings.setValue("DBSettings/dbport", ret_port);
        dbsettings.sync();
    }

    return ret_port;
}

quint16 NHelper::getServerPort()
{
    quint16 ret_port = 4002;
    QSettings dbsettings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    ret_port = static_cast<quint16>(dbsettings.value("Server/serverport", ret_port).toUInt());

    return ret_port;
}

void NHelper::initSettings()
{
    if (!QFile(SETTINGS_FILE_NAME).exists())
    {
        QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
        settings.beginGroup("Server");
        settings.setValue("serverport", 4002);
        settings.setValue("serveraddr", "localhost");
        settings.endGroup();
        settings.beginGroup("DBSettings");
        settings.setValue("dbport", 5432);
        settings.setValue("dbaddress", "192.168.0.90");
        settings.setValue("dbusr", "postgres");
        settings.setValue("dbname", "IbTrade");
        settings.setValue("pswd", "docker");
        settings.endGroup();
        settings.beginGroup("Logger");
        settings.setValue("mask", 0);
        settings.endGroup();
        settings.sync();
    }
}

QString NHelper::getServerAddress()
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return settings.value("Server/serveraddr", "localhost").toString();
}

QString NHelper::getDBServerAddress()
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return settings.value("DBSettings/dbaddress", "localhost").toString();
}

QString NHelper::getDBUser()
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return settings.value("DBSettings/dbusr", "postgres").toString();
}

QString NHelper::getDBName()
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return settings.value("DBSettings/dbname", "IbTrade").toString();
}

quint8 NHelper::getLoggerMask()
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return static_cast<quint8>(settings.value("Logger/mask", 0).toUInt());
}

void NHelper::writeLoggerMask(const quint8 _mask)
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    settings.setValue("Logger/mask", _mask);
    settings.sync();
}

QString NHelper::getDBPswd()
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return settings.value("DBSettings/pswd", "passw").toString();
}

void NHelper::writeServerPort(const qint32 &_port)
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    settings.setValue("Server/serverport", _port);
    settings.sync();
}

void NHelper::writeServerAddress(const QString &_adr)
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    settings.setValue("Server/serveraddr", _adr);
    settings.sync();
}

void NHelper::writeSomeData(const QString &_dataslot, const QVariant &_data)
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    settings.setValue(_dataslot, _data);
    settings.sync();
}

const QVariant NHelper::readSomeData(const QString &_dataslot, const QVariant &_data)
{
    QSettings settings(SETTINGS_FILE_NAME, QSettings::IniFormat);
    return settings.value(_dataslot, _data);
}
