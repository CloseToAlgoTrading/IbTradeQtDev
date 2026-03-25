#ifndef DATAMANAGEMENT_DATASOURCEIDNORMALIZER_H
#define DATAMANAGEMENT_DATASOURCEIDNORMALIZER_H

#include <QString>

namespace DataManagement {

/// Normalizes dataSourceId for storage: trim, lower, spaces→_, charset [a-z0-9._-].
/// On invalid empty or bad chars, returns empty string (caller treats as error).
QString normalizeDataSourceId(const QString& raw);

bool isValidNormalizedDataSourceId(const QString& s);

} // namespace DataManagement

#endif
