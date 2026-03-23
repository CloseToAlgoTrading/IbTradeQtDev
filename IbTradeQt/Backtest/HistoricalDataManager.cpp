#include "Backtest/HistoricalDataManager.h"
#include "Backtest/InstrumentInference.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include "Backtest/MarketSessionUtils.h"
#include "Backtest/YahooFinanceDataSource.h"
#include "DB/dbquery.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QTimer>
#include <QHash>
#include <QLoggingCategory>
#include <QVariantMap>

Q_LOGGING_CATEGORY(lcHistData, "backtest.historical")

namespace Backtest {

namespace {

QDateTime effectiveToForYahooDay1(const QStringList& symbolsInBatch,
                                  const QString& dataSourceId,
                                  const QString& resolution,
                                  const QDateTime& to,
                                  const QString& dbConnectionName,
                                  const QHash<QString, QVariantMap>& strategyAssetBySymbol = {})
{
    if (dataSourceId != QLatin1String("yahoo") || resolution != QLatin1String("Day1"))
        return to;
    if (symbolsInBatch.isEmpty()
        || !allSymbolsUseYahooUsCashEquitySessionDaily(dbConnectionName, dataSourceId, symbolsInBatch,
                                                       strategyAssetBySymbol))
        return to;
    return clampEndDateTimeForUsEquityDaily(to);
}

/// Yahoo daily bars use ~session-close UTC (e.g. 21:00); UI `to` is often end-of-calendar-day
/// (23:59:59). Comparing QDateTime would set needAfter every run even when that day is cached.
bool cacheNeedsBefore(const QDateTime& from,
                      const QDateTime& cachedMin,
                      const QString& resolution,
                      const QString& dataSourceId)
{
    if (dataSourceId == QLatin1String("yahoo") && resolution == QLatin1String("Day1"))
        return from.toUTC().date() < cachedMin.toUTC().date();
    return from < cachedMin;
}

bool cacheNeedsAfter(const QDateTime& effectiveTo,
                      const QDateTime& cachedMax,
                      const QString& resolution,
                      const QString& dataSourceId)
{
    if (dataSourceId == QLatin1String("yahoo") && resolution == QLatin1String("Day1"))
        return effectiveTo.toUTC().date() > cachedMax.toUTC().date();
    return effectiveTo > cachedMax;
}

} // namespace

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
    QString* dataRefreshedAt,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol)
{
    const QString fromUtc = toUtcIso(from);
    const QString toUtc   = toUtcIso(to);

    const QDateTime effectiveTo =
        effectiveToForYahooDay1(QStringList{symbol}, dataSourceId, resolution, to, m_dbConnectionName,
                                strategyAssetBySymbol);

    CachedRange cached = queryCachedRange(symbol, resolution, dataSourceId);

    bool needFetchBefore = false;
    bool needFetchAfter  = false;
    QDateTime fetchFrom  = from;
    QDateTime fetchTo    = effectiveTo;

    if (!cached.hasData) {
        // No cache at all — fetch the entire range
        needFetchBefore = true;
    } else {
        QDateTime cachedMin = fromUtcIso(cached.minTs);
        QDateTime cachedMax = fromUtcIso(cached.maxTs);

        if (cacheNeedsBefore(from, cachedMin, resolution, dataSourceId))
            needFetchBefore = true;
        if (cacheNeedsAfter(effectiveTo, cachedMax, resolution, dataSourceId))
            needFetchAfter = true;

        // Trailing gap is US equity weekend-only — Yahoo has no daily bars; skip fetch.
        if (needFetchAfter && !needFetchBefore && dataSourceId == QLatin1String("yahoo")
            && resolution == QLatin1String("Day1")
            && appliesYahooUsCashEquitySessionDaily(
                   InstrumentMetadataResolver::resolve(m_dbConnectionName, symbol, dataSourceId,
                                                     strategyAssetBySymbol.value(symbol))
                       .effectiveAssetKind)
            && isWeekendOnlyYahooEquityGap(cachedMax.addDays(1), effectiveTo)) {
            needFetchAfter = false;
        }

        // Compute the minimal fetch range covering both gaps
        if (needFetchBefore && needFetchAfter) {
            // Fetch from before cachedMin to after cachedMax
            fetchFrom = from;
            fetchTo   = effectiveTo;
        } else if (needFetchBefore) {
            fetchFrom = from;
            fetchTo   = cachedMin.addDays(-1);
        } else if (needFetchAfter) {
            fetchFrom = cachedMax.addDays(1);
            fetchTo   = effectiveTo;
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
    QString* dataRefreshedAt,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol)
{
    const QDateTime effectiveTo =
        effectiveToForYahooDay1(symbols, dataSourceId, resolution, to, m_dbConnectionName,
                                strategyAssetBySymbol);

    // Determine which symbols need fetching and what range is missing
    QStringList symbolsToFetch;
    QDateTime   fetchFrom = effectiveTo; // will be min'd down
    QDateTime   fetchTo   = from;      // will be max'd up

    for (const QString& sym : symbols) {
        CachedRange cached = queryCachedRange(sym, resolution, dataSourceId);

        if (!cached.hasData) {
            symbolsToFetch.append(sym);
            fetchFrom = qMin(fetchFrom, from);
            fetchTo   = qMax(fetchTo, effectiveTo);
        } else {
            QDateTime cachedMin = fromUtcIso(cached.minTs);
            QDateTime cachedMax = fromUtcIso(cached.maxTs);

            bool needBefore = cacheNeedsBefore(from, cachedMin, resolution, dataSourceId);
            bool needAfter  = cacheNeedsAfter(effectiveTo, cachedMax, resolution, dataSourceId);

            if (needAfter && !needBefore && dataSourceId == QLatin1String("yahoo")
                && resolution == QLatin1String("Day1")
                && appliesYahooUsCashEquitySessionDaily(
                       InstrumentMetadataResolver::resolve(m_dbConnectionName, sym, dataSourceId,
                                                           strategyAssetBySymbol.value(sym))
                           .effectiveAssetKind)
                && isWeekendOnlyYahooEquityGap(cachedMax.addDays(1), effectiveTo)) {
                needAfter = false;
            }

            if (needBefore || needAfter) {
                symbolsToFetch.append(sym);
                // Expand fetch window to cover all gaps across all symbols
                if (needBefore) fetchFrom = qMin(fetchFrom, from);
                if (needAfter)  fetchTo   = qMax(fetchTo, effectiveTo);
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
        qCWarning(lcHistData) << "HistoricalDataManager: readFromCache failed:" << q.lastError().text();
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
        source.setInstrumentMetadataDbConnection(m_dbConnectionName);

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
        qCWarning(lcHistData) << "HistoricalDataManager: unsupported dataSourceId for fetch:" << dataSourceId;
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
            qCWarning(lcHistData) << "HistoricalDataManager: insertIntoCache failed for"
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
