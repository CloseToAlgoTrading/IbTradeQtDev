#ifndef BACKTEST_HISTORICALRANGENORMALIZER_H
#define BACKTEST_HISTORICALRANGENORMALIZER_H

#include <QDateTime>
#include <QString>

namespace Backtest {

/// How cache coverage compares requested [from, to] to stored bar timestamps.
enum class HistoricalRangeComparisonMode {
    UtcCalendarDay, ///< Day1 — compare UTC calendar dates (Yahoo daily semantics).
    BarPeriodUtc,   ///< Fixed-period bars (Sec5, Min1, …) — compare aligned UTC instants.
    FullTimestamp   ///< Tick — full QDateTime precision.
};

/// Canonical [from, to] for gap analysis and coverage planning (single source of truth).
struct NormalizedCoverageRange {
    QDateTime from;
    QDateTime to;
    HistoricalRangeComparisonMode mode = HistoricalRangeComparisonMode::FullTimestamp;
};

/// Maps pipeline / DB resolution string to comparison mode.
HistoricalRangeComparisonMode comparisonModeForResolution(const QString& resolution);

/// Bar period in seconds for fixed-period resolutions; 0 = Tick (no alignment); Day1 uses calendar-day mode.
int barPeriodSecondsForResolution(const QString& resolution);

/// Align [from, to] to bar boundaries for coverage checks. Tick leaves instants unchanged; Day1 uses UTC day bounds.
NormalizedCoverageRange normalizeRangeForCoverage(const QString& resolution,
                                                  const QDateTime& from,
                                                  const QDateTime& to);

/// Whether cached data does not extend early enough to cover \a requestFrom.
bool gapNeedsBefore(const QDateTime& requestFrom,
                    const QDateTime& cachedMin,
                    const QString& resolution,
                    const QString& dataSourceId);

/// Whether cached data does not extend late enough to cover \a requestToEffective.
bool gapNeedsAfter(const QDateTime& requestToEffective,
                   const QDateTime& cachedMax,
                   const QString& resolution,
                   const QString& dataSourceId);

} // namespace Backtest

#endif // BACKTEST_HISTORICALRANGENORMALIZER_H
