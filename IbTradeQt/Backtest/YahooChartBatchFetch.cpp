#include "Backtest/YahooChartBatchFetch.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/IHistoricalDataSource.h"
#include "Backtest/YahooFinanceDataSource.h"
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QTimer>

namespace Backtest {

YahooFetchResult fetchYahooChartBatch(const QStringList& symbols,
                                      const QDateTime& from,
                                      const QDateTime& to,
                                      const QString& /*resolution*/,
                                      int timeoutMs,
                                      QNetworkAccessManager* networkManager,
                                      const QString& instrumentMetadataDbConnection)
{
    YahooFetchResult result;

    YahooFinanceDataSource source;
    source.setNetworkManager(networkManager);
    source.setInstrumentMetadataDbConnection(instrumentMetadataDbConnection);

    QEventLoop loop;
    QObject::connect(&source, &IHistoricalDataSource::loadFinished,
                     &loop, &QEventLoop::quit);
    QObject::connect(&source, &IHistoricalDataSource::loadFailed,
                     &loop, &QEventLoop::quit);
    QObject::connect(&source, &IHistoricalDataSource::barLoaded,
                     [&result](const IBComm::HistoricalBar& bar) {
                         result.bars.append(bar);
                     });
    QObject::connect(&source, &IHistoricalDataSource::symbolFailed,
                     [&result](const QString& symbol, const QString& /*reason*/) {
                         result.failedSymbols.insert(symbol);
                     });

    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(timeoutMs);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start();

    const BarResolution res = BarResolution::Day1;
    source.requestBars(symbols, from, to, res);
    loop.exec();

    return result;
}

} // namespace Backtest
