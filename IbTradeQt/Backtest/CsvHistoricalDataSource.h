#ifndef BACKTEST_CSVHISTORICALDATASOURCE_H
#define BACKTEST_CSVHISTORICALDATASOURCE_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include "Backtest/IHistoricalDataSource.h"

namespace Backtest {

// Synchronous historical data source backed by a CSV file.
// Expected format (header required):
//   symbol,timestamp,open,high,low,close,volume
//   AAPL,2025-01-02T09:30:00,185.00,186.50,184.80,186.20,1200000
//
// Emits barLoaded() for each row. BacktestSession wires barLoaded() to
// MarketDataReplayer::addBar() which performs OHLC tick synthesis.
class CsvHistoricalDataSource : public IHistoricalDataSource {
    Q_OBJECT
public:
    explicit CsvHistoricalDataSource(const QString& filePath,
                                     QObject* parent = nullptr)
        : IHistoricalDataSource(parent)
        , m_filePath(filePath)
    {}

    QString sourceId() const override { return "csv"; }
    bool requiresLiveBroker() const override { return false; }

    bool supportsResolution(BarResolution r) const override {
        Q_UNUSED(r);
        return true;
    }

    void requestBars(const QStringList& symbols,
                     const QDateTime& from,
                     const QDateTime& to,
                     BarResolution /*resolution*/) override
    {
        QFile file(m_filePath);
        qDebug() << "CsvHistoricalDataSource: opening" << m_filePath
                 << "exists:" << QFile::exists(m_filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit loadFailed("Cannot open CSV file: " + m_filePath);
            return;
        }

        QTextStream in(&file);
        bool firstLine = true;
        int  emittedCount = 0;
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            if (firstLine) {
                firstLine = false;
                continue; // skip header
            }

            const QStringList parts = line.split(',');
            if (parts.size() < 7) continue;

            IBComm::HistoricalBar bar;
            bar.symbol    = parts[0].trimmed();
            bar.timestamp = QDateTime::fromString(parts[1].trimmed(), Qt::ISODate);
            if (!bar.timestamp.isValid()) {
                bar.timestamp = QDateTime::fromString(parts[1].trimmed(), Qt::ISODateWithMs);
            }
            bar.open   = parts[2].toDouble();
            bar.high   = parts[3].toDouble();
            bar.low    = parts[4].toDouble();
            bar.close  = parts[5].toDouble();
            bar.volume = parts[6].toDouble();

            if (!bar.timestamp.isValid()) continue;
            if (!symbols.isEmpty() && !symbols.contains(bar.symbol)) continue;
            if (from.isValid() && bar.timestamp < from) continue;
            if (to.isValid()   && bar.timestamp > to)   continue;

            emit barLoaded(bar);
            ++emittedCount;
        }

        qDebug() << "CsvHistoricalDataSource: emitted" << emittedCount << "bars from" << m_filePath;
        emit loadFinished();
    }

private:
    QString m_filePath;
};

} // namespace Backtest

#endif // BACKTEST_CSVHISTORICALDATASOURCE_H
