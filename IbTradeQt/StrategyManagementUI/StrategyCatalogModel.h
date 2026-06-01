#ifndef STRATEGYCATALOGMODEL_H
#define STRATEGYCATALOGMODEL_H

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>

namespace StrategyMgmt {

// Flat-list model exposing strategy catalog entries for a QTreeView.
// Columns: Name | Kind | State | Versions | Last Updated
class StrategyCatalogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column {
        ColName = 0,
        ColKind,
        ColStatus,
        ColVersions,
        ColLastUpdated,
        ColumnCount
    };

    enum Role {
        StrategyIdRole = Qt::UserRole + 1,
    };

    explicit StrategyCatalogModel(QObject* parent = nullptr);

    // Replace all rows with fresh data from the backend.
    void resetData(const QJsonArray& catalogEntries,
                   const QMap<QString, int>& versionCounts);

    QJsonObject entryAt(const QModelIndex& index) const;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    struct Entry {
        QString strategyId;
        QString name;
        int     strategyKind = 0;
        QString stateLabel;
        int     versionCount = 0;
        QString updatedAt;
        QJsonObject raw;
    };

    QList<Entry> m_entries;
};

} // namespace StrategyMgmt

#endif // STRATEGYCATALOGMODEL_H
