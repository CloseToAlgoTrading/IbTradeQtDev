#include "DataManagement/ExportFilenameUtils.h"

#include <QChar>

namespace DataManagement {

QString sanitizeFilenameSegment(const QString& raw, int maxLen)
{
    QString out;
    out.reserve(qMin(raw.size(), maxLen));
    for (const QChar& c : raw) {
        const ushort u = c.unicode();
        const bool ok = (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z') || (u >= '0' && u <= '9')
                        || u == '.' || u == '_' || u == '-';
        out.append(ok ? c : QChar(QLatin1Char('_')));
        if (out.size() >= maxLen)
            break;
    }
    if (out.isEmpty())
        out = QStringLiteral("x");
    return out;
}

QString buildExportBasename(const QString& symbol, const QString& resolution,
                            const QString& dataSourceId)
{
    return sanitizeFilenameSegment(symbol) + QStringLiteral("__")
        + sanitizeFilenameSegment(resolution) + QStringLiteral("__")
        + sanitizeFilenameSegment(dataSourceId.toLower()) + QStringLiteral(".csv");
}

} // namespace DataManagement
