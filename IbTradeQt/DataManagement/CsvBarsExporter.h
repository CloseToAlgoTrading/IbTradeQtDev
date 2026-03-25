#ifndef DATAMANAGEMENT_CSVBARSEXPORTER_H
#define DATAMANAGEMENT_CSVBARSEXPORTER_H

#include "DataManagement/DataManagementTypes.h"
#include <QString>

namespace DataManagement {

/// Stream UTF-8 CSV with \\n line endings; timestamps UTC Qt::ISODate.
bool exportDatasetToFile(const QString& connectionName,
                           const HistoricalBarsDatasetKey& key,
                           const QString& filePath,
                           QString* errorOut = nullptr);

} // namespace DataManagement

#endif
