#include "Backtest/HistoricalRangeNormalizer.h"

namespace Backtest {

HistoricalRangeComparisonMode comparisonModeForResolution(const QString& resolution)
{
    if (resolution == QLatin1String("Tick"))
        return HistoricalRangeComparisonMode::FullTimestamp;
    if (resolution == QLatin1String("Day1"))
        return HistoricalRangeComparisonMode::UtcCalendarDay;
    return HistoricalRangeComparisonMode::BarPeriodUtc;
}

int barPeriodSecondsForResolution(const QString& resolution)
{
    if (resolution == QLatin1String("Tick"))
        return 0;
    if (resolution == QLatin1String("Sec5"))
        return 5;
    if (resolution == QLatin1String("Min1"))
        return 60;
    if (resolution == QLatin1String("Min5"))
        return 300;
    if (resolution == QLatin1String("Min15"))
        return 900;
    if (resolution == QLatin1String("Min30"))
        return 1800;
    if (resolution == QLatin1String("Hour1"))
        return 3600;
    if (resolution == QLatin1String("Day1"))
        return 86400;
    return 0;
}

NormalizedCoverageRange normalizeRangeForCoverage(const QString& resolution,
                                                  const QDateTime& from,
                                                  const QDateTime& to)
{
    NormalizedCoverageRange out;
    out.mode = comparisonModeForResolution(resolution);

    if (!from.isValid() || !to.isValid()) {
        out.from = from;
        out.to   = to;
        return out;
    }

    const QDateTime fUtc = from.toUTC();
    const QDateTime tUtc = to.toUTC();

    if (resolution == QLatin1String("Tick")) {
        out.from = fUtc;
        out.to   = tUtc;
        return out;
    }

    if (resolution == QLatin1String("Day1")) {
        const QDate d1 = fUtc.date();
        const QDate d2 = tUtc.date();
        out.from = QDateTime(d1, QTime(0, 0), Qt::UTC);
        out.to   = QDateTime(d2, QTime(23, 59, 59), Qt::UTC).addMSecs(999);
        return out;
    }

    const int sec = barPeriodSecondsForResolution(resolution);
    if (sec <= 0) {
        out.from = fUtc;
        out.to   = tUtc;
        return out;
    }

    const qint64 periodMs = static_cast<qint64>(sec) * 1000LL;
    qint64       fms      = fUtc.toMSecsSinceEpoch();
    qint64       tms      = tUtc.toMSecsSinceEpoch();
    const qint64 floored  = (fms / periodMs) * periodMs;
    const qint64 ceiled   = ((tms + periodMs - 1) / periodMs) * periodMs - 1;
    const qint64 safeCeil = qMax(ceiled, floored);
    out.from = QDateTime::fromMSecsSinceEpoch(floored, Qt::UTC);
    out.to   = QDateTime::fromMSecsSinceEpoch(safeCeil, Qt::UTC);
    return out;
}

bool gapNeedsBefore(const QDateTime& requestFrom,
                    const QDateTime& cachedMin,
                    const QString& resolution,
                    const QString& dataSourceId)
{
    if (dataSourceId == QLatin1String("yahoo") && resolution == QLatin1String("Day1"))
        return requestFrom.toUTC().date() < cachedMin.toUTC().date();
    return requestFrom < cachedMin;
}

bool gapNeedsAfter(const QDateTime& requestToEffective,
                   const QDateTime& cachedMax,
                   const QString& resolution,
                   const QString& dataSourceId)
{
    if (dataSourceId == QLatin1String("yahoo") && resolution == QLatin1String("Day1"))
        return requestToEffective.toUTC().date() > cachedMax.toUTC().date();
    return requestToEffective > cachedMax;
}

} // namespace Backtest
