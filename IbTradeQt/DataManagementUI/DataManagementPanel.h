#ifndef DATAMANAGEMENTUI_DATAMANAGEMENTPANEL_H
#define DATAMANAGEMENTUI_DATAMANAGEMENTPANEL_H

#include "DataManagement/DataManagementTypes.h"
#include <QWidget>
#include <QVector>

class QTableView;
class QLineEdit;
class QComboBox;
class QTextEdit;
class QLabel;
class QDateTimeEdit;
class QPushButton;
class QSortFilterProxyModel;

namespace DataManagementUI {

class DatasetTableModel;

class DataManagementPanel : public QWidget {
    Q_OBJECT
public:
    explicit DataManagementPanel(QWidget* parent = nullptr);

    void setStorageContext(const DataManagement::DataManagementContext& ctx);
    void setDatasets(const QVector<DataManagement::HistoricalBarsDataset>& rows);
    void setPreviewText(const QString& text);
    void setBusy(bool busy);

    /// Selected dataset keys (multi-select).
    QList<DataManagement::HistoricalBarsDatasetKey> selectedKeys() const;

    QString importResolution() const;
    QString importDataSourceIdRaw() const;

    QDateTime coverageSyncFromUtc() const;
    QDateTime coverageSyncToUtc() const;

    /// Comma-separated line (same format as backtest run config).
    QString yahooBatchSymbolsLine() const;

signals:
    void refreshClicked();
    void deleteClicked();
    void exportClicked();
    void importClicked();
    void previewClicked();
    void coverageSyncClicked();
    void yahooBatchFetchClicked();

private:
    void rebuildStorageLabel();
    void updateCoverageSyncAvailability();
    void updateYahooBatchAvailability();

    QLabel* m_storageLabel = nullptr;
    QLineEdit* m_symbolFilter = nullptr;
    QComboBox* m_resolutionFilter = nullptr;
    QLineEdit* m_sourceFilter = nullptr;
    QTableView* m_table = nullptr;
    DatasetTableModel* m_model = nullptr;
    QSortFilterProxyModel* m_proxy = nullptr;
    QTextEdit* m_preview = nullptr;
    QComboBox* m_importResolution = nullptr;
    QLineEdit* m_importDataSource = nullptr;

    QLabel* m_coverageSyncLabel = nullptr;
    QDateTimeEdit* m_syncFrom = nullptr;
    QDateTimeEdit* m_syncTo = nullptr;
    QPushButton* m_syncCoverageButton = nullptr;
    QLabel* m_coverageSyncHint = nullptr;

    QLineEdit* m_yahooBatchSymbolsEdit = nullptr;
    QPushButton* m_yahooBatchFetchButton = nullptr;
    QLabel* m_yahooBatchHint = nullptr;

    DataManagement::HistoricalBarsDatasetKey m_lastCoverageSyncKey;

    DataManagement::DataManagementContext m_ctx;
};

} // namespace DataManagementUI

#endif
