#ifndef PIPELINE_JSONCONFIGVALUE_H
#define PIPELINE_JSONCONFIGVALUE_H

#include <QJsonValue>
#include <QString>

namespace Pipeline {

inline int jsonInt(const QJsonValue& value, int fallback)
{
    if (value.isDouble())
        return value.toInt(fallback);
    bool ok = false;
    const int parsed = value.toString().trimmed().toInt(&ok);
    return ok ? parsed : fallback;
}

inline double jsonDouble(const QJsonValue& value, double fallback)
{
    if (value.isDouble())
        return value.toDouble(fallback);
    bool ok = false;
    const double parsed = value.toString().trimmed().toDouble(&ok);
    return ok ? parsed : fallback;
}

inline bool jsonBool(const QJsonValue& value, bool fallback)
{
    if (value.isBool())
        return value.toBool(fallback);

    const QString s = value.toString().trimmed().toLower();
    if (s == QLatin1String("true") || s == QLatin1String("1")
        || s == QLatin1String("yes") || s == QLatin1String("on")) {
        return true;
    }
    if (s == QLatin1String("false") || s == QLatin1String("0")
        || s == QLatin1String("no") || s == QLatin1String("off")) {
        return false;
    }
    return fallback;
}

} // namespace Pipeline

#endif // PIPELINE_JSONCONFIGVALUE_H
