
#ifndef CICONHANDLER_H
#define CICONHANDLER_H

#include <QIcon>
#include <QString>

class CIconHandler
{
public:
    CIconHandler();

    // Helper function to load an icon from the embedded resource theme
    QIcon loadIconFromResourceTheme(const QString& iconName) const;

protected:
    QString m_themePath;
};

#endif // CICONHANDLER_H
