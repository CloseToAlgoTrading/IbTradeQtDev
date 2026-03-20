
#include "ciconhandler.h"
#include "ThemePalette.h"

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QStringList>

namespace {

bool skipIconTint(const QString &iconName)
{
    // Keep semantic colors from PNGs. "NotConnected" is monochrome-on-dark — tint it.
    static const QStringList kSkip{
        QStringLiteral("Connected"),
    };
    return kSkip.contains(iconName);
}

/**
 * Line-art toolbar icons: map dark "ink" to tint, drop light/white (opaque background).
 *
 * Using (tint * gray) / 255 leaves pure black (gray==0) at rgb 0 — still invisible on
 * dark chrome. Use ink = 255 - gray so black → full tint; white → transparent.
 */
QPixmap tintMonochromePixmap(const QPixmap &source, const QColor &tint)
{
    if (source.isNull())
        return source;

    QImage img = source.toImage().convertToFormat(QImage::Format_ARGB32);
    const int tr = tint.red();
    const int tg = tint.green();
    const int tb = tint.blue();

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QRgb pixel = img.pixel(x, y);
            const int a = qAlpha(pixel);
            if (a == 0)
                continue;

            const int ink = 255 - qGray(pixel);
            if (ink <= 0) {
                img.setPixel(x, y, qRgba(0, 0, 0, 0));
                continue;
            }

            const int nr = qBound(0, (tr * ink) / 255, 255);
            const int ng = qBound(0, (tg * ink) / 255, 255);
            const int nb = qBound(0, (tb * ink) / 255, 255);
            const int na = qBound(0, (a * ink + 127) / 255, 255);
            img.setPixel(x, y, qRgba(nr, ng, nb, na));
        }
    }

    QPixmap out = QPixmap::fromImage(std::move(img));
    out.setDevicePixelRatio(source.devicePixelRatio());
    return out;
}

} // namespace

CIconHandler::CIconHandler()
    : m_themePath(":/IBTradeSystem/x_resources/icons/default/%1.png")
{

}

QIcon CIconHandler::loadIconFromResourceTheme(const QString &iconName) const
{
    QString iconPath = m_themePath.arg(iconName);
    return QIcon(iconPath);
}

QIcon CIconHandler::loadIconForChrome(const QString &iconName) const
{
    QString iconPath = m_themePath.arg(iconName);

    QPixmap pm(iconPath);
    if (pm.isNull())
        return QIcon(iconPath);

    if (skipIconTint(iconName))
        return QIcon(pm);

    const QColor tint(QString::fromLatin1(UiTheme::kToolbarIcon));
    QIcon icon;
    icon.addPixmap(tintMonochromePixmap(pm, tint));
    return icon;
}
