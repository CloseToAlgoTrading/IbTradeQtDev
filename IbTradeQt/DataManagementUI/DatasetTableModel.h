#ifndef DATAMANAGEMENTUI_DATASETTABLEMODEL_H
#define DATAMANAGEMENTUI_DATASETTABLEMODEL_H

#include "DataManagement/DataManagementTypes.h"
#include <QAbstractTableModel>
#include <QVector>

namespace DataManagementUI {

class DatasetTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColSymbol = 0, ColResolution, ColDataSource, ColRowCount, ColFrom, ColTo, ColCount };

    explicit DatasetTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setDatasets(const QVector<DataManagement::HistoricalBarsDataset>& rows);
    DataManagement::HistoricalBarsDataset datasetAt(int row) const;
    DataManagement::HistoricalBarsDatasetKey keyAt(int row) const;

private:
    QVector<DataManagement::HistoricalBarsDataset> m_rows;
};

} // namespace DataManagementUI

#endif
