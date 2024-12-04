
#include "ciconhandler.h"

CIconHandler::CIconHandler()
    : m_themePath(":/IBTradeSystem/x_resources/icons/default/%1.png")
{

}

QIcon CIconHandler::loadIconFromResourceTheme(const QString &iconName) const
{
    // Construct the path to the icon within the resource file
    QString iconPath = m_themePath.arg(iconName);

    // Return the QIcon object created from the resource path
    return QIcon(iconPath);
}
