#include "DataManagement/DataManagementWorker.h"
#include "DataManagement/BarResolutionConstants.h"
#include "DataManagement/CsvBarsImporter.h"
#include "DataManagement/CsvBarsExporter.h"
#include "DataManagement/ExportFilenameUtils.h"
#include "DataManagement/HistoricalBarsCoverageSyncRegistry.h"
#include "DataManagement/IHistoricalBarsCoverageSyncProvider.h"
#include "DB/dbquery.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QSqlDatabase>
#include <QSqlError>

namespace DataManagement {

DataManagementWorker::DataManagementWorker(QObject* parent)
    : QObject(parent)
    , m_repo(QStringLiteral("DataMgmtBacktest_") + QString::number(quintptr(this), 16))
{}

void DataManagementWorker::setBrokerDataProvider(CBrokerDataProvider* provider)
{
    m_brokerDataProvider = provider;
}

void DataManagementWorker::ensureNetworkManager()
{
    if (!m_networkManager)
        m_networkManager = new QNetworkAccessManager(this);
}

void DataManagementWorker::doInventory(quint64 operationId, QString dbPath)
{
    QString err;
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Inventory;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        emit inventoryFailed(operationId, e);
        return;
    }
    QList<HistoricalBarsDataset> rows = m_repo.loadInventory(&err);
    if (!err.isEmpty()) {
        DataManagementError e;
        e.operation = DataManagementOperation::Inventory;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        emit inventoryFailed(operationId, e);
        return;
    }
    emit inventoryFinished(operationId, rows);
}

void DataManagementWorker::doDelete(quint64 operationId, QString dbPath,
                                    QList<HistoricalBarsDatasetKey> keys)
{
    QString err;
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Delete;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        emit deleteFailed(operationId, e);
        return;
    }
    if (!m_repo.deleteDatasets(keys, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Delete;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        emit deleteFailed(operationId, e);
        return;
    }
    emit deleteFinished(operationId);
}

void DataManagementWorker::doPreview(quint64 operationId, QString dbPath,
                                     HistoricalBarsDatasetKey key, int firstN, int lastN)
{
    QString err;
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Preview;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        e.keyValid = true;
        e.key = key;
        emit previewFailed(operationId, e);
        return;
    }
    HistoricalBarsPreview pv;
    if (!m_repo.loadPreview(key, firstN, lastN, &pv, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Preview;
        e.operationId = operationId;
        e.message = err;
        e.keyValid = true;
        e.key = key;
        emit previewFailed(operationId, e);
        return;
    }
    emit previewFinished(operationId, pv);
}

void DataManagementWorker::doImport(quint64 operationId, QString dbPath, QString csvPath,
                                    QString resolution, QString normalizedDataSourceId)
{
    QString err;
    if (!isCanonicalResolution(resolution)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Import;
        e.operationId = operationId;
        e.message = QStringLiteral("Invalid resolution");
        e.path = csvPath;
        emit importFailed(operationId, e);
        return;
    }
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Import;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        emit importFailed(operationId, e);
        return;
    }

    CsvBarsImportParseResult parse =
        parseCsvBarsForImport(csvPath, resolution, normalizedDataSourceId);
    if (!parse.ok) {
        DataManagementError e;
        e.operation = DataManagementOperation::Import;
        e.operationId = operationId;
        e.message = parse.error;
        e.importLine = parse.failLine;
        e.path = csvPath;
        emit importFailed(operationId, e);
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(m_repo.connectionName());
    if (!db.transaction()) {
        DataManagementError e;
        e.operation = DataManagementOperation::Import;
        e.operationId = operationId;
        e.message = db.lastError().text();
        e.path = csvPath;
        emit importFailed(operationId, e);
        return;
    }

    for (const DbHistoricalBar& b : parse.bars) {
        auto q = query_upsertHistoricalBar(b, m_repo.connectionName());
        if (!q.exec()) {
            db.rollback();
            DataManagementError e;
            e.operation = DataManagementOperation::Import;
            e.operationId = operationId;
            e.message = q.lastError().text();
            e.path = csvPath;
            emit importFailed(operationId, e);
            return;
        }
    }

    if (!db.commit()) {
        DataManagementError e;
        e.operation = DataManagementOperation::Import;
        e.operationId = operationId;
        e.message = db.lastError().text();
        e.path = csvPath;
        emit importFailed(operationId, e);
        return;
    }

    ImportResult ir;
    ir.rowsParsed = parse.bars.size();
    ir.rowsUpserted = parse.bars.size();
    emit importFinished(operationId, ir);
}

void DataManagementWorker::doExport(quint64 operationId, QString dbPath,
                                    QList<HistoricalBarsDatasetKey> keys, QString targetDir,
                                    int overwritePolicyInt)
{
    const auto policy = static_cast<ExportOverwritePolicy>(overwritePolicyInt);

    QString err;
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation = DataManagementOperation::Export;
        e.operationId = operationId;
        e.message = err;
        e.path = dbPath;
        emit exportFailed(operationId, e);
        return;
    }

    if (policy == ExportOverwritePolicy::Cancel) {
        ExportBatchSummary sum;
        sum.operationId = operationId;
        emit exportFinished(operationId, sum);
        return;
    }

    ExportBatchSummary sum;
    sum.operationId = operationId;
    QDir dir(targetDir);
    for (const HistoricalBarsDatasetKey& k : keys) {
        const QString base = buildExportBasename(k.symbol, k.resolution, k.dataSourceId);
        const QString path = dir.filePath(base);

        ExportFileResult fr;
        fr.key = k;
        fr.path = path;

        const bool existed = QFile::exists(path);
        if (existed && policy == ExportOverwritePolicy::Skip) {
            fr.ok = true;
            fr.action = ExportFileAction::Skipped;
            sum.files.append(fr);
            continue;
        }

        QString werr;
        if (!exportDatasetToFile(m_repo.connectionName(), k, path, &werr)) {
            fr.ok = false;
            fr.action = ExportFileAction::Failed;
            fr.errorMessage = werr;
            sum.files.append(fr);
            continue;
        }

        fr.ok = true;
        if (existed && policy == ExportOverwritePolicy::Overwrite)
            fr.action = ExportFileAction::Overwritten;
        else
            fr.action = ExportFileAction::Written;
        sum.files.append(fr);
    }

    emit exportFinished(operationId, sum);
}

void DataManagementWorker::doSyncCoverage(quint64 operationId, QString dbPath,
                                            HistoricalBarsDatasetKey key,
                                            QDateTime requestedFromUtc, QDateTime requestedToUtc)
{
    QString err;
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation   = DataManagementOperation::CoverageSync;
        e.operationId = operationId;
        e.message     = err;
        e.path        = dbPath;
        e.keyValid    = true;
        e.key         = key;
        emit coverageSyncFailed(operationId, e);
        return;
    }

    ensureNetworkManager();

    auto provider = HistoricalBarsCoverageSyncRegistry::makeProviderForKey(
        key, m_networkManager, 60000, m_brokerDataProvider);
    if (!provider) {
        DataManagementError e;
        e.operation   = DataManagementOperation::CoverageSync;
        e.operationId = operationId;
        e.message =
            QStringLiteral("Coverage sync is not available for this dataSourceId.");
        e.keyValid = true;
        e.key      = key;
        e.path     = dbPath;
        emit coverageSyncFailed(operationId, e);
        return;
    }

    QString syncErr;
    auto    opt = provider->syncCoverage(key, requestedFromUtc, requestedToUtc, m_repo, &syncErr);
    if (!opt) {
        DataManagementError e;
        e.operation   = DataManagementOperation::CoverageSync;
        e.operationId = operationId;
        e.message     = syncErr.isEmpty() ? QStringLiteral("Coverage sync failed.") : syncErr;
        e.keyValid    = true;
        e.key         = key;
        e.path        = dbPath;
        emit coverageSyncFailed(operationId, e);
        return;
    }

    emit coverageSyncFinished(operationId, *opt);
}

void DataManagementWorker::doBatchYahooImport(quint64 operationId, QString dbPath,
                                              QStringList symbols, QDateTime requestedFromUtc,
                                              QDateTime requestedToUtc,
                                              QString resolution, QString dataSourceId)
{
    QString err;
    if (!m_repo.ensureOpen(dbPath, &err)) {
        DataManagementError e;
        e.operation   = DataManagementOperation::YahooBatchImport;
        e.operationId = operationId;
        e.message     = err;
        e.path        = dbPath;
        emit yahooBatchImportFailed(operationId, e);
        return;
    }

    ensureNetworkManager();

    HistoricalBarsDatasetKey templateKey;
    templateKey.symbol =
        symbols.isEmpty() ? QStringLiteral("_") : symbols.first();
    templateKey.resolution   = resolution;
    templateKey.dataSourceId = dataSourceId;

    auto provider = HistoricalBarsCoverageSyncRegistry::makeProviderForKey(
        templateKey, m_networkManager, 60000, m_brokerDataProvider);
    if (!provider) {
        DataManagementError e;
        e.operation   = DataManagementOperation::YahooBatchImport;
        e.operationId = operationId;
        e.message =
            QStringLiteral("Batch import is not available for this dataSourceId.");
        e.path = dbPath;
        emit yahooBatchImportFailed(operationId, e);
        return;
    }

    BatchYahooImportResult agg;
    agg.symbolsRequested = symbols.size();
    QString                syncErr;

    for (const QString& sym : symbols) {
        HistoricalBarsDatasetKey k;
        k.symbol       = sym;
        k.resolution   = resolution;
        k.dataSourceId = dataSourceId;

        auto opt = provider->syncCoverage(k, requestedFromUtc, requestedToUtc, m_repo, &syncErr);
        if (!opt) {
            agg.failedSymbols.append(sym);
            agg.failedReasons.append(syncErr.isEmpty() ? QStringLiteral("Provider coverage sync failed.")
                                                       : syncErr);
            continue;
        }
        ++agg.symbolsSucceeded;
        agg.totalBarsFetched += opt->barsFetched;
        agg.totalRowsUpserted += opt->rowsUpserted;
    }

    emit yahooBatchImportFinished(operationId, agg);
}

} // namespace DataManagement
