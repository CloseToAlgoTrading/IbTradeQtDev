#include "DataManagement/DataManagementService.h"
#include "DataManagement/DataManagementWorker.h"

#include <QDateTime>
#include <QMetaObject>
#include <QThread>
#include <QTimer>

namespace DataManagement {

DataManagementService::DataManagementService(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<HistoricalBarsDatasetKey>();
    qRegisterMetaType<QList<HistoricalBarsDataset>>();
    qRegisterMetaType<QList<HistoricalBarsDatasetKey>>();
    qRegisterMetaType<DataManagementError>();
    qRegisterMetaType<HistoricalBarsPreview>();
    qRegisterMetaType<ExportBatchSummary>();
    qRegisterMetaType<ImportResult>();
    qRegisterMetaType<SyncCoverageResult>();
    qRegisterMetaType<BatchYahooImportResult>();

    m_thread = new QThread(this);
    m_worker = new DataManagementWorker();
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    wireWorker();
    m_thread->start();
}

DataManagementService::~DataManagementService()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait(5000);
    }
    m_worker = nullptr;
}

void DataManagementService::wireWorker()
{
    connect(m_worker, &DataManagementWorker::inventoryFinished, this,
            &DataManagementService::barsInventoryLoaded, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::inventoryFailed, this,
            &DataManagementService::barsInventoryFailed, Qt::QueuedConnection);

    connect(m_worker, &DataManagementWorker::deleteFinished, this,
            &DataManagementService::barsDeleteFinished, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::deleteFailed, this,
            &DataManagementService::barsDeleteFailed, Qt::QueuedConnection);

    connect(m_worker, &DataManagementWorker::previewFinished, this,
            &DataManagementService::barsPreviewReady, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::previewFailed, this,
            &DataManagementService::barsPreviewFailed, Qt::QueuedConnection);

    connect(m_worker, &DataManagementWorker::importFinished, this,
            &DataManagementService::barsImportFinished, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::importFailed, this,
            &DataManagementService::barsImportFailed, Qt::QueuedConnection);

    connect(m_worker, &DataManagementWorker::exportFinished, this,
            &DataManagementService::barsExportFinished, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::exportFailed, this,
            &DataManagementService::barsExportFailed, Qt::QueuedConnection);

    connect(m_worker, &DataManagementWorker::coverageSyncFinished, this,
            &DataManagementService::barsCoverageSyncFinished, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::coverageSyncFailed, this,
            &DataManagementService::barsCoverageSyncFailed, Qt::QueuedConnection);

    connect(m_worker, &DataManagementWorker::yahooBatchImportFinished, this,
            &DataManagementService::barsYahooBatchImportFinished, Qt::QueuedConnection);
    connect(m_worker, &DataManagementWorker::yahooBatchImportFailed, this,
            &DataManagementService::barsYahooBatchImportFailed, Qt::QueuedConnection);
}

quint64 DataManagementService::requestBarsInventory(const QString& backtestDbPath)
{
    const quint64 id = ++m_nextOperationId;
    QMetaObject::invokeMethod(
        m_worker, "doInventory", Qt::QueuedConnection, Q_ARG(quint64, id),
        Q_ARG(QString, backtestDbPath));
    return id;
}

quint64 DataManagementService::requestDeleteDatasets(const QString& backtestDbPath,
                                                      const QList<HistoricalBarsDatasetKey>& keys)
{
    const quint64 id = ++m_nextOperationId;
    QMetaObject::invokeMethod(m_worker, "doDelete", Qt::QueuedConnection, Q_ARG(quint64, id),
                              Q_ARG(QString, backtestDbPath),
                              Q_ARG(QList<HistoricalBarsDatasetKey>, keys));
    return id;
}

quint64 DataManagementService::requestPreview(const QString& backtestDbPath,
                                              const HistoricalBarsDatasetKey& key, int firstN,
                                              int lastN)
{
    const quint64 id = ++m_nextOperationId;
    QMetaObject::invokeMethod(m_worker, "doPreview", Qt::QueuedConnection, Q_ARG(quint64, id),
                              Q_ARG(QString, backtestDbPath),
                              Q_ARG(DataManagement::HistoricalBarsDatasetKey, key),
                              Q_ARG(int, firstN), Q_ARG(int, lastN));
    return id;
}

quint64 DataManagementService::requestImport(const QString& backtestDbPath, const QString& csvPath,
                                             const QString& resolution,
                                             const QString& normalizedDataSourceId)
{
    const quint64 id = ++m_nextOperationId;
    QMetaObject::invokeMethod(m_worker, "doImport", Qt::QueuedConnection, Q_ARG(quint64, id),
                              Q_ARG(QString, backtestDbPath), Q_ARG(QString, csvPath),
                              Q_ARG(QString, resolution), Q_ARG(QString, normalizedDataSourceId));
    return id;
}

quint64 DataManagementService::requestExport(const QString& backtestDbPath,
                                             const QList<HistoricalBarsDatasetKey>& keys,
                                             const QString& targetDir,
                                             ExportOverwritePolicy policy)
{
    const quint64 id = ++m_nextOperationId;
    QMetaObject::invokeMethod(
        m_worker, "doExport", Qt::QueuedConnection, Q_ARG(quint64, id), Q_ARG(QString, backtestDbPath),
        Q_ARG(QList<HistoricalBarsDatasetKey>, keys), Q_ARG(QString, targetDir),
        Q_ARG(int, static_cast<int>(policy)));
    return id;
}

quint64 DataManagementService::requestSyncBarsCoverage(const QString& backtestDbPath,
                                                       const HistoricalBarsDatasetKey& key,
                                                       const QDateTime& requestedFromUtc,
                                                       const QDateTime& requestedToUtc)
{
    const quint64 id = ++m_nextOperationId;

    const QDateTime fromUtc = requestedFromUtc.toUTC();
    const QDateTime toUtc   = requestedToUtc.toUTC();

    if (!requestedFromUtc.isValid() || !requestedToUtc.isValid() || !fromUtc.isValid() || !toUtc.isValid()) {
        DataManagementError e;
        e.operation   = DataManagementOperation::CoverageSync;
        e.operationId = id;
        e.message     = QStringLiteral("Requested range must use valid date-times.");
        e.keyValid    = true;
        e.key         = key;
        e.path        = backtestDbPath;
        emit barsCoverageSyncFailed(id, e);
        return id;
    }
    if (fromUtc >= toUtc) {
        DataManagementError e;
        e.operation   = DataManagementOperation::CoverageSync;
        e.operationId = id;
        e.message     = QStringLiteral("Requested coverage range is invalid (from must be before to).");
        e.keyValid    = true;
        e.key         = key;
        e.path        = backtestDbPath;
        emit barsCoverageSyncFailed(id, e);
        return id;
    }

    QMetaObject::invokeMethod(m_worker, "doSyncCoverage", Qt::QueuedConnection, Q_ARG(quint64, id),
                              Q_ARG(QString, backtestDbPath),
                              Q_ARG(DataManagement::HistoricalBarsDatasetKey, key),
                              Q_ARG(QDateTime, fromUtc), Q_ARG(QDateTime, toUtc));
    return id;
}

quint64 DataManagementService::requestBatchYahooImport(const QString& backtestDbPath,
                                                     const QStringList& symbols,
                                                     const QDateTime& requestedFromUtc,
                                                     const QDateTime& requestedToUtc)
{
    const quint64 id = ++m_nextOperationId;

    const QDateTime fromUtc = requestedFromUtc.toUTC();
    const QDateTime toUtc   = requestedToUtc.toUTC();

    if (symbols.isEmpty()) {
        DataManagementError e;
        e.operation   = DataManagementOperation::YahooBatchImport;
        e.operationId = id;
        e.message     = QStringLiteral("Enter at least one symbol (comma-separated).");
        e.path        = backtestDbPath;
        QTimer::singleShot(0, this, [this, id, e]() { emit barsYahooBatchImportFailed(id, e); });
        return id;
    }
    if (!requestedFromUtc.isValid() || !requestedToUtc.isValid() || !fromUtc.isValid() || !toUtc.isValid()) {
        DataManagementError e;
        e.operation   = DataManagementOperation::YahooBatchImport;
        e.operationId = id;
        e.message     = QStringLiteral("Requested range must use valid date-times.");
        e.path        = backtestDbPath;
        QTimer::singleShot(0, this, [this, id, e]() { emit barsYahooBatchImportFailed(id, e); });
        return id;
    }
    if (fromUtc >= toUtc) {
        DataManagementError e;
        e.operation   = DataManagementOperation::YahooBatchImport;
        e.operationId = id;
        e.message     = QStringLiteral("Requested coverage range is invalid (from must be before to).");
        e.path        = backtestDbPath;
        QTimer::singleShot(0, this, [this, id, e]() { emit barsYahooBatchImportFailed(id, e); });
        return id;
    }

    QMetaObject::invokeMethod(m_worker, "doBatchYahooImport", Qt::QueuedConnection, Q_ARG(quint64, id),
                              Q_ARG(QString, backtestDbPath), Q_ARG(QStringList, symbols),
                              Q_ARG(QDateTime, fromUtc), Q_ARG(QDateTime, toUtc));
    return id;
}

} // namespace DataManagement
