#ifndef BACKTEST_IHISTORICALDATASOURCE_H
#define BACKTEST_IHISTORICALDATASOURCE_H

#include <QObject>
#include <QStringList>
#include <QDateTime>
#include "IBComm/HistoricalDataRouter.h"
#include "Pipeline/Contracts.h"
#include "Backtest/BacktestConfig.h"

namespace Backtest {

// QObject-based interface for all historical data sources.
// Synchronous sources (PostgreSQL, CSV, JSONL) emit all signals before returning
// from requestBars(). Asynchronous sources (IB API, Yahoo) emit them later.
//
// BacktestSession::loadHistoricalData() uses a QEventLoop to wait for
// loadFinished() or loadFailed(), making the session code identical regardless
// of source type.
class IHistoricalDataSource : public QObject {
    Q_OBJECT
public:
    explicit IHistoricalDataSource(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IHistoricalDataSource() = default;

    // Non-blocking. Emits barLoaded/tickLoaded/tickByTickLoaded per event,
    // then loadFinished() or loadFailed().
    virtual void requestBars(const QStringList& symbols,
                             const QDateTime& from,
                             const QDateTime& to,
                             BarResolution resolution) = 0;

    virtual QString sourceId() const = 0;
    virtual bool requiresLiveBroker() const = 0;
    virtual bool supportsResolution(BarResolution r) const = 0;

signals:
    void barLoaded(const IBComm::HistoricalBar& bar);
    void tickLoaded(const Pipeline::MarketTick& tick);
    void tickByTickLoaded(const Pipeline::TickByTickTrade& trade);
    void loadFinished();
    void loadFailed(const QString& reason);
};

} // namespace Backtest

#endif // BACKTEST_IHISTORICALDATASOURCE_H
