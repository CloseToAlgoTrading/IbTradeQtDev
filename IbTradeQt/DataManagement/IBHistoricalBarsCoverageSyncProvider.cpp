#include "DataManagement/IBHistoricalBarsCoverageSyncProvider.h"

#include "Adapters/IBHistoricalDataFetcher.h"
#include "DataManagement/BacktestMarketDataRepository.h"
#include "DB/dbdatatypes.h"
#include "DB/dbquery.h"

#include <QDate>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace DataManagement {

namespace {

bool isIncompleteBar(const IBComm::HistoricalBar& bar, const QString& resolution)
{
    if (resolution == QLatin1String("Day1"))
        return bar.timestamp.toUTC().date() >= QDate::currentDate();
    return false;
}

QDateTime parseUtcIso(const QString& value)
{
    QDateTime dt = QDateTime::fromString(value, Qt::ISODate).toUTC();
    if (!dt.isValid())
        dt = QDateTime::fromString(value, Qt::ISODateWithMs).toUTC();
    return dt;
}

void fillRange(SyncCoverageResult* r, const HistoricalBarsDatasetKey& key, const QString& dbConn,
               bool previous)
{
    auto q = query_cachedBarRange(key.symbol, key.resolution, key.dataSourceId, dbConn);
    if (!q.exec() || !q.next())
        return;

    const QDateTime minUtc = parseUtcIso(q.value(QStringLiteral("minTs")).toString());
    const QDateTime maxUtc = parseUtcIso(q.value(QStringLiteral("maxTs")).toString());
    if (!minUtc.isValid() || !maxUtc.isValid())
        return;

    if (previous) {
        r->previousMinUtc = minUtc;
        r->previousMaxUtc = maxUtc;
        r->hasPreviousRange = true;
    } else {
        r->resultingMinUtc = minUtc;
        r->resultingMaxUtc = maxUtc;
        r->hasResultingRange = true;
    }
}

int insertBarsIntoCache(const QVector<IBComm::HistoricalBar>& bars, const QString& resolution,
                        const QString& dataSourceId, const QString& dbConnectionName)
{
    QSqlDatabase db = QSqlDatabase::database(dbConnectionName);
    if (!db.isOpen())
        return 0;

    int upserted = 0;
    db.transaction();
    for (const auto& bar : bars) {
        if (isIncompleteBar(bar, resolution))
            continue;

        DbHistoricalBar dbBar;
        dbBar.symbol = bar.symbol;
        dbBar.resolution = resolution;
        dbBar.dataSourceId = dataSourceId;
        dbBar.timestamp = bar.timestamp.toUTC().toString(Qt::ISODate);
        dbBar.open = bar.open;
        dbBar.high = bar.high;
        dbBar.low = bar.low;
        dbBar.close = bar.close;
        dbBar.volume = bar.volume;

        auto q = query_upsertHistoricalBar(dbBar, dbConnectionName);
        if (q.exec())
            ++upserted;
    }
    db.commit();
    return upserted;
}

} // namespace

IBHistoricalBarsCoverageSyncProvider::IBHistoricalBarsCoverageSyncProvider(
    CBrokerDataProvider* broker, int fetchTimeoutMs)
    : m_broker(broker)
    , m_fetchTimeoutMs(fetchTimeoutMs)
{}

QString IBHistoricalBarsCoverageSyncProvider::providerId() const
{
    return QStringLiteral("ib");
}

bool IBHistoricalBarsCoverageSyncProvider::canSync(const HistoricalBarsDatasetKey& key) const
{
    return key.dataSourceId == QLatin1String("ib");
}

std::optional<SyncCoverageResult> IBHistoricalBarsCoverageSyncProvider::syncCoverage(
    const HistoricalBarsDatasetKey& key, const QDateTime& requestedFromUtc,
    const QDateTime& requestedToUtc, BacktestMarketDataRepository& repo, QString* errorMessage)
{
    if (!errorMessage)
        return std::nullopt;
    errorMessage->clear();

    if (!canSync(key)) {
        *errorMessage = QStringLiteral("IB coverage sync requires dataSourceId \"ib\".");
        return std::nullopt;
    }

    const QString dbConn = repo.connectionName();
    SyncCoverageResult out;
    out.key = key;
    out.requestedFromUtc = requestedFromUtc.toUTC();
    out.requestedToUtc = requestedToUtc.toUTC();
    out.effectiveRequestedFromUtc = out.requestedFromUtc;
    out.effectiveRequestedToUtc = out.requestedToUtc;
    out.segments.append({SyncCoverageSegmentKind::Full, out.requestedFromUtc, out.requestedToUtc});
    fillRange(&out, key, dbConn, true);

    Adapters::IBHistoricalDataFetcher fetcher(m_broker);
    Adapters::IBHistoricalFetchRequest req;
    req.symbols = QStringList{key.symbol};
    req.resolution = key.resolution;
    req.fromUtc = out.requestedFromUtc;
    req.toUtc = out.requestedToUtc;
    req.timeoutMs = m_fetchTimeoutMs;
    auto fetchResult = fetcher.fetch(req);
    if (!fetchResult.errorMessage.isEmpty()) {
        *errorMessage = fetchResult.errorMessage;
        return std::nullopt;
    }

    out.barsFetched = fetchResult.bars.size();
    out.rowsUpserted = insertBarsIntoCache(fetchResult.bars, key.resolution, key.dataSourceId, dbConn);
    fillRange(&out, key, dbConn, false);
    return out;
}

} // namespace DataManagement
