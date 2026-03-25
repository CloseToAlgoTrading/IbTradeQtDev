#ifndef DATAMANAGEMENT_EXPORTFILENAMEUTILS_H
#define DATAMANAGEMENT_EXPORTFILENAMEUTILS_H

#include <QString>

namespace DataManagement {

/// Sanitize one path segment for export filenames: [A-Za-z0-9._-], max length.
QString sanitizeFilenameSegment(const QString& raw, int maxLen = 64);

QString buildExportBasename(const QString& symbol, const QString& resolution,
                            const QString& dataSourceId);

} // namespace DataManagement

#endif
