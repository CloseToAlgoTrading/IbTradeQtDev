#include "DataManagement/YahooHistoricalBarsCoveragePlanner.h"
#include "Backtest/InstrumentInference.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include "Backtest/MarketSessionUtils.h"
#include "DB/dbquery.h"

#include <QSqlDatabase>
#include <QSqlQuery>

namespace DataManagement {

namespace {

QDateTime fromUtcIso(const QString& s)
{
    return QDateTime::fromString(s, Qt::ISODate).toUTC();
}

QDateTime effectiveToForYahooDay1(const QStringList& symbolsInBatch, const QString& dataSourceId,
                                  const QString& resolution, const QDateTime& to,
                                  const QString& dbConnectionName)
{
    if (dataSourceId != QLatin1String("yahoo") || resolution != QLatin1String("Day1"))
        return to;
    if (symbolsInBatch.isEmpty()
        || !Backtest::allSymbolsUseYahooUsCashEquitySessionDaily(dbConnectionName, dataSourceId,
                                                                  symbolsInBatch, {}))
        return to;
    return Backtest::clampEndDateTimeForUsEquityDaily(to);
}

bool cacheNeedsBefore(const QDateTime& from, const QDateTime& cachedMin, const QString& resolution,
                      const QString& dataSourceId)
{
    if (dataSourceId == QLatin1String("yahoo") && resolution == QLatin1String("Day1"))
        return from.toUTC().date() < cachedMin.toUTC().date();
    return from < cachedMin;
}

bool cacheNeedsAfter(const QDateTime& effectiveTo, const QDateTime& cachedMax,
                     const QString& resolution, const QString& dataSourceId)
{
    if (dataSourceId == QLatin1String("yahoo") && resolution == QLatin1String("Day1"))
        return effectiveTo.toUTC().date() > cachedMax.toUTC().date();
    return effectiveTo > cachedMax;
}

struct CachedRange {
    bool    hasData = false;
    QString minTs;
    QString maxTs;
};

CachedRange queryCachedRange(const QString& symbol, const QString& resolution,
                             const QString& dataSourceId, const QString& dbConnectionName)
{
    CachedRange r;
    QSqlDatabase db = QSqlDatabase::database(dbConnectionName);
    if (!db.isOpen())
        return r;

    auto q = query_cachedBarRange(symbol, resolution, dataSourceId, dbConnectionName);
    if (q.exec() && q.next()) {
        r.minTs   = q.value(QStringLiteral("minTs")).toString();
        r.maxTs   = q.value(QStringLiteral("maxTs")).toString();
        r.hasData = !r.minTs.isEmpty() && !r.maxTs.isEmpty();
    }
    return r;
}

} // namespace

YahooHistoricalBarsCoveragePlanner::Plan YahooHistoricalBarsCoveragePlanner::plan(
    const HistoricalBarsDatasetKey& key, const QDateTime& requestedFromUtc,
    const QDateTime& requestedToUtc, const QString& dbConnectionName) const
{
    Plan out;
    const QString& symbol        = key.symbol;
    const QString& resolution    = key.resolution;
    const QString& dataSourceId  = key.dataSourceId;

    const QDateTime reqFromUtc = requestedFromUtc.toUTC();

    out.effectiveRequestedFromUtc = reqFromUtc;
    const QDateTime reqToUtc = requestedToUtc.toUTC();
    const QDateTime effectiveTo =
        effectiveToForYahooDay1(QStringList{symbol}, dataSourceId, resolution, reqToUtc, dbConnectionName);
    out.effectiveRequestedToUtc = effectiveTo;

    CachedRange cached = queryCachedRange(symbol, resolution, dataSourceId, dbConnectionName);

    bool needFetchBefore = false;
    bool needFetchAfter  = false;

    if (!cached.hasData) {
        needFetchBefore = true;
    } else {
        QDateTime cachedMin = fromUtcIso(cached.minTs);
        QDateTime cachedMax = fromUtcIso(cached.maxTs);

        out.hasPreviousRange = true;
        out.previousMinUtc     = cachedMin;
        out.previousMaxUtc     = cachedMax;

        if (cacheNeedsBefore(reqFromUtc, cachedMin, resolution, dataSourceId))
            needFetchBefore = true;
        if (cacheNeedsAfter(effectiveTo, cachedMax, resolution, dataSourceId))
            needFetchAfter = true;

        if (needFetchBefore && cachedMax >= effectiveTo)
            needFetchBefore = false;

        if (needFetchAfter && !needFetchBefore && dataSourceId == QLatin1String("yahoo")
            && resolution == QLatin1String("Day1")
            && Backtest::appliesYahooUsCashEquitySessionDaily(
                   Backtest::InstrumentMetadataResolver::resolve(dbConnectionName, symbol, dataSourceId,
                                                                 {})
                       .effectiveAssetKind)
            && Backtest::isWeekendOnlyYahooEquityGap(cachedMax.addDays(1), effectiveTo)) {
            needFetchAfter = false;
        }
    }

    if (!needFetchBefore && !needFetchAfter)
        return out;

    if (needFetchBefore && needFetchAfter) {
        SyncCoverageSegment seg;
        seg.kind    = SyncCoverageSegmentKind::Full;
        seg.fromUtc = reqFromUtc;
        seg.toUtc   = effectiveTo;
        out.segments.append(seg);
        return out;
    }

    if (!cached.hasData) {
        SyncCoverageSegment seg;
        seg.kind    = SyncCoverageSegmentKind::Full;
        seg.fromUtc = reqFromUtc;
        seg.toUtc   = effectiveTo;
        out.segments.append(seg);
        return out;
    }

    QDateTime cachedMin = fromUtcIso(cached.minTs);
    QDateTime cachedMax = fromUtcIso(cached.maxTs);

    if (needFetchBefore) {
        SyncCoverageSegment seg;
        seg.kind    = SyncCoverageSegmentKind::Prefix;
        seg.fromUtc = reqFromUtc;
        seg.toUtc   = cachedMin.addDays(-1);
        out.segments.append(seg);
    } else if (needFetchAfter) {
        SyncCoverageSegment seg;
        seg.kind    = SyncCoverageSegmentKind::Suffix;
        seg.fromUtc = cachedMax.addDays(1);
        seg.toUtc   = effectiveTo;
        out.segments.append(seg);
    }

    return out;
}

} // namespace DataManagement
