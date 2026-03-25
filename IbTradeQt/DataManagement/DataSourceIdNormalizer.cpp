#include "DataManagement/DataSourceIdNormalizer.h"

#include <QRegularExpression>

namespace DataManagement {

QString normalizeDataSourceId(const QString& raw)
{
    QString s = raw.trimmed().toLower();
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    if (s.isEmpty())
        return {};

    static const QRegularExpression allowed(QStringLiteral("^[a-z0-9._-]+$"));
    if (!allowed.match(s).hasMatch())
        return {};
    return s;
}

bool isValidNormalizedDataSourceId(const QString& s)
{
    return !s.isEmpty() && normalizeDataSourceId(s) == s;
}

} // namespace DataManagement
