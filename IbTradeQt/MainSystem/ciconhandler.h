
#ifndef CICONHANDLER_H
#define CICONHANDLER_H

#include <QIcon>
#include <QString>

class CIconHandler
{
public:
    CIconHandler();

    /** Icons for trees, lists, delegates — uses PNG as-is (may be full colour). */
    QIcon loadIconFromResourceTheme(const QString& iconName) const;

    /**
     * Icons on dark chrome (toolbars, status bar, context menus): tints monochrome
     * assets to UiTheme::kToolbarIcon; skips only semantic glyphs (e.g. Connected).
     */
    QIcon loadIconForChrome(const QString& iconName) const;

protected:
    QString m_themePath;
};

#endif // CICONHANDLER_H
