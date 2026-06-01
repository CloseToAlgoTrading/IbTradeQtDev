#ifndef DATAMANAGEMENTCOORDINATOR_H
#define DATAMANAGEMENTCOORDINATOR_H

#include "DataManagement/DataManagementTypes.h"
#include <QObject>

class CIBTradeSystemView;
class CBrokerDataProvider;

namespace DataManagement {
class DataManagementService;
}

namespace DataManagementUI {
class DataManagementPanel;
}

class DataManagementCoordinator : public QObject {
    Q_OBJECT
public:
    explicit DataManagementCoordinator(QObject* parent = nullptr);
    ~DataManagementCoordinator() override;

    void setView(CIBTradeSystemView* view);
    void setPanel(DataManagementUI::DataManagementPanel* panel);
    void setBrokerDataProvider(CBrokerDataProvider* provider);
    void setBrokerConnected(bool connected);

    void wireSignals();
    void refreshContextAndInventory();

private slots:
    void onPanelRefresh();
    void onPanelDelete();
    void onPanelExport();
    void onPanelImport();
    void onPanelPreview();
    void onPanelCoverageSync();
    void onPanelYahooBatch();

    void onInventoryLoaded(quint64 operationId, QList<DataManagement::HistoricalBarsDataset> rows);
    void onInventoryFailed(quint64 operationId, DataManagement::DataManagementError err);

    void onDeleteFinished(quint64 operationId);
    void onDeleteFailed(quint64 operationId, DataManagement::DataManagementError err);

    void onPreviewReady(quint64 operationId, DataManagement::HistoricalBarsPreview preview);
    void onPreviewFailed(quint64 operationId, DataManagement::DataManagementError err);

    void onImportFinished(quint64 operationId, DataManagement::ImportResult result);
    void onImportFailed(quint64 operationId, DataManagement::DataManagementError err);

    void onExportFinished(quint64 operationId, DataManagement::ExportBatchSummary summary);
    void onExportFailed(quint64 operationId, DataManagement::DataManagementError err);

    void onCoverageSyncFinished(quint64 operationId, DataManagement::SyncCoverageResult result);
    void onCoverageSyncFailed(quint64 operationId, DataManagement::DataManagementError err);

    void onYahooBatchImportFinished(quint64 operationId, DataManagement::BatchYahooImportResult result);
    void onYahooBatchImportFailed(quint64 operationId, DataManagement::DataManagementError err);

private:
    QString backtestDbPath() const;
    void requestInventoryAfterMutation();
    QString formatPreview(const DataManagement::HistoricalBarsPreview& p) const;

    CIBTradeSystemView* m_view = nullptr;
    DataManagementUI::DataManagementPanel* m_panel = nullptr;
    DataManagement::DataManagementService* m_service = nullptr;
    bool m_busy = false;
    bool m_brokerConnected = false;

    /// Last-issued operation id per request type; completions with a different id are ignored (stale).
    quint64 m_pendingInventoryOpId = 0;
    quint64 m_pendingDeleteOpId = 0;
    quint64 m_pendingPreviewOpId = 0;
    quint64 m_pendingImportOpId = 0;
    quint64 m_pendingExportOpId = 0;
    quint64 m_pendingCoverageSyncOpId = 0;
    quint64 m_pendingYahooBatchOpId = 0;
};

#endif
