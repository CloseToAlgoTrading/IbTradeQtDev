#include "DataManagement/YahooHistoricalBarsCoverageSyncProvider.h"
#include "DataManagement/BacktestMarketDataRepository.h"
#include "Backtest/IHistoricalDataSource.h"
#include "Backtest/YahooFinanceDataSource.h"
#include "DB/dbquery.h"
#include "DB/dbdatatypes.h"
#include "IBComm/HistoricalDataRouter.h"

#include <QDate>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QSqlDatabase>
#include <QTimer>
#include <QVector>

using Backtest::BarResolution;
using Backtest::IHistoricalDataSource;
using Backtest::YahooFinanceDataSource;

namespace DataManagement {

namespace {

bool isIncompleteBar(const IBComm::HistoricalBar& bar, const QString& resolution)
{
    if (resolution == QLatin1String("Day1"))
        return bar.timestamp.toUTC().date() >= QDate::currentDate();
    return false;
}

void fillResultingRangeFromDb(SyncCoverageResult* r, const HistoricalBarsDatasetKey& key,
                              const QString& dbConn)
{
    if (!r)
        return;
    auto q = query_cachedBarRange(key.symbol, key.resolution, key.dataSourceId, dbConn);
    if (!q.exec() || !q.next())
        return;
    const QString minTs = q.value(QStringLiteral("minTs")).toString();
    const QString maxTs = q.value(QStringLiteral("maxTs")).toString();
    if (minTs.isEmpty() || maxTs.isEmpty())
        return;
    r->resultingMinUtc = QDateTime::fromString(minTs, Qt::ISODate).toUTC();
    if (!r->resultingMinUtc.isValid())
        r->resultingMinUtc = QDateTime::fromString(minTs, Qt::ISODateWithMs).toUTC();
    r->resultingMaxUtc = QDateTime::fromString(maxTs, Qt::ISODate).toUTC();
    if (!r->resultingMaxUtc.isValid())
        r->resultingMaxUtc = QDateTime::fromString(maxTs, Qt::ISODateWithMs).toUTC();
    r->hasResultingRange = r->resultingMinUtc.isValid() && r->resultingMaxUtc.isValid();
}

/// Returns bars on success. On Yahoo `loadFailed` or empty outcome with error, sets \a outError.
QVector<IBComm::HistoricalBar> fetchBatchFromYahoo(QNetworkAccessManager* nam, const QString& dbConn,
                                                   const QStringList& symbols, const QDateTime& from,
                                                   const QDateTime& to, int timeoutMs, QString* outError)
{
    QVector<IBComm::HistoricalBar> batchBars;
    if (outError)
        outError->clear();

    YahooFinanceDataSource source;
    source.setNetworkManager(nam);
    source.setInstrumentMetadataDbConnection(dbConn);

    QEventLoop loop;
    bool         loadFailed = false;
    QString      loadFailReason;
    QObject::connect(&source, &IHistoricalDataSource::loadFailed,
                     [&](const QString& reason) {
                         loadFailed     = true;
                         loadFailReason = reason;
                         loop.quit();
                     });
    QObject::connect(&source, &IHistoricalDataSource::loadFinished, &loop, &QEventLoop::quit);
    QObject::connect(&source, &IHistoricalDataSource::barLoaded,
                     [&batchBars](const IBComm::HistoricalBar& bar) { batchBars.append(bar); });

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMs);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();

    source.requestBars(symbols, from, to, BarResolution::Day1);
    loop.exec();

    source.disconnectFinishedHandler();

    if (loadFailed) {
        if (outError)
            *outError = loadFailReason.isEmpty() ? QStringLiteral("Yahoo Finance request failed.")
                                                 : loadFailReason;
        return {};
    }
    return batchBars;
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
        dbBar.symbol       = bar.symbol;
        dbBar.resolution   = resolution;
        dbBar.dataSourceId = dataSourceId;
        dbBar.timestamp    = bar.timestamp.toUTC().toString(Qt::ISODate);
        dbBar.open         = bar.open;
        dbBar.high         = bar.high;
        dbBar.low          = bar.low;
        dbBar.close        = bar.close;
        dbBar.volume       = bar.volume;

        auto q = query_upsertHistoricalBar(dbBar, dbConnectionName);
        if (q.exec())
            ++upserted;
    }
    db.commit();
    return upserted;
}

} // namespace

YahooHistoricalBarsCoverageSyncProvider::YahooHistoricalBarsCoverageSyncProvider(
    QNetworkAccessManager* networkManager, int yahooFetchTimeoutMs)
    : m_networkManager(networkManager)
    , m_yahooTimeoutMs(yahooFetchTimeoutMs)
{}

QString YahooHistoricalBarsCoverageSyncProvider::providerId() const
{
    return QStringLiteral("yahoo");
}

bool YahooHistoricalBarsCoverageSyncProvider::canSync(const HistoricalBarsDatasetKey& key) const
{
    return key.dataSourceId == QLatin1String("yahoo") && key.resolution == QLatin1String("Day1");
}

std::optional<SyncCoverageResult> YahooHistoricalBarsCoverageSyncProvider::syncCoverage(
    const HistoricalBarsDatasetKey& key, const QDateTime& requestedFromUtc,
    const QDateTime& requestedToUtc, BacktestMarketDataRepository& repo, QString* errorMessage)
{
    if (!errorMessage)
        return std::nullopt;
    errorMessage->clear();

    if (!canSync(key)) {
        *errorMessage =
            QStringLiteral("Yahoo coverage sync requires dataSourceId \"yahoo\" and resolution Day1.");
        return std::nullopt;
    }
    if (!m_networkManager) {
        *errorMessage = QStringLiteral("Network manager not available.");
        return std::nullopt;
    }

    const QString dbConn = repo.connectionName();

    YahooHistoricalBarsCoveragePlanner::Plan plan =
        m_planner.plan(key, requestedFromUtc, requestedToUtc, dbConn);

    SyncCoverageResult out;
    out.key                        = key;
    out.requestedFromUtc           = requestedFromUtc.toUTC();
    out.requestedToUtc             = requestedToUtc.toUTC();
    out.effectiveRequestedFromUtc  = plan.effectiveRequestedFromUtc;
    out.effectiveRequestedToUtc    = plan.effectiveRequestedToUtc;
    out.previousMinUtc             = plan.previousMinUtc;
    out.previousMaxUtc             = plan.previousMaxUtc;
    out.hasPreviousRange           = plan.hasPreviousRange;
    out.segments                   = plan.segments;
    out.warnings                   = plan.warnings;
    out.barsFetched                = 0;
    out.rowsUpserted               = 0;

    const QStringList syms{key.symbol};

    if (!plan.segments.isEmpty()) {
        qint64 totalBars   = 0;
        qint64 totalUpsert = 0;
        for (const SyncCoverageSegment& seg : plan.segments) {
            QString fetchErr;
            QVector<IBComm::HistoricalBar> fetched =
                fetchBatchFromYahoo(m_networkManager, dbConn, syms, seg.fromUtc, seg.toUtc,
                                    m_yahooTimeoutMs, &fetchErr);
            if (!fetchErr.isEmpty()) {
                *errorMessage = fetchErr;
                return std::nullopt;
            }
            totalBars += fetched.size();
            totalUpsert += insertBarsIntoCache(fetched, key.resolution, key.dataSourceId, dbConn);
        }
        out.barsFetched  = totalBars;
        out.rowsUpserted = totalUpsert;
    }

    fillResultingRangeFromDb(&out, key, dbConn);
    return out;
}

} // namespace DataManagement
