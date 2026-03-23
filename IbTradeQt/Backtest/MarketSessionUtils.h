#ifndef BACKTEST_MARKETSESSIONUTILS_H
#define BACKTEST_MARKETSESSIONUTILS_H

#include "Backtest/InstrumentClassification.h"
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Backtest {

/// @deprecated Prefer inferAssetKindFromSymbolHeuristic — kept for call-site clarity.
AssetKind classifyYahooSymbol(const QString& symbol);

/// True if \a nyDate is Saturday or Sunday in America/New_York.
bool isWeekendDateInNy(QDate nyDate);

/// Move \a end backward in NY until the NY calendar date is Mon–Fri; preserves
/// time-of-day in NY. Used to clamp user end dates that fall on US equity weekends.
QDateTime clampEndDateTimeForUsEquityDaily(const QDateTime& end);

/// True if every symbol uses Yahoo US cash equity daily session rules (weekend clamp, gap skip).
/// When \a dbConnectionName is non-empty and the DB is open, uses \ref InstrumentMetadataResolver
/// (provider truth + strategy overrides). Otherwise falls back to symbol heuristics only.
bool allSymbolsUseYahooUsCashEquitySessionDaily(const QString& dbConnectionName,
                                                const QString& providerId,
                                                const QStringList& symbols,
                                                const QHash<QString, QVariantMap>& strategyAssetBySymbol = {});

/// Convenience: no DB / heuristic-only resolution (tests and offline call sites).
bool allSymbolsUseYahooUsCashEquitySessionDaily(const QStringList& symbols);

/// True if [period1, period2) (Yahoo chart UTC epoch bounds, period2 exclusive)
/// contains only UTC days whose noon-instant maps to Sat/Sun in NY.
bool isWeekendOnlyChartWindowUtcNy(qint64 period1, qint64 period2);

/// True if the gap [fetchFrom, fetchTo] for a Yahoo Day1 request would only cover
/// NY weekend calendar days (same semantics as chart window, using epoch bounds).
bool isWeekendOnlyYahooEquityGap(const QDateTime& fetchFrom, const QDateTime& fetchTo);

} // namespace Backtest

#endif
