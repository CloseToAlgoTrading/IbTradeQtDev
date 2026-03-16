#ifndef BACKTEST_HISTORICALDATAMANAGER_H
#define BACKTEST_HISTORICALDATAMANAGER_H

// HistoricalDataManager — smart cache layer between BacktestController and data sources.
//
// Responsibilities:
//   1. Query HistoricalBars table for cached range per (symbol, resolution, dataSourceId).
//   2. Compute missing sub-ranges (before, after, or completely absent).
//   3. Fetch only missing ranges from Yahoo Finance / CSV.
//   4. Insert fetched bars with INSERT OR REPLACE — new fetch always trusted;
//      no provenance retention, no versioning.
//   5. Return the full requested range from cache as ordered, deduplicated bars.
//
// Bar normalisation policy (enforced here):
//   - Yahoo bars: split-adjusted + dividend-adjusted as returned by the API.
//     Unadjusted close is not cached separately.
//   - All timestamps normalised to UTC ISO 8601 before storage.
//   - Daily bars anchored to 21:00 UTC (= 16:00 ET session close).
//   - Calendar gaps (holidays, weekends) are left as gaps — not filled in cache.
//   - Bars with timestamp >= today (UTC) at daily resolution are NOT cached
//     (incomplete session guard).
//
// Usage (synchronous — call from a background QThread):
//   HistoricalDataManager mgr(dbConnectionName, networkManager);
//   auto bars = mgr.getBars("AMD", "Day1", "yahoo", from, to, &refreshedAt);

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QSqlDatabase>
#include <QEventLoop>
#include "IBComm/HistoricalDataRouter.h"
#include "Backtest/IHistoricalDataSource.h"
#include "DB/dbdatatypes.h"

class QNetworkAccessManager;

namespace Backtest {

class HistoricalDataManager : public QObject {
    Q_OBJECT

public:
    // dbConnectionName: name of the already-open QSqlDatabase connection.
    // networkManager:   shared QNetworkAccessManager (may be nullptr — one is created internally).
    explicit HistoricalDataManager(const QString& dbConnectionName,
                                   QNetworkAccessManager* networkManager = nullptr,
                                   QObject* parent = nullptr);

    // Fetch bars for a single (symbol, resolution, dataSourceId) tuple within [from, to].
    // Queries cache, fetches only missing sub-ranges, inserts into DB, returns full range.
    // Blocks until all network fetches complete (use on a background thread).
    // dataRefreshedAt is set to the UTC time when any remote fetch was performed,
    // or empty if the entire range was served from cache.
    // Returns bars ordered by timestamp ascending, deduplicated, normalised to UTC.
    QVector<IBComm::HistoricalBar> getBars(const QString& symbol,
                                            const QString& resolution,
                                            const QString& dataSourceId,
                                            const QDateTime& from,
                                            const QDateTime& to,
                                            QString* dataRefreshedAt = nullptr);

    // Batch variant: fetches bars for multiple symbols with the same resolution/source.
    // Returns a map from symbol -> bars.
    QMap<QString, QVector<IBComm::HistoricalBar>> getBarsMulti(
        const QStringList& symbols,
        const QString& resolution,
        const QString& dataSourceId,
        const QDateTime& from,
        const QDateTime& to,
        QString* dataRefreshedAt = nullptr);

private:
    struct CachedRange {
        bool   hasData  = false;
        QString minTs;
        QString maxTs;
    };

    CachedRange queryCachedRange(const QString& symbol,
                                  const QString& resolution,
                                  const QString& dataSourceId) const;

    QVector<IBComm::HistoricalBar> readFromCache(const QString& symbol,
                                                   const QString& resolution,
                                                   const QString& dataSourceId,
                                                   const QString& fromUtc,
                                                   const QString& toUtc) const;

    // Fetch from Yahoo Finance and insert into cache. Returns fetched bars.
    QVector<IBComm::HistoricalBar> fetchAndCache(const QStringList& symbols,
                                                   const QString& resolution,
                                                   const QString& dataSourceId,
                                                   const QDateTime& from,
                                                   const QDateTime& to);

    // Insert bars into HistoricalBars table (INSERT OR REPLACE).
    // Bars with timestamp >= today at daily resolution are skipped.
    void insertIntoCache(const QVector<IBComm::HistoricalBar>& bars,
                          const QString& resolution,
                          const QString& dataSourceId);

    static QString toUtcIso(const QDateTime& dt);
    static QDateTime fromUtcIso(const QString& s);
    static bool isIncompleteBar(const IBComm::HistoricalBar& bar, const QString& resolution);

    QString                  m_dbConnectionName;
    QNetworkAccessManager*   m_networkManager;
    bool                     m_ownsNetworkManager = false;
};

} // namespace Backtest

#endif // BACKTEST_HISTORICALDATAMANAGER_H
