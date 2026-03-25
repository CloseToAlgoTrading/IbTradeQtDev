#ifndef BACKTEST_YAHOOUNIVERSEVALIDATOR_H
#define BACKTEST_YAHOOUNIVERSEVALIDATOR_H

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QStringList>

class QNetworkAccessManager;

namespace Backtest {

/// Probes Yahoo chart API for symbol usability (transport failures / 404). No SQLite; no pipeline JSON.
class YahooUniverseValidator {
public:
    struct Config {
        int yahooFetchTimeoutMs = 60000;
        int yahooFetchBatchSize = 20;
        /// Optional: forwarded to Yahoo chart metadata upsert; may be empty for probe-only.
        QString instrumentMetadataDbConnection;
    };

    struct Result {
        QSet<QString>            okSymbols;
        QHash<QString, QString> failedSymbolErrors;
    };

    /// Day-based probe window ending at \a probeToUtc (typically "now" UTC).
    static Result validate(const QStringList& symbols,
                           QNetworkAccessManager* networkManager,
                           const Config& config,
                           const QDateTime& probeToUtc);
};

} // namespace Backtest

#endif
