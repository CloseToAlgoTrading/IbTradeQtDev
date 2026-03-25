#ifndef DATAMANAGEMENT_DATAMANAGEMENTTYPES_H
#define DATAMANAGEMENT_DATAMANAGEMENTTYPES_H

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMetaType>

namespace DataManagement {

enum class DataManagementOperation {
    Inventory,
    Delete,
    Export,
    Import,
    Preview,
    CoverageSync,
    YahooBatchImport
};

struct HistoricalBarsDatasetKey {
    QString symbol;
    QString resolution;
    QString dataSourceId;

    bool operator==(const HistoricalBarsDatasetKey& o) const
    {
        return symbol == o.symbol && resolution == o.resolution && dataSourceId == o.dataSourceId;
    }
};

inline uint qHash(const HistoricalBarsDatasetKey& k, uint seed = 0)
{
    return qHash(k.symbol, seed) ^ qHash(k.resolution, seed) ^ qHash(k.dataSourceId, seed);
}

struct HistoricalBarsDataset {
    QString symbol;
    QString resolution;
    QString dataSourceId;
    qint64  rowCount = 0;
    QDateTime fromUtc;
    QDateTime toUtc;
};

struct DataManagementContext {
    QString appDbPath;
    QString backtestDbPath;
    QString activeBarsPath;
    bool    legacyPostgresConfigured = false;
};

struct HistoricalBarPreviewRow {
    QDateTime timestampUtc;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
};

struct HistoricalBarsPreview {
    HistoricalBarsDatasetKey key;
    QVector<HistoricalBarPreviewRow> firstRowsAsc;
    QVector<HistoricalBarPreviewRow> lastRowsAsc;
};

struct DataManagementError {
    DataManagementOperation operation = DataManagementOperation::Inventory;
    quint64 operationId = 0;
    QString message;
    bool keyValid = false;
    HistoricalBarsDatasetKey key;
    QString path;
    int importLine = -1;
};

enum class ExportFileAction {
    Written,
    Overwritten,
    Skipped,
    Failed
};

struct ExportFileResult {
    HistoricalBarsDatasetKey key;
    QString path;
    bool ok = false;
    QString errorMessage;
    ExportFileAction action = ExportFileAction::Failed;
};

struct ExportBatchSummary {
    quint64 operationId = 0;
    QVector<ExportFileResult> files;
};

enum class ExportOverwritePolicy {
    Overwrite,
    Skip,
    Cancel
};

struct ImportResult {
    int rowsParsed = 0;
    int rowsUpserted = 0;
};

/// Segment of a coverage sync fetch (normalized UTC bounds; Yahoo planner-owned).
enum class SyncCoverageSegmentKind { Full, Prefix, Suffix };

struct SyncCoverageSegment {
    SyncCoverageSegmentKind kind = SyncCoverageSegmentKind::Full;
    QDateTime fromUtc;
    QDateTime toUtc;
};

/// Success payload for coverage sync only — failures use `barsCoverageSyncFailed` + `DataManagementError`.
/// `barsFetched` = total raw bars from HTTP across all segments; `rowsUpserted` includes INSERT OR REPLACE.
struct SyncCoverageResult {
    HistoricalBarsDatasetKey key;
    QDateTime requestedFromUtc;
    QDateTime requestedToUtc;
    QDateTime effectiveRequestedFromUtc;
    QDateTime effectiveRequestedToUtc;
    QDateTime previousMinUtc;
    QDateTime previousMaxUtc;
    bool hasPreviousRange = false;
    QVector<SyncCoverageSegment> segments;
    qint64 barsFetched = 0;
    qint64 rowsUpserted = 0;
    /// Optional; filled when cheap to compute (e.g. MIN/MAX query after upsert). UI may still prefer inventory.
    QDateTime resultingMinUtc;
    QDateTime resultingMaxUtc;
    bool hasResultingRange = false;
    QStringList warnings;
};

/// Aggregate result for multi-symbol Yahoo Day1 fetch (same semantics as `SyncCoverageResult` per symbol).
struct BatchYahooImportResult {
    int      symbolsRequested = 0;
    int      symbolsSucceeded = 0;
    qint64   totalBarsFetched = 0;
    qint64   totalRowsUpserted = 0;
    QStringList failedSymbols;
    QStringList failedReasons;
};

} // namespace DataManagement

Q_DECLARE_METATYPE(DataManagement::HistoricalBarsDatasetKey)
Q_DECLARE_METATYPE(DataManagement::HistoricalBarsDataset)
Q_DECLARE_METATYPE(DataManagement::DataManagementError)
Q_DECLARE_METATYPE(DataManagement::ExportBatchSummary)
Q_DECLARE_METATYPE(DataManagement::ImportResult)
Q_DECLARE_METATYPE(DataManagement::HistoricalBarsPreview)
Q_DECLARE_METATYPE(DataManagement::SyncCoverageSegment)
Q_DECLARE_METATYPE(DataManagement::SyncCoverageResult)
Q_DECLARE_METATYPE(DataManagement::BatchYahooImportResult)

#endif
