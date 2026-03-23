#include "Backtest/MarketSessionUtils.h"
#include "Backtest/InstrumentInference.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include <QSqlDatabase>
#include <QTimeZone>

namespace Backtest {

namespace {
const QTimeZone kNyTz(QByteArrayLiteral("America/New_York"));

/// Match YahooFinanceDataSource daily bounds: UTC midnights for from/to dates.
void chartEpochBoundsFromRange(const QDateTime& from, const QDateTime& to,
                               qint64* outP1, qint64* outP2)
{
    if (!from.isValid() || !to.isValid()) {
        *outP1 = 0;
        *outP2 = 0;
        return;
    }
    const QDate d1 = from.toUTC().date();
    const QDate d2 = to.toUTC().date();
    *outP1 = QDateTime(d1, QTime(0, 0), QTimeZone::utc()).toSecsSinceEpoch();
    *outP2 = QDateTime(d2.addDays(1), QTime(0, 0), QTimeZone::utc()).toSecsSinceEpoch();
}

} // namespace

AssetKind classifyYahooSymbol(const QString& symbol)
{
    return inferAssetKindFromSymbolHeuristic(symbol);
}

bool isWeekendDateInNy(QDate nyDate)
{
    const int dow = nyDate.dayOfWeek(); // Qt: Mon=1 .. Sun=7
    return dow == Qt::Saturday || dow == Qt::Sunday;
}

QDateTime clampEndDateTimeForUsEquityDaily(const QDateTime& end)
{
    if (!end.isValid())
        return end;

    QDateTime nyDt = end.toTimeZone(kNyTz);
    QDate     d    = nyDt.date();
    while (isWeekendDateInNy(d))
        d = d.addDays(-1);

    const QDateTime clampedNy = QDateTime(d, nyDt.time(), kNyTz);
    return clampedNy.toUTC();
}

bool allSymbolsUseYahooUsCashEquitySessionDaily(const QString& dbConnectionName,
                                                const QString& providerId,
                                                const QStringList& symbols,
                                                const QHash<QString, QVariantMap>& strategyAssetBySymbol)
{
    if (symbols.isEmpty())
        return false;
    const bool dbOk = !dbConnectionName.isEmpty() && QSqlDatabase::database(dbConnectionName).isOpen();
    for (const QString& sym : symbols) {
        AssetKind k;
        if (dbOk) {
            const QVariantMap entry = strategyAssetBySymbol.value(sym);
            const EffectiveInstrumentProfile p =
                InstrumentMetadataResolver::resolve(dbConnectionName, sym, providerId, entry);
            k = p.effectiveAssetKind;
        } else {
            k = inferAssetKindFromSymbolHeuristic(sym);
        }
        if (!appliesYahooUsCashEquitySessionDaily(k))
            return false;
    }
    return true;
}

bool allSymbolsUseYahooUsCashEquitySessionDaily(const QStringList& symbols)
{
    return allSymbolsUseYahooUsCashEquitySessionDaily(QString(), QStringLiteral("yahoo"), symbols, {});
}

bool isWeekendOnlyChartWindowUtcNy(qint64 period1, qint64 period2)
{
    if (period2 <= period1)
        return false;

    const QDate firstDay =
        QDateTime::fromSecsSinceEpoch(period1, QTimeZone::utc()).date();
    const QDate lastUtcDay =
        QDateTime::fromSecsSinceEpoch(period2, QTimeZone::utc()).date().addDays(-1);
    // [period1, period2) covers UTC calendar days firstDay .. lastUtcDay inclusive
    for (QDate d = firstDay; d <= lastUtcDay; d = d.addDays(1)) {
        const QDateTime noonUtc(d, QTime(12, 0), QTimeZone::utc());
        const QDate     nyDate = noonUtc.toTimeZone(kNyTz).date();
        if (!isWeekendDateInNy(nyDate))
            return false;
    }
    return true;
}

bool isWeekendOnlyYahooEquityGap(const QDateTime& fetchFrom, const QDateTime& fetchTo)
{
    qint64 p1 = 0;
    qint64 p2 = 0;
    chartEpochBoundsFromRange(fetchFrom, fetchTo, &p1, &p2);
    if (p2 <= p1)
        return false;
    return isWeekendOnlyChartWindowUtcNy(p1, p2);
}

} // namespace Backtest
