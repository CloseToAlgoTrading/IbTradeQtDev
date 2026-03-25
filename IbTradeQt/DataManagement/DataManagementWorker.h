#ifndef DATAMANAGEMENT_DATAMANAGEMENTWORKER_H
#define DATAMANAGEMENT_DATAMANAGEMENTWORKER_H

#include "DataManagement/BacktestMarketDataRepository.h"
#include "DataManagement/DataManagementTypes.h"
#include <QObject>
#include <QList>

class QNetworkAccessManager;

namespace DataManagement {

/// Lives on worker thread only. Owns BacktestMarketDataRepository and SQLite connection.
class DataManagementWorker : public QObject {
    Q_OBJECT
public:
    explicit DataManagementWorker(QObject* parent = nullptr);

public slots:
    void doInventory(quint64 operationId, QString dbPath);
    void doDelete(quint64 operationId, QString dbPath, QList<HistoricalBarsDatasetKey> keys);
    void doPreview(quint64 operationId, QString dbPath, HistoricalBarsDatasetKey key, int firstN,
                   int lastN);
    void doImport(quint64 operationId, QString dbPath, QString csvPath, QString resolution,
                  QString normalizedDataSourceId);
    void doExport(quint64 operationId, QString dbPath, QList<HistoricalBarsDatasetKey> keys,
                  QString targetDir, int overwritePolicyInt);
    void doSyncCoverage(quint64 operationId, QString dbPath, HistoricalBarsDatasetKey key,
                        QDateTime requestedFromUtc, QDateTime requestedToUtc);
    void doBatchYahooImport(quint64 operationId, QString dbPath, QStringList symbols,
                            QDateTime requestedFromUtc, QDateTime requestedToUtc);

signals:
    void inventoryFinished(quint64 operationId, QList<DataManagement::HistoricalBarsDataset> rows);
    void inventoryFailed(quint64 operationId, DataManagement::DataManagementError err);

    void deleteFinished(quint64 operationId);
    void deleteFailed(quint64 operationId, DataManagement::DataManagementError err);

    void previewFinished(quint64 operationId, DataManagement::HistoricalBarsPreview preview);
    void previewFailed(quint64 operationId, DataManagement::DataManagementError err);

    void importFinished(quint64 operationId, DataManagement::ImportResult result);
    void importFailed(quint64 operationId, DataManagement::DataManagementError err);

    void exportFinished(quint64 operationId, DataManagement::ExportBatchSummary summary);
    void exportFailed(quint64 operationId, DataManagement::DataManagementError err);

    void coverageSyncFinished(quint64 operationId, DataManagement::SyncCoverageResult result);
    void coverageSyncFailed(quint64 operationId, DataManagement::DataManagementError err);

    void yahooBatchImportFinished(quint64 operationId, DataManagement::BatchYahooImportResult result);
    void yahooBatchImportFailed(quint64 operationId, DataManagement::DataManagementError err);

private:
    void ensureNetworkManager();

    BacktestMarketDataRepository m_repo;
    QNetworkAccessManager*       m_networkManager = nullptr;
};

} // namespace DataManagement

#endif
