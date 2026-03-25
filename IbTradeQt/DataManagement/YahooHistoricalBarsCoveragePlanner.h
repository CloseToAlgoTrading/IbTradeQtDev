#ifndef DATAMANAGEMENT_YAHOOHISTORICALBARSCOVERAGEPLANNER_H
#define DATAMANAGEMENT_YAHOOHISTORICALBARSCOVERAGEPLANNER_H

#include "DataManagement/DataManagementTypes.h"
#include <QStringList>
#include <QVector>

namespace DataManagement {

/// Yahoo-specific segment planning — mirrors `HistoricalDataManager::getBars` gap policy for Day1 + yahoo.
/// Introduce a generic planner only when a second provider shares real segment logic.
class YahooHistoricalBarsCoveragePlanner {
public:
    struct Plan {
        QVector<SyncCoverageSegment> segments;
        QDateTime effectiveRequestedFromUtc;
        QDateTime effectiveRequestedToUtc;
        QDateTime previousMinUtc;
        QDateTime previousMaxUtc;
        bool hasPreviousRange = false;
        QStringList warnings;
    };

    Plan plan(const HistoricalBarsDatasetKey& key, const QDateTime& requestedFromUtc,
              const QDateTime& requestedToUtc, const QString& dbConnectionName) const;
};

} // namespace DataManagement

#endif
