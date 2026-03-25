#include "Backtest/YahooUniverseValidator.h"
#include "Backtest/YahooChartBatchFetch.h"
#include <QDateTime>
#include <QNetworkAccessManager>

namespace Backtest {

namespace {

QDateTime probeFromUtc(const QDateTime& probeToUtc)
{
    return probeToUtc.addDays(-7);
}

QStringList normalizeSymbolList(const QStringList& symbols)
{
    QStringList out;
    QSet<QString> seen;
    for (const QString& s : symbols) {
        const QString t = s.trimmed().toUpper();
        if (t.isEmpty() || seen.contains(t))
            continue;
        seen.insert(t);
        out.append(t);
    }
    return out;
}

} // namespace

YahooUniverseValidator::Result YahooUniverseValidator::validate(const QStringList& symbols,
                                                               QNetworkAccessManager* networkManager,
                                                               const Config& config,
                                                               const QDateTime& probeToUtc)
{
    Result result;
    if (!networkManager)
        return result;

    const QStringList symList = normalizeSymbolList(symbols);
    if (symList.isEmpty())
        return result;

    const QDateTime from       = probeFromUtc(probeToUtc);
    const QString   resolution = QStringLiteral("Day1");
    const int       timeoutMs  = config.yahooFetchTimeoutMs;
    const int       batchSize  = qMax(1, config.yahooFetchBatchSize);
    const QString&  dbConn     = config.instrumentMetadataDbConnection;

    for (int i = 0; i < symList.size(); i += batchSize) {
        const QStringList batch = symList.mid(i, batchSize);
        YahooFetchResult    fr  = fetchYahooChartBatch(batch, from, probeToUtc, resolution, timeoutMs,
                                                       networkManager, dbConn);
        for (const QString& sym : batch) {
            if (fr.failedSymbols.contains(sym))
                result.failedSymbolErrors.insert(sym, QStringLiteral("Yahoo chart request failed"));
            else
                result.okSymbols.insert(sym);
        }
    }

    return result;
}

} // namespace Backtest
