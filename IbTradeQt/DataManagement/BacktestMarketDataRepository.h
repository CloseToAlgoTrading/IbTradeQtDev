#ifndef DATAMANAGEMENT_BACKTESTMARKETDATAREPOSITORY_H
#define DATAMANAGEMENT_BACKTESTMARKETDATAREPOSITORY_H

#include "DataManagement/DataManagementTypes.h"
#include <QString>
#include <QList>

namespace DataManagement {

/// SQLite HistoricalBars on the backtest DB path. Use only from the worker thread
/// that owns \a connectionName.
class BacktestMarketDataRepository {
public:
    explicit BacktestMarketDataRepository(QString connectionName);

    /// Open (or create) DB file, ensure HistoricalBars table exists.
    bool ensureOpen(const QString& sqlitePath, QString* errorOut = nullptr);
    void close();

    bool isOpen() const;

    QList<HistoricalBarsDataset> loadInventory(QString* errorOut = nullptr);

    /// Single transaction; all triples deleted or none.
    bool deleteDatasets(const QList<HistoricalBarsDatasetKey>& keys, QString* errorOut = nullptr);

    bool loadPreview(const HistoricalBarsDatasetKey& key, int firstN, int lastN,
                     HistoricalBarsPreview* out, QString* errorOut = nullptr);

    const QString& connectionName() const { return m_connectionName; }

private:
    QString m_connectionName;
    QString m_sqlitePath;
};

} // namespace DataManagement

#endif
