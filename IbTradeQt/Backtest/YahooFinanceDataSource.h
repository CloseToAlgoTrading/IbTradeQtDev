#ifndef BACKTEST_YAHOOFINANCEDATASOURCE_H
#define BACKTEST_YAHOOFINANCEDATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QDateTime>
#include <QVector>
#include <QSet>
#include "Backtest/IHistoricalDataSource.h"

namespace Backtest {

// Asynchronous historical data source backed by Yahoo Finance CSV API (v7/v8).
//
// Yahoo Finance endpoint:
//   https://query1.finance.yahoo.com/v8/finance/chart/{symbol}
//   ?interval=1d&period1=<unix>&period2=<unix>
//
// Supports Day1 resolution only (Yahoo's free API does not provide intraday
// data beyond 60 days). For each symbol in the request, one HTTP request is
// fired. loadFinished() is emitted after ALL symbols have been fetched.
//
// Data quality: DailyBars (OHLC tick synthesis will be applied by BacktestSession).
//
// Usage:
//   YahooFinanceDataSource src;
//   connect(&src, &IHistoricalDataSource::barLoaded, ...);
//   connect(&src, &IHistoricalDataSource::loadFinished, ...);
//   src.requestBars({"AMD","NVDA","SPY"}, from, to, BarResolution::Day1);
class YahooFinanceDataSource : public IHistoricalDataSource {
    Q_OBJECT
public:
    explicit YahooFinanceDataSource(QObject* parent = nullptr);

    QString sourceId() const override { return "yahoo"; }
    bool requiresLiveBroker() const override { return false; }

    bool supportsResolution(BarResolution r) const override {
        return r == BarResolution::Day1;
    }

    // Fires one HTTPS request per symbol. Asynchronous — returns immediately.
    // loadFinished() or loadFailed() is emitted once all symbols are done.
    void requestBars(const QStringList& symbols,
                     const QDateTime& from,
                     const QDateTime& to,
                     BarResolution resolution) override;

    // Inject a custom QNetworkAccessManager for testing (e.g. mock).
    // Must be called before requestBars(). Takes ownership if parent is set.
    void setNetworkManager(QNetworkAccessManager* mgr);

    // Disconnect from QNetworkAccessManager::finished after a load completes.
    // Required when multiple YahooFinanceDataSource instances share one NAM
    // (e.g. BacktestSession loads strategy data then benchmark data).
    void disconnectFinishedHandler();

    /// When set, chart JSON meta is upserted into InstrumentMetadata (providerId yahoo).
    void setInstrumentMetadataDbConnection(const QString& dbConnectionName);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    void parseChartReply(QNetworkReply* reply);
    void checkAllDone();

    QNetworkAccessManager*  m_nam          = nullptr;
    bool                    m_ownNam       = false;
    QStringList             m_pendingSymbols;
    QStringList             m_requestedSymbols;
    QDateTime               m_from;
    QDateTime               m_to;
    QString                 m_lastError;
    int                     m_pendingCount = 0;
    QSet<QString>           m_failedSymbols;
    QString                 m_instrumentMetadataDbConnection;
};

} // namespace Backtest

#endif // BACKTEST_YAHOOFINANCEDATASOURCE_H
