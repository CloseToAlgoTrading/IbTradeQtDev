#include "Backtest/HistoricalDataManager.h"
#include "Backtest/YahooFinanceDataSource.h"
#include "DB/dbquery.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace Backtest {

HistoricalDataManager::HistoricalDataManager(const QString& dbConnectionName,
                                               QNetworkAccessManager* networkManager,
                                               QObject* parent)
    : QObject(parent)
    , m_dbConnectionName(dbConnectionName)
    , m_networkManager(networkManager)
{
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
        m_ownsNetworkManager = true;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

QVector<IBComm::HistoricalBar> HistoricalDataManager::getBars(
    const QString& symbol,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    QString* dataRefreshedAt)
{
    const QString fromUtc = toUtcIso(from);
    const QString toUtc   = toUtcIso(to);

    CachedRange cached = queryCachedRange(symbol, resolution, dataSourceId);

    bool needFetchBefore = false;
    bool needFetchAfter  = false;
    QDateTime fetchFrom  = from;
    QDateTime fetchTo    = to;

    if (!cached.hasData) {
        // No cache at all — fetch the entire range
        needFetchBefore = true;
    } else {
        QDateTime cachedMin = fromUtcIso(cached.minTs);
        QDateTime cachedMax = fromUtcIso(cached.maxTs);

        if (from < cachedMin) needFetchBefore = true;
        if (to   > cachedMax) needFetchAfter  = true;

        // Compute the minimal fetch range covering both gaps
        if (needFetchBefore && needFetchAfter) {
            // Fetch from before cachedMin to after cachedMax
            fetchFrom = from;
            fetchTo   = to;
        } else if (needFetchBefore) {
            fetchFrom = from;
            fetchTo   = cachedMin.addDays(-1);
        } else if (needFetchAfter) {
            fetchFrom = cachedMax.addDays(1);
            fetchTo   = to;
        }
    }

    bool didFetch = false;
    if (needFetchBefore || needFetchAfter) {
        QVector<IBComm::HistoricalBar> fetched =
            fetchAndCache({symbol}, resolution, dataSourceId, fetchFrom, fetchTo);

        if (!fetched.isEmpty()) {
            didFetch = true;
        }
    }

    if (dataRefreshedAt) {
        *dataRefreshedAt = didFetch
            ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
            : QString();
    }

    return readFromCache(symbol, resolution, dataSourceId, fromUtc, toUtc);
}

QMap<QString, QVector<IBComm::HistoricalBar>> HistoricalDataManager::getBarsMulti(
    const QStringList& symbols,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    QString* dataRefreshedAt)
{
    // Determine which symbols need fetching and what range is missing
    QStringList symbolsToFetch;
    QDateTime   fetchFrom = to;   // will be min'd down
    QDateTime   fetchTo   = from; // will be max'd up

    for (const QString& sym : symbols) {
        CachedRange cached = queryCachedRange(sym, resolution, dataSourceId);

        if (!cached.hasData) {
            symbolsToFetch.append(sym);
            fetchFrom = qMin(fetchFrom, from);
            fetchTo   = qMax(fetchTo,   to);
        } else {
            QDateTime cachedMin = fromUtcIso(cached.minTs);
            QDateTime cachedMax = fromUtcIso(cached.maxTs);

            bool needBefore = (from < cachedMin);
            bool needAfter  = (to   > cachedMax);

            if (needBefore || needAfter) {
                symbolsToFetch.append(sym);
                // Expand fetch window to cover all gaps across all symbols
                if (needBefore) fetchFrom = qMin(fetchFrom, from);
                if (needAfter)  fetchTo   = qMax(fetchTo,   to);
            }
        }
    }

    bool didFetch = false;
    if (!symbolsToFetch.isEmpty()) {
        QVector<IBComm::HistoricalBar> fetched =
            fetchAndCache(symbolsToFetch, resolution, dataSourceId, fetchFrom, fetchTo);
        didFetch = !fetched.isEmpty();
    }

    if (dataRefreshedAt) {
        *dataRefreshedAt = didFetch
            ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
            : QString();
    }

    const QString fromUtc = toUtcIso(from);
    const QString toUtc   = toUtcIso(to);

    QMap<QString, QVector<IBComm::HistoricalBar>> result;
    for (const QString& sym : symbols) {
        result[sym] = readFromCache(sym, resolution, dataSourceId, fromUtc, toUtc);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Private implementation
// ---------------------------------------------------------------------------

HistoricalDataManager::CachedRange HistoricalDataManager::queryCachedRange(
    const QString& symbol,
    const QString& resolution,
    const QString& dataSourceId) const
{
    CachedRange r;
    QSqlDatabase db = QSqlDatabase::database(m_dbConnectionName);
    if (!db.isOpen()) return r;

    auto q = query_cachedBarRange(symbol, resolution, dataSourceId, m_dbConnectionName);
    if (q.exec() && q.next()) {
        r.minTs   = q.value("minTs").toString();
        r.maxTs   = q.value("maxTs").toString();
        r.hasData = !r.minTs.isEmpty() && !r.maxTs.isEmpty();
    }
    return r;
}

QVector<IBComm::HistoricalBar> HistoricalDataManager::readFromCache(
    const QString& symbol,
    const QString& resolution,
    const QString& dataSourceId,
    const QString& fromUtc,
    const QString& toUtc) const
{
    QVector<IBComm::HistoricalBar> bars;
    auto q = query_fetchHistoricalBars(symbol, resolution, dataSourceId,
                                        fromUtc, toUtc, m_dbConnectionName);
    if (!q.exec()) {
        qWarning() << "HistoricalDataManager: readFromCache failed:" << q.lastError().text();
        return bars;
    }
    while (q.next()) {
        IBComm::HistoricalBar bar;
        bar.symbol    = symbol;
        bar.timestamp = fromUtcIso(q.value("timestamp").toString());
        bar.open      = q.value("open").toDouble();
        bar.high      = q.value("high").toDouble();
        bar.low       = q.value("low").toDouble();
        bar.close     = q.value("close").toDouble();
        bar.volume    = q.value("volume").toDouble();
        bars.append(bar);
    }
    return bars;
}

QVector<IBComm::HistoricalBar> HistoricalDataManager::fetchAndCache(
    const QStringList& symbols,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to)
{
    QVector<IBComm::HistoricalBar> fetchedBars;

    if (dataSourceId == QLatin1String("yahoo")) {
        YahooFinanceDataSource source;
        source.setNetworkManager(m_networkManager);

        QEventLoop loop;
        QObject::connect(&source, &IHistoricalDataSource::loadFinished,
                         &loop, &QEventLoop::quit);
        QObject::connect(&source, &IHistoricalDataSource::loadFailed,
                         &loop, &QEventLoop::quit);
        QObject::connect(&source, &IHistoricalDataSource::barLoaded,
                         [&fetchedBars](const IBComm::HistoricalBar& bar) {
            fetchedBars.append(bar);
        });

        // 60-second timeout guard
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(60000);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeout.start();

        BarResolution res = BarResolution::Day1;
        source.requestBars(symbols, from, to, res);
        loop.exec();
    } else {
        qWarning() << "HistoricalDataManager: unsupported dataSourceId for fetch:" << dataSourceId;
        return fetchedBars;
    }

    if (!fetchedBars.isEmpty()) {
        insertIntoCache(fetchedBars, resolution, dataSourceId);
    }

    return fetchedBars;
}

void HistoricalDataManager::insertIntoCache(const QVector<IBComm::HistoricalBar>& bars,
                                              const QString& resolution,
                                              const QString& dataSourceId)
{
    QSqlDatabase db = QSqlDatabase::database(m_dbConnectionName);
    if (!db.isOpen()) return;

    const QString todayUtc = QDate::currentDate().startOfDay(Qt::UTC).toString(Qt::ISODate);

    db.transaction();
    for (const auto& bar : bars) {
        if (isIncompleteBar(bar, resolution)) continue;

        DbHistoricalBar dbBar;
        dbBar.symbol       = bar.symbol;
        dbBar.resolution   = resolution;
        dbBar.dataSourceId = dataSourceId;
        // Normalise timestamp to UTC ISO 8601
        dbBar.timestamp    = bar.timestamp.toUTC().toString(Qt::ISODate);
        dbBar.open         = bar.open;
        dbBar.high         = bar.high;
        dbBar.low          = bar.low;
        dbBar.close        = bar.close;
        dbBar.volume       = bar.volume;

        auto q = query_upsertHistoricalBar(dbBar, m_dbConnectionName);
        if (!q.exec()) {
            qWarning() << "HistoricalDataManager: insertIntoCache failed for"
                       << bar.symbol << "@" << dbBar.timestamp << ":" << q.lastError().text();
        }
    }
    db.commit();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString HistoricalDataManager::toUtcIso(const QDateTime& dt) {
    return dt.toUTC().toString(Qt::ISODate);
}

QDateTime HistoricalDataManager::fromUtcIso(const QString& s) {
    return QDateTime::fromString(s, Qt::ISODate).toUTC();
}

bool HistoricalDataManager::isIncompleteBar(const IBComm::HistoricalBar& bar,
                                              const QString& resolution)
{
    // At daily resolution, skip bars on today's date — the session may not be closed yet.
    if (resolution == QLatin1String("Day1")) {
        return bar.timestamp.toUTC().date() >= QDate::currentDate();
    }
    return false;
}

} // namespace Backtest
