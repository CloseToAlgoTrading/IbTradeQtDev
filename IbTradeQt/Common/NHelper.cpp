#include "NHelper.h"
#include "StorageConfig.h"
#include <QSettings>
#include <QFile>

namespace {

QString settingsFilePath()
{
    const QString overridePath =
        qEnvironmentVariable("IBTRADE_SETTINGS_FILE").trimmed();
    if (!overridePath.isEmpty())
        return overridePath;
    return QStringLiteral(SETTINGS_FILE_NAME);
}

} // namespace

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
            l_dt = QDateTime::fromString(date, "yyyyMMdd");
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
    return static_cast<qint32>(getStorageConfig().legacyMarketDataDb.port);
}

quint16 NHelper::getServerPort()
{
    quint16 ret_port = 4002;
    QSettings dbsettings(settingsFilePath(), QSettings::IniFormat);
    ret_port = static_cast<quint16>(dbsettings.value("Server/serverport", ret_port).toUInt());

    return ret_port;
}

void NHelper::initSettings()
{
    const QString path = settingsFilePath();
    if (!QFile(path).exists())
    {
        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup("Server");
        settings.setValue("serverport", 4002);
        settings.setValue("serveraddr", "localhost");
        settings.endGroup();
        settings.beginGroup("Logger");
        settings.setValue("mask", 0);
        settings.endGroup();
        settings.sync();

        const StorageConfig cfg = StorageConfig::loadDefaults();
        StorageConfig::saveToIni(path, cfg);
    }
}

StorageConfig NHelper::getStorageConfig()
{
    return StorageConfig::loadFromIni(settingsFilePath());
}

void NHelper::saveStorageConfig(const StorageConfig& cfg)
{
    StorageConfig::saveToIni(settingsFilePath(), cfg);
}

QString NHelper::getServerAddress()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    return settings.value("Server/serveraddr", "localhost").toString();
}

QString NHelper::getDBServerAddress()
{
    return getStorageConfig().legacyMarketDataDb.host;
}

QString NHelper::getDBUser()
{
    return getStorageConfig().legacyMarketDataDb.user;
}

QString NHelper::getDBName()
{
    return getStorageConfig().legacyMarketDataDb.database;
}

quint8 NHelper::getLoggerMask()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    return static_cast<quint8>(settings.value("Logger/mask", 0).toUInt());
}

void NHelper::writeLoggerMask(const quint8 _mask)
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("Logger/mask", _mask);
    settings.sync();
}

QString NHelper::getDBPswd()
{
    return getStorageConfig().legacyMarketDataDb.password;
}

void NHelper::writeServerPort(const qint32 &_port)
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("Server/serverport", _port);
    settings.sync();
}

void NHelper::writeServerAddress(const QString &_adr)
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("Server/serveraddr", _adr);
    settings.sync();
}

void NHelper::writeSomeData(const QString &_dataslot, const QVariant &_data)
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue(_dataslot, _data);
    settings.sync();
}

const QVariant NHelper::readSomeData(const QString &_dataslot, const QVariant &_data)
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    return settings.value(_dataslot, _data);
}

QString NHelper::convertQTDataTimeToString(quint64 time, QString format)
{
    return QDateTime::fromMSecsSinceEpoch(time).toString(format);

}
