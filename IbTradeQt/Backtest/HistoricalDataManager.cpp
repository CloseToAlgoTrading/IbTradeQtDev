#include "Backtest/HistoricalDataManager.h"
#include "Backtest/HistoricalRangeNormalizer.h"
#include "Backtest/InstrumentInference.h"
#include "Backtest/YahooChartBatchFetch.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include "Backtest/MarketSessionUtils.h"
#include "Adapters/IBHistoricalDataFetcher.h"
#include "DB/dbquery.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QNetworkAccessManager>
#include <QHash>
#include <QLoggingCategory>
#include <QThread>
#include <QVariantMap>

Q_LOGGING_CATEGORY(lcHistData, "backtest.historical")

namespace Backtest {

CBrokerDataProvider* HistoricalDataManager::s_brokerDataProvider = nullptr;

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

void HistoricalDataManager::setBrokerDataProvider(CBrokerDataProvider* provider)
{
    s_brokerDataProvider = provider;
}

CBrokerDataProvider* HistoricalDataManager::brokerDataProvider()
{
    return s_brokerDataProvider;
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
    const QHash<QString, QVariantMap>& strategyAssetBySymbol,
    Pipeline::HistoricalReadPolicy policy)
{
    switch (policy) {
    case Pipeline::HistoricalReadPolicy::PreferCache:
        return getBarsPreferCache(symbol, resolution, dataSourceId, from, to,
                                  dataRefreshedAt, strategyAssetBySymbol);
    
    case Pipeline::HistoricalReadPolicy::RefreshFromSource:
        return getBarsRefreshFromSource(symbol, resolution, dataSourceId, from, to,
                                        dataRefreshedAt, strategyAssetBySymbol);
    
    case Pipeline::HistoricalReadPolicy::SourceOnly:
        return getBarsSourceOnly(symbol, resolution, dataSourceId, from, to,
                                dataRefreshedAt, strategyAssetBySymbol);
    }
    return QVector<IBComm::HistoricalBar>();
}

QVector<IBComm::HistoricalBar> HistoricalDataManager::getBarsPreferCache(
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

    const NormalizedCoverageRange norm = normalizeRangeForCoverage(resolution, from, to);
    const QDateTime                  compareFrom = norm.from;

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

        if (gapNeedsBefore(compareFrom, cachedMin, resolution, dataSourceId))
            needFetchBefore = true;
        if (gapNeedsAfter(effectiveTo, cachedMax, resolution, dataSourceId))
            needFetchAfter = true;

        // If the cache already runs through `effectiveTo`, a missing prefix (request `from`
        // before cachedMin) does not require a network fetch: readFromCache still returns
        // bars in [cachedMin, effectiveTo] ∩ [from, to]. Prefix fetches often fail under
        // mock Yahoo in tests and leave momentum (and similar) with only symbols that
        // happened to hit the cache without a gap.
        if (needFetchBefore && cachedMax >= effectiveTo)
            needFetchBefore = false;

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
            fetchAndCache({symbol}, resolution, dataSourceId, fetchFrom, fetchTo,
                          strategyAssetBySymbol);

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

QVector<IBComm::HistoricalBar> HistoricalDataManager::getBarsRefreshFromSource(
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

    QVector<IBComm::HistoricalBar> fetched =
        fetchAndCache({symbol}, resolution, dataSourceId, from, effectiveTo,
                      strategyAssetBySymbol);

    if (dataRefreshedAt) {
        *dataRefreshedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    return normalizeAndFilterBars(fetched, fromUtc, toUtc);
}

QVector<IBComm::HistoricalBar> HistoricalDataManager::getBarsSourceOnly(
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

    QVector<IBComm::HistoricalBar> fetched;
    if (dataSourceId == QLatin1String("yahoo")) {
        if (m_cancelRequested && m_cancelRequested())
            return fetched;
        YahooFetchResult fetchResult = fetchBatchFromYahoo({symbol}, from, effectiveTo, resolution, m_config.yahooFetchTimeoutMs);
        fetched = fetchResult.bars;
    } else if (dataSourceId == QLatin1String("ib")) {
        Adapters::IBHistoricalDataFetcher fetcher(s_brokerDataProvider);
        Adapters::IBHistoricalFetchRequest req;
        req.symbols = QStringList{symbol};
        req.resolution = resolution;
        req.fromUtc = from.toUTC();
        req.toUtc = effectiveTo.toUTC();
        req.timeoutMs = m_config.yahooFetchTimeoutMs;
        req.assetBySymbol = strategyAssetBySymbol;
        req.cancelRequested = m_cancelRequested;
        auto fetchResult = fetcher.fetch(req);
        if (!fetchResult.errorMessage.isEmpty())
            qCWarning(lcHistData) << "HistoricalDataManager:" << fetchResult.errorMessage;
        fetched = fetchResult.bars;
    } else {
        qCWarning(lcHistData) << "HistoricalDataManager: SourceOnly unsupported for dataSourceId:" << dataSourceId;
    }

    if (dataRefreshedAt) {
        // For SourceOnly, this indicates fetch time even though no cache update occurred.
        // The field semantics are "provider consultation time," not "cache update time."
        *dataRefreshedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    return normalizeAndFilterBars(fetched, fromUtc, toUtc);
}

QVector<IBComm::HistoricalBar> HistoricalDataManager::normalizeAndFilterBars(
    const QVector<IBComm::HistoricalBar>& fetched,
    const QString& fromUtc,
    const QString& toUtc) const
{
    const QDateTime fromDt = QDateTime::fromString(fromUtc, Qt::ISODate).toUTC();
    const QDateTime toDt = QDateTime::fromString(toUtc, Qt::ISODate).toUTC();
    
    QMap<QPair<QString, QDateTime>, IBComm::HistoricalBar> uniqueBars;
    for (const auto& bar : fetched) {
        QPair<QString, QDateTime> key{bar.symbol, bar.timestamp.toUTC()};
        uniqueBars[key] = bar;
    }
    
    QVector<IBComm::HistoricalBar> filtered;
    for (auto it = uniqueBars.begin(); it != uniqueBars.end(); ++it) {
        const QDateTime barDt = it.value().timestamp.toUTC();
        if (barDt >= fromDt && barDt <= toDt) {
            filtered.append(it.value());
        }
    }
    
    std::sort(filtered.begin(), filtered.end(),
              [](const IBComm::HistoricalBar& a, const IBComm::HistoricalBar& b) {
                  const QDateTime aDt = a.timestamp.toUTC();
                  const QDateTime bDt = b.timestamp.toUTC();
                  if (aDt != bDt)
                      return aDt < bDt;
                  return a.symbol < b.symbol;
              });
    
    return filtered;
}

QMap<QString, QVector<IBComm::HistoricalBar>> HistoricalDataManager::getBarsMultiSimple(
    const QStringList& symbols,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    QString* dataRefreshedAt,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol,
    Pipeline::HistoricalReadPolicy policy)
{
    QMap<QString, QVector<IBComm::HistoricalBar>> result;
    const QString fromUtc = toUtcIso(from);
    const QString toUtc = toUtcIso(to);

    const QDateTime effectiveTo =
        effectiveToForYahooDay1(symbols, dataSourceId, resolution, to, m_dbConnectionName,
                                strategyAssetBySymbol);

    if (dataSourceId == QLatin1String("yahoo")) {
        if (m_cancelRequested && m_cancelRequested())
            return result;
        YahooFetchResult fetchResult =
            fetchBatchFromYahoo(symbols, from, effectiveTo, resolution, m_config.yahooFetchTimeoutMs);

        if (policy == Pipeline::HistoricalReadPolicy::RefreshFromSource && !fetchResult.bars.isEmpty()) {
            insertIntoCache(fetchResult.bars, resolution, dataSourceId);
        }

        QMap<QString, QVector<IBComm::HistoricalBar>> rawBySymbol;
        for (const auto& bar : fetchResult.bars) {
            rawBySymbol[bar.symbol].append(bar);
        }

        for (const QString& sym : symbols) {
            if (fetchResult.failedSymbols.contains(sym)) {
                qCWarning(lcHistData) << "getBarsMulti:" << sym << "— transport/network error, omitting from result";
                continue;
            }
            
            if (rawBySymbol.contains(sym)) {
                result[sym] = normalizeAndFilterBars(rawBySymbol[sym], fromUtc, toUtc);
            } else {
                qCInfo(lcHistData) << "getBarsMulti:" << sym << "— provider returned no bars for range (valid empty)";
                result[sym] = QVector<IBComm::HistoricalBar>();
            }
        }
    } else if (dataSourceId == QLatin1String("ib")) {
        Adapters::IBHistoricalDataFetcher fetcher(s_brokerDataProvider);
        Adapters::IBHistoricalFetchRequest req;
        req.symbols = symbols;
        req.resolution = resolution;
        req.fromUtc = from.toUTC();
        req.toUtc = effectiveTo.toUTC();
        req.timeoutMs = m_config.yahooFetchTimeoutMs;
        req.assetBySymbol = strategyAssetBySymbol;
        req.cancelRequested = m_cancelRequested;
        auto fetchResult = fetcher.fetch(req);

        if (policy == Pipeline::HistoricalReadPolicy::RefreshFromSource && !fetchResult.bars.isEmpty()) {
            insertIntoCache(fetchResult.bars, resolution, dataSourceId);
        }

        QMap<QString, QVector<IBComm::HistoricalBar>> rawBySymbol;
        for (const auto& bar : fetchResult.bars)
            rawBySymbol[bar.symbol].append(bar);

        for (const QString& sym : symbols) {
            if (fetchResult.failedSymbols.contains(sym)) {
                qCWarning(lcHistData) << "getBarsMulti:" << sym << "— IB request failed, omitting from result";
                continue;
            }
            result[sym] = normalizeAndFilterBars(rawBySymbol.value(sym), fromUtc, toUtc);
        }
    } else {
        qCWarning(lcHistData) << "HistoricalDataManager: unsupported dataSourceId for RefreshFromSource/SourceOnly:" << dataSourceId;
    }

    if (dataRefreshedAt) {
        *dataRefreshedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    return result;
}

QMap<QString, QVector<IBComm::HistoricalBar>> HistoricalDataManager::getBarsMulti(
    const QStringList& symbols,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    QString* dataRefreshedAt,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol,
    Pipeline::HistoricalReadPolicy policy)
{
    if (policy == Pipeline::HistoricalReadPolicy::RefreshFromSource || 
        policy == Pipeline::HistoricalReadPolicy::SourceOnly) {
        return getBarsMultiSimple(symbols, resolution, dataSourceId, from, to,
                                  dataRefreshedAt, strategyAssetBySymbol, policy);
    }

    const QDateTime effectiveTo =
        effectiveToForYahooDay1(symbols, dataSourceId, resolution, to, m_dbConnectionName,
                                strategyAssetBySymbol);

    const NormalizedCoverageRange norm = normalizeRangeForCoverage(resolution, from, to);
    const QDateTime                  compareFrom = norm.from;

    // Union of per-symbol fetch segments — must mirror getBars() so trailing-only /
    // prefix-only gaps do not collapse to a degenerate window (fetchFrom == effectiveTo
    // or fetchTo == from), which would ask Yahoo for a single calendar day.
    QStringList symbolsToFetch;
    bool        haveFetchWindow = false;
    QDateTime   fetchFrom;
    QDateTime   fetchTo;

    auto expandFetchWindow = [&](const QDateTime& segFrom, const QDateTime& segTo) {
        if (!haveFetchWindow) {
            fetchFrom       = segFrom;
            fetchTo         = segTo;
            haveFetchWindow = true;
        } else {
            fetchFrom = qMin(fetchFrom, segFrom);
            fetchTo   = qMax(fetchTo, segTo);
        }
    };

    for (const QString& sym : symbols) {
        CachedRange cached = queryCachedRange(sym, resolution, dataSourceId);

        if (!cached.hasData) {
            symbolsToFetch.append(sym);
            expandFetchWindow(from, effectiveTo);
        } else {
            QDateTime cachedMin = fromUtcIso(cached.minTs);
            QDateTime cachedMax = fromUtcIso(cached.maxTs);

            bool needBefore = gapNeedsBefore(compareFrom, cachedMin, resolution, dataSourceId);
            bool needAfter  = gapNeedsAfter(effectiveTo, cachedMax, resolution, dataSourceId);

            if (needBefore && cachedMax >= effectiveTo)
                needBefore = false;

            if (needAfter && !needBefore && dataSourceId == QLatin1String("yahoo")
                && resolution == QLatin1String("Day1")
                && appliesYahooUsCashEquitySessionDaily(
                       InstrumentMetadataResolver::resolve(m_dbConnectionName, sym, dataSourceId,
                                                           strategyAssetBySymbol.value(sym))
                           .effectiveAssetKind)
                && isWeekendOnlyYahooEquityGap(cachedMax.addDays(1), effectiveTo)) {
                needAfter = false;
            }

            if (!needBefore && !needAfter)
                continue;

            symbolsToFetch.append(sym);
            if (needBefore && needAfter) {
                expandFetchWindow(from, effectiveTo);
            } else if (needBefore) {
                expandFetchWindow(from, cachedMin.addDays(-1));
            } else if (needAfter) {
                expandFetchWindow(cachedMax.addDays(1), effectiveTo);
            }
        }
    }

    bool didFetch = false;
    if (!symbolsToFetch.isEmpty() && haveFetchWindow) {
        QVector<IBComm::HistoricalBar> fetched =
            fetchAndCache(symbolsToFetch, resolution, dataSourceId, fetchFrom, fetchTo,
                          strategyAssetBySymbol);
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

QMap<QString, SymbolCoveragePlanEntry> HistoricalDataManager::computeCoveragePlan(
    const QStringList& symbols,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol) const
{
    QMap<QString, SymbolCoveragePlanEntry> out;

    const QDateTime effectiveTo =
        effectiveToForYahooDay1(symbols, dataSourceId, resolution, to, m_dbConnectionName,
                                strategyAssetBySymbol);
    const NormalizedCoverageRange norm = normalizeRangeForCoverage(resolution, from, to);
    const QDateTime                  compareFrom = norm.from;

    for (const QString& sym : symbols) {
        SymbolCoveragePlanEntry entry;
        CachedRange             cached = queryCachedRange(sym, resolution, dataSourceId);

        if (!cached.hasData) {
            entry.status = SymbolCoveragePlanEntry::Status::NeedsFetch;
            out.insert(sym, entry);
            continue;
        }

        QDateTime cachedMin = fromUtcIso(cached.minTs);
        QDateTime cachedMax = fromUtcIso(cached.maxTs);

        bool needBefore = gapNeedsBefore(compareFrom, cachedMin, resolution, dataSourceId);
        bool needAfter  = gapNeedsAfter(effectiveTo, cachedMax, resolution, dataSourceId);

        if (needBefore && cachedMax >= effectiveTo)
            needBefore = false;

        if (needAfter && !needBefore && dataSourceId == QLatin1String("yahoo")
            && resolution == QLatin1String("Day1")
            && appliesYahooUsCashEquitySessionDaily(
                   InstrumentMetadataResolver::resolve(m_dbConnectionName, sym, dataSourceId,
                                                       strategyAssetBySymbol.value(sym))
                       .effectiveAssetKind)
            && isWeekendOnlyYahooEquityGap(cachedMax.addDays(1), effectiveTo)) {
            needAfter = false;
        }

        if (!needBefore && !needAfter) {
            entry.status = SymbolCoveragePlanEntry::Status::FullyCached;
        } else {
            entry.status = SymbolCoveragePlanEntry::Status::PartialGap;
        }
        out.insert(sym, entry);
    }

    return out;
}

QMap<QString, QVector<IBComm::HistoricalBar>> HistoricalDataManager::getBarsMultiWithRetry(
    const QStringList& symbols,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to,
    const PrefetchRetryOptions& retry,
    QString* dataRefreshedAt,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol,
    Pipeline::HistoricalReadPolicy policy)
{
    QString refreshed;
    QMap<QString, QVector<IBComm::HistoricalBar>> barsBySym =
        getBarsMulti(symbols, resolution, dataSourceId, from, to, &refreshed, strategyAssetBySymbol, policy);

    auto symbolsBelowThreshold = [&](const QStringList& syms) {
        QStringList out;
        for (const QString& sym : syms) {
            if (retry.minBarsPerSymbol > 0 && barsBySym.value(sym).size() < retry.minBarsPerSymbol)
                out.append(sym);
        }
        return out;
    };

    for (int r = 0; r < retry.maxRetries; ++r) {
        const QStringList missing = symbolsBelowThreshold(symbols);
        if (missing.isEmpty())
            break;
        const int backoff = (r == 0) ? retry.firstBackoffMs : retry.laterBackoffMs;
        QThread::msleep(static_cast<unsigned long>(backoff));
        QString                                       r2;
        QMap<QString, QVector<IBComm::HistoricalBar>> extra =
            getBarsMulti(missing, resolution, dataSourceId, from, to, &r2, strategyAssetBySymbol, policy);
        for (auto it = extra.begin(); it != extra.end(); ++it) {
            if (!it.value().isEmpty())
                barsBySym[it.key()] = it.value();
        }
        if (!r2.isEmpty())
            refreshed = r2;
    }

    if (dataRefreshedAt)
        *dataRefreshedAt = refreshed;
    return barsBySym;
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
    const QDateTime& to,
    const QHash<QString, QVariantMap>& strategyAssetBySymbol)
{
    QVector<IBComm::HistoricalBar> fetchedBars;

    if (dataSourceId == QLatin1String("yahoo")) {
        const int batchSize = m_config.yahooFetchBatchSize;
        const int timeoutMs = m_config.yahooFetchTimeoutMs;

        if (symbols.size() > batchSize) {
            qCInfo(lcHistData) << "HistoricalDataManager: fetching" << symbols.size()
                               << "symbols in batches of" << batchSize << "(timeout:" << timeoutMs << "ms each)";
            
            for (int i = 0; i < symbols.size(); i += batchSize) {
                if (m_cancelRequested && m_cancelRequested())
                    break;
                const int remaining = symbols.size() - i;
                const int currentBatchSize = qMin(batchSize, remaining);
                QStringList batch = symbols.mid(i, currentBatchSize);
                
                qCInfo(lcHistData) << "  Batch" << (i/batchSize + 1) << "of"
                                   << ((symbols.size() + batchSize - 1) / batchSize)
                                   << ":" << batch.size() << "symbols";
                
                YahooFetchResult batchResult =
                    fetchBatchFromYahoo(batch, from, to, resolution, timeoutMs);
                
                fetchedBars.append(batchResult.bars);
                
                if (!batchResult.bars.isEmpty()) {
                    insertIntoCache(batchResult.bars, resolution, dataSourceId);
                }
            }
        } else {
            if (m_cancelRequested && m_cancelRequested())
                return fetchedBars;
            YahooFetchResult fetchResult = fetchBatchFromYahoo(symbols, from, to, resolution, timeoutMs);
            fetchedBars = fetchResult.bars;
            
            if (!fetchResult.bars.isEmpty()) {
                insertIntoCache(fetchResult.bars, resolution, dataSourceId);
            }
        }
    } else if (dataSourceId == QLatin1String("ib")) {
        Adapters::IBHistoricalDataFetcher fetcher(s_brokerDataProvider);
        Adapters::IBHistoricalFetchRequest req;
        req.symbols = symbols;
        req.resolution = resolution;
        req.fromUtc = from.toUTC();
        req.toUtc = to.toUTC();
        req.timeoutMs = m_config.yahooFetchTimeoutMs;
        req.assetBySymbol = strategyAssetBySymbol;
        req.cancelRequested = m_cancelRequested;
        auto fetchResult = fetcher.fetch(req);
        fetchedBars = fetchResult.bars;
        if (!fetchedBars.isEmpty())
            insertIntoCache(fetchedBars, resolution, dataSourceId);
        if (!fetchResult.errorMessage.isEmpty())
            qCWarning(lcHistData) << "HistoricalDataManager:" << fetchResult.errorMessage;
    } else {
        qCWarning(lcHistData) << "HistoricalDataManager: unsupported dataSourceId for fetch:" << dataSourceId;
        return fetchedBars;
    }

    return fetchedBars;
}

YahooFetchResult HistoricalDataManager::fetchBatchFromYahoo(
    const QStringList& symbols,
    const QDateTime& from,
    const QDateTime& to,
    const QString& resolution,
    int timeoutMs) const
{
    return fetchYahooChartBatch(symbols, from, to, resolution, timeoutMs, m_networkManager,
                                m_dbConnectionName);
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
