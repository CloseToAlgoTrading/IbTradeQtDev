#ifndef NHELPER_H
#define NHELPER_H

#include <QString>
#include <QDateTime>
#include <QVariant>


#pragma once

#define SETTINGS_FILE_NAME "ibtrade.ini"

namespace NHelper
{


// functions
    QString convertQTDataTimeToString(quint64 time, QString format="yyyy/MM/dd hh:mm:ss");
    quint64 convertTimeStringToTimestamp(QString date, bool isFormat = false);

    void initSettings();

    quint16 getServerPort();
    void writeServerPort(const qint32 & _port);
    QString getServerAddress();
    void writeServerAddress(const QString & _adr);

    qint32 getDBPort();
    QString getDBServerAddress();
    QString getDBUser();
    QString getDBName();
    QString getDBPswd();

    quint8 getLoggerMask();
    void writeLoggerMask(const quint8 _mask);

    void writeSomeData(const QString & _dataslot, const QVariant & _data);
    const QVariant readSomeData(const QString & _dataslot, const QVariant & _data);

}





#endif //GRAPHHELPER_H

