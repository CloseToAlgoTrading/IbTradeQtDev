#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <QtCore/qglobal.h>

class GlobalSettings
{
public:
    GlobalSettings();

    static void readSettings();

    static quint16 getDbServerPort();
};

#endif // GLOBALSETTINGS_H
