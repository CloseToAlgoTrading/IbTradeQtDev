#include "globalsettings.h"
#include <QSettings>

static quint16 serverPort = 4002;

GlobalSettings::GlobalSettings()
{

}

void GlobalSettings::readSettings()
{
    QSettings sett("settings.ini", QSettings::IniFormat);
    serverPort = sett.value("DB/port", 21).toInt();
}



quint16 GlobalSettings::getDbServerPort()
{
    return serverPort;
}
