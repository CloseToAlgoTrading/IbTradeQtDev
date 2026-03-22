#ifndef BACKTEST_MARKETSESSIONUTILS_H
#define BACKTEST_MARKETSESSIONUTILS_H

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QStringList>

namespace Backtest {

/// Yahoo Finance symbol classification (heuristic). International listings may
/// be misclassified as EquityUs until explicit overrides exist.
enum class InstrumentKind {
    EquityUs,
    Forex,
    Crypto,
};

InstrumentKind classifyYahooSymbol(const QString& symbol);

/// True if \a nyDate is Saturday or Sunday in America/New_York.
bool isWeekendDateInNy(QDate nyDate);

/// Move \a end backward in NY until the NY calendar date is Mon–Fri; preserves
/// time-of-day in NY. Used to clamp user end dates that fall on US equity weekends.
QDateTime clampEndDateTimeForUsEquityDaily(const QDateTime& end);

/// True if every symbol is US equity (no forex/crypto) — safe for global equity clamp.
bool allSymbolsClassifyAsUsEquity(const QStringList& symbols);

/// True if [period1, period2) (Yahoo chart UTC epoch bounds, period2 exclusive)
/// contains only UTC days whose noon-instant maps to Sat/Sun in NY.
bool isWeekendOnlyChartWindowUtcNy(qint64 period1, qint64 period2);

/// True if the gap [fetchFrom, fetchTo] for a Yahoo Day1 request would only cover
/// NY weekend calendar days (same semantics as chart window, using epoch bounds).
bool isWeekendOnlyYahooEquityGap(const QDateTime& fetchFrom, const QDateTime& fetchTo);

} // namespace Backtest

#endif
