#include "StrategyCatalogModel.h"

namespace StrategyMgmt {

static QString kindToString(int kind)
{
    switch (kind) {
    case 5:  return QStringLiteral("Pipeline");
    case 1:  return QStringLiteral("Basic Test");
    case 2:  return QStringLiteral("MA");
    case 3:  return QStringLiteral("Momentum");
    default: return QStringLiteral("Strategy");
    }
}

StrategyCatalogModel::StrategyCatalogModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

void StrategyCatalogModel::resetData(const QJsonArray& catalogEntries,
                                      const QMap<QString, int>& versionCounts)
{
    beginResetModel();
    m_entries.clear();
    m_entries.reserve(catalogEntries.size());

    for (const auto& val : catalogEntries) {
        QJsonObject obj = val.toObject();
        Entry e;
        e.strategyId     = obj.value("strategyId").toString();
        e.name           = obj.value("name").toString();
        e.strategyKind   = obj.value("strategyKind").toInt();
        e.stateLabel = obj.value(QStringLiteral("derivedStateLabel")).toString();
        if (e.stateLabel.isEmpty())
            e.stateLabel = obj.value(QStringLiteral("lifecycleSummary"))
                               .toObject()
                               .value(QStringLiteral("stateLabel"))
                               .toString();
        if (e.stateLabel.isEmpty()) {
            e.stateLabel = obj.value("isArchived").toBool()
                               ? QStringLiteral("Retired")
                               : obj.value("lifecycleState").toString();
        }
        e.updatedAt      = obj.value("updatedAt").toString();
        e.versionCount   = versionCounts.value(e.strategyId, 0);
        e.raw            = obj;
        m_entries.append(e);
    }

    endResetModel();
}

QJsonObject StrategyCatalogModel::entryAt(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    return m_entries.at(index.row()).raw;
}

QModelIndex StrategyCatalogModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() || row < 0 || row >= m_entries.size()
        || column < 0 || column >= ColumnCount)
        return {};
    return createIndex(row, column);
}

QModelIndex StrategyCatalogModel::parent(const QModelIndex&) const
{
    return {};
}

int StrategyCatalogModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

int StrategyCatalogModel::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

QVariant StrategyCatalogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const Entry& e = m_entries.at(index.row());

    if (role == StrategyIdRole)
        return e.strategyId;

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:        return e.name;
        case ColKind:        return kindToString(e.strategyKind);
        case ColStatus:      return e.stateLabel;
        case ColVersions:    return e.versionCount;
        case ColLastUpdated: return e.updatedAt;
        }
    }

    return {};
}

QVariant StrategyCatalogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColName:        return QStringLiteral("Name");
    case ColKind:        return QStringLiteral("Kind");
    case ColStatus:      return QStringLiteral("State");
    case ColVersions:    return QStringLiteral("Versions");
    case ColLastUpdated: return QStringLiteral("Last Updated");
    }
    return {};
}

} // namespace StrategyMgmt
