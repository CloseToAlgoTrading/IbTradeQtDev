#include "DataManagement/BacktestMarketDataRepository.h"
#include "DB/dbquery.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtGlobal>

namespace DataManagement {

BacktestMarketDataRepository::BacktestMarketDataRepository(QString connectionName)
    : m_connectionName(std::move(connectionName))
{}

void BacktestMarketDataRepository::close()
{
    if (!QSqlDatabase::contains(m_connectionName))
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_sqlitePath.clear();
}

bool BacktestMarketDataRepository::isOpen() const
{
    if (!QSqlDatabase::contains(m_connectionName))
        return false;
    return QSqlDatabase::database(m_connectionName).isOpen();
}

bool BacktestMarketDataRepository::ensureOpen(const QString& sqlitePath, QString* errorOut)
{
    if (m_sqlitePath == sqlitePath && isOpen())
        return true;

    close();

    QFileInfo fi(sqlitePath);
    QDir dir = fi.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            if (errorOut)
                *errorOut = QStringLiteral("Cannot create directory: %1").arg(dir.absolutePath());
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(QFileInfo(sqlitePath).absoluteFilePath());
    if (!db.open()) {
        if (errorOut)
            *errorOut = QStringLiteral("Cannot open database: %1").arg(db.lastError().text());
        return false;
    }

    QSqlQuery q(db);
    if (!q.exec(QLatin1String(CREATE_TABLE_HISTORICAL_BARS))) {
        if (errorOut)
            *errorOut = QStringLiteral("Cannot create HistoricalBars: %1").arg(q.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase(m_connectionName);
        return false;
    }

    m_sqlitePath = sqlitePath;
    return true;
}

QList<HistoricalBarsDataset> BacktestMarketDataRepository::loadInventory(QString* errorOut)
{
    QList<HistoricalBarsDataset> out;
    if (!isOpen()) {
        if (errorOut)
            *errorOut = QStringLiteral("Database not open");
        return out;
    }

    auto q = query_historicalBarsInventory(m_connectionName);
    if (!q.exec()) {
        if (errorOut)
            *errorOut = q.lastError().text();
        return out;
    }

    while (q.next()) {
        HistoricalBarsDataset row;
        row.symbol = q.value(0).toString();
        row.resolution = q.value(1).toString();
        row.dataSourceId = q.value(2).toString();
        row.rowCount = q.value(3).toLongLong();
        const QString minTs = q.value(4).toString();
        const QString maxTs = q.value(5).toString();
        row.fromUtc = QDateTime::fromString(minTs, Qt::ISODate).toUTC();
        row.toUtc = QDateTime::fromString(maxTs, Qt::ISODate).toUTC();
        out.append(row);
    }
    return out;
}

bool BacktestMarketDataRepository::deleteDatasets(const QList<HistoricalBarsDatasetKey>& keys,
                                                    QString* errorOut)
{
    if (!isOpen()) {
        if (errorOut)
            *errorOut = QStringLiteral("Database not open");
        return false;
    }
    if (keys.isEmpty())
        return true;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        if (errorOut)
            *errorOut = db.lastError().text();
        return false;
    }

    for (const HistoricalBarsDatasetKey& k : keys) {
        auto q = query_deleteHistoricalBarsDataset(k.symbol, k.resolution, k.dataSourceId,
                                                    m_connectionName);
        if (!q.exec()) {
            if (errorOut)
                *errorOut = q.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        if (errorOut)
            *errorOut = db.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

static HistoricalBarPreviewRow rowFromQuery(const QSqlQuery& q)
{
    HistoricalBarPreviewRow r;
    const QString ts = q.value(0).toString();
    r.timestampUtc = QDateTime::fromString(ts, Qt::ISODate).toUTC();
    if (!r.timestampUtc.isValid())
        r.timestampUtc = QDateTime::fromString(ts, Qt::ISODateWithMs).toUTC();
    r.open = q.value(1).toDouble();
    r.high = q.value(2).toDouble();
    r.low = q.value(3).toDouble();
    r.close = q.value(4).toDouble();
    r.volume = q.value(5).toDouble();
    return r;
}

bool BacktestMarketDataRepository::loadPreview(const HistoricalBarsDatasetKey& key, int firstN,
                                                int lastN, HistoricalBarsPreview* out,
                                                QString* errorOut)
{
    if (!out) {
        if (errorOut)
            *errorOut = QStringLiteral("null out");
        return false;
    }
    out->key = key;
    out->firstRowsAsc.clear();
    out->lastRowsAsc.clear();

    if (!isOpen()) {
        if (errorOut)
            *errorOut = QStringLiteral("Database not open");
        return false;
    }

    if (firstN > 0) {
        auto q = query_historicalBarsPreviewFirst(key.symbol, key.resolution, key.dataSourceId,
                                                  firstN, m_connectionName);
        if (!q.exec()) {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
        while (q.next())
            out->firstRowsAsc.append(rowFromQuery(q));
    }

    if (lastN > 0) {
        auto q = query_historicalBarsPreviewLast(key.symbol, key.resolution, key.dataSourceId,
                                                 lastN, m_connectionName);
        if (!q.exec()) {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
        QVector<HistoricalBarPreviewRow> desc;
        desc.reserve(lastN);
        while (q.next())
            desc.append(rowFromQuery(q));
        for (int i = desc.size() - 1; i >= 0; --i)
            out->lastRowsAsc.append(desc.at(i));
    }

    return true;
}

} // namespace DataManagement
