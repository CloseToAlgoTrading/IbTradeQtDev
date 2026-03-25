#ifndef DATAMANAGEMENT_BARRESOLUTIONCONSTANTS_H
#define DATAMANAGEMENT_BARRESOLUTIONCONSTANTS_H

#include <QStringList>

namespace DataManagement {

/// Canonical bar resolutions — aligned with BacktestRunConfigPanel / BacktestController.
inline QStringList canonicalBarResolutions()
{
    return QStringList{QStringLiteral("Day1"), QStringLiteral("Hour1"), QStringLiteral("Min30"),
                       QStringLiteral("Min15"), QStringLiteral("Min5"), QStringLiteral("Min1")};
}

inline bool isCanonicalResolution(const QString& r)
{
    return canonicalBarResolutions().contains(r);
}

} // namespace DataManagement

#endif
