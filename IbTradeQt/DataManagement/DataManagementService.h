#ifndef DATAMANAGEMENT_DATAMANAGEMENTSERVICE_H
#define DATAMANAGEMENT_DATAMANAGEMENTSERVICE_H

#include "DataManagement/DataManagementTypes.h"
#include <QObject>
#include <QList>

class CBrokerDataProvider;

namespace DataManagement {

class DataManagementWorker;

/// GUI-thread facade. Dispatches to DataManagementWorker on a dedicated thread.
class DataManagementService : public QObject {
    Q_OBJECT
public:
    explicit DataManagementService(QObject* parent = nullptr);
    ~DataManagementService() override;

    void setBrokerDataProvider(CBrokerDataProvider* provider);

    quint64 requestBarsInventory(const QString& backtestDbPath);
    quint64 requestDeleteDatasets(const QString& backtestDbPath,
                                  const QList<HistoricalBarsDatasetKey>& keys);
    quint64 requestPreview(const QString& backtestDbPath, const HistoricalBarsDatasetKey& key,
                           int firstN, int lastN);
    quint64 requestImport(const QString& backtestDbPath, const QString& csvPath,
                          const QString& resolution, const QString& normalizedDataSourceId);
    quint64 requestExport(const QString& backtestDbPath,
                          const QList<HistoricalBarsDatasetKey>& keys, const QString& targetDir,
                          ExportOverwritePolicy policy);

    /// Validates UTC range (`from < to`) on the GUI thread before dispatching. Unsupported sources fail in the worker.
    quint64 requestSyncBarsCoverage(const QString& backtestDbPath, const HistoricalBarsDatasetKey& key,
                                    const QDateTime& requestedFromUtc, const QDateTime& requestedToUtc);

    /// Provider-backed batch import; one coverage sync per symbol. Yahoo defaults to Day1.
    quint64 requestBatchYahooImport(const QString& backtestDbPath, const QStringList& symbols,
                                    const QDateTime& requestedFromUtc, const QDateTime& requestedToUtc,
                                    const QString& resolution = {},
                                    const QString& dataSourceId = {});

signals:
    void barsInventoryLoaded(quint64 operationId, QList<DataManagement::HistoricalBarsDataset> rows);
    void barsInventoryFailed(quint64 operationId, DataManagement::DataManagementError err);

    void barsDeleteFinished(quint64 operationId);
    void barsDeleteFailed(quint64 operationId, DataManagement::DataManagementError err);

    void barsPreviewReady(quint64 operationId, DataManagement::HistoricalBarsPreview preview);
    void barsPreviewFailed(quint64 operationId, DataManagement::DataManagementError err);

    void barsImportFinished(quint64 operationId, DataManagement::ImportResult result);
    void barsImportFailed(quint64 operationId, DataManagement::DataManagementError err);

    void barsExportFinished(quint64 operationId, DataManagement::ExportBatchSummary summary);
    void barsExportFailed(quint64 operationId, DataManagement::DataManagementError err);

    void barsCoverageSyncFinished(quint64 operationId, DataManagement::SyncCoverageResult result);
    void barsCoverageSyncFailed(quint64 operationId, DataManagement::DataManagementError err);

    void barsYahooBatchImportFinished(quint64 operationId, DataManagement::BatchYahooImportResult result);
    void barsYahooBatchImportFailed(quint64 operationId, DataManagement::DataManagementError err);

private:
    void wireWorker();

    QThread* m_thread = nullptr;
    DataManagementWorker* m_worker = nullptr;
    quint64 m_nextOperationId = 0;
};

} // namespace DataManagement

#endif
