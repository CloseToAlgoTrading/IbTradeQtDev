#include "tst_historical_range_normalizer.h"
#include "Backtest/HistoricalRangeNormalizer.h"

#include <QtTest>
#include <QDate>
#include <QTimeZone>

using namespace Backtest;

void TestHistoricalRangeNormalizer::normalize_day1_usesUtcCalendarBounds()
{
    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(10, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2024, 1, 4), QTime(18, 30), QTimeZone::utc());
    const NormalizedCoverageRange n = normalizeRangeForCoverage(QStringLiteral("Day1"), from, to);
    QCOMPARE(n.mode, HistoricalRangeComparisonMode::UtcCalendarDay);
    QCOMPARE(n.from, QDateTime(QDate(2024, 1, 2), QTime(0, 0), Qt::UTC));
    QCOMPARE(n.to.date(), QDate(2024, 1, 4));
}

void TestHistoricalRangeNormalizer::normalize_min1_alignsToBarPeriod()
{
    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(9, 31, 15), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2024, 1, 2), QTime(10, 29, 45), QTimeZone::utc());
    const NormalizedCoverageRange n = normalizeRangeForCoverage(QStringLiteral("Min1"), from, to);
    QCOMPARE(n.mode, HistoricalRangeComparisonMode::BarPeriodUtc);
    QCOMPARE(n.from, QDateTime(QDate(2024, 1, 2), QTime(9, 31, 0), Qt::UTC));
    QCOMPARE(n.to, QDateTime(QDate(2024, 1, 2), QTime(10, 29, 59), Qt::UTC).addMSecs(999));
}

void TestHistoricalRangeNormalizer::normalize_tick_preservesFullTimestamp()
{
    const QDateTime from(QDate(2024, 1, 2), QTime(9, 31, 15, 123), Qt::UTC);
    const QDateTime to(QDate(2024, 1, 2), QTime(9, 31, 15, 456), Qt::UTC);
    const NormalizedCoverageRange n = normalizeRangeForCoverage(QStringLiteral("Tick"), from, to);
    QCOMPARE(n.mode, HistoricalRangeComparisonMode::FullTimestamp);
    QCOMPARE(n.from, from);
    QCOMPARE(n.to, to);
}
