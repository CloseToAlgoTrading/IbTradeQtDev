#ifndef BACKTEST_YAHOOCHARTBATCHFETCH_H
#define BACKTEST_YAHOOCHARTBATCHFETCH_H

#include "IBComm/HistoricalDataRouter.h"
#include <QDateTime>
#include <QSet>
#include <QStringList>
#include <QVector>

class QNetworkAccessManager;

namespace Backtest {

/// Result of a single Yahoo chart batch request (no DB cache).
struct YahooFetchResult {
    QVector<IBComm::HistoricalBar> bars;
    QSet<QString>                failedSymbols;
};

/// One-shot Yahoo Finance chart fetch (same behavior as HistoricalDataManager::fetchBatchFromYahoo).
YahooFetchResult fetchYahooChartBatch(const QStringList& symbols,
                                      const QDateTime& from,
                                      const QDateTime& to,
                                      const QString& resolution,
                                      int timeoutMs,
                                      QNetworkAccessManager* networkManager,
                                      const QString& instrumentMetadataDbConnection = QString());

} // namespace Backtest

#endif
