#include "DataManagementUI/DatasetTableModel.h"

#include <QLocale>

namespace DataManagementUI {

DatasetTableModel::DatasetTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

int DatasetTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

int DatasetTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ColCount;
}

QVariant DatasetTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const auto& r = m_rows.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColSymbol:
            return r.symbol;
        case ColResolution:
            return r.resolution;
        case ColDataSource:
            return r.dataSourceId;
        case ColRowCount:
            return QLocale().toString(r.rowCount);
        case ColFrom:
            return r.fromUtc.isValid() ? r.fromUtc.toString(Qt::ISODate) : QString();
        case ColTo:
            return r.toUtc.isValid() ? r.toUtc.toString(Qt::ISODate) : QString();
        default:
            break;
        }
    }
    return {};
}

QVariant DatasetTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColSymbol:
        return QStringLiteral("Symbol");
    case ColResolution:
        return QStringLiteral("Resolution");
    case ColDataSource:
        return QStringLiteral("Source");
    case ColRowCount:
        return QStringLiteral("Rows");
    case ColFrom:
        return QStringLiteral("From (UTC)");
    case ColTo:
        return QStringLiteral("To (UTC)");
    default:
        return {};
    }
}

void DatasetTableModel::setDatasets(const QVector<DataManagement::HistoricalBarsDataset>& rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

DataManagement::HistoricalBarsDataset DatasetTableModel::datasetAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_rows.at(row);
}

DataManagement::HistoricalBarsDatasetKey DatasetTableModel::keyAt(int row) const
{
    DataManagement::HistoricalBarsDatasetKey k;
    if (row < 0 || row >= m_rows.size())
        return k;
    const auto& r = m_rows.at(row);
    k.symbol = r.symbol;
    k.resolution = r.resolution;
    k.dataSourceId = r.dataSourceId;
    return k;
}

} // namespace DataManagementUI
