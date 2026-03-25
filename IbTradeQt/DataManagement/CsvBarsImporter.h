#ifndef DATAMANAGEMENT_CSVBARSIMPORTER_H
#define DATAMANAGEMENT_CSVBARSIMPORTER_H

#include "DB/dbdatatypes.h"
#include <QString>
#include <QList>

namespace DataManagement {

struct CsvBarsImportParseResult {
    bool ok = false;
    QString error;
    int failLine = 0;
    QList<DbHistoricalBar> bars;
};

/// UTF-8, header must match CsvHistoricalDataSource. Fail-fast on first bad data row.
CsvBarsImportParseResult parseCsvBarsForImport(const QString& filePath,
                                                const QString& resolution,
                                                const QString& normalizedDataSourceId);

} // namespace DataManagement

#endif
