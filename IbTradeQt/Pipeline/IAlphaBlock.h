#ifndef PIPELINE_IALPHABLOCK_H
#define PIPELINE_IALPHABLOCK_H

#include <QObject>
#include <QJsonObject>
#include "Contracts.h"
#include "../IBComm/MarketDataRouter.h"
#include "../Common/IClock.h"

namespace Pipeline {

class IAlphaBlock : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~IAlphaBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual void initialize() = 0;
    virtual void shutdown() = 0;

    // Inject a clock for deterministic time. Live path passes nullptr (falls back
    // to QDateTime::currentDateTime()). Backtest path passes SimulatedClock*.
    virtual void setClock(IClock* clock) { m_clock = clock; }

public slots:
    virtual void onTick(const IBComm::MarketTick& tick) = 0;

    virtual void onBarClose(const QString& symbol, const QDateTime& timestamp) {
        Q_UNUSED(symbol);
        Q_UNUSED(timestamp);
    }

    virtual void onTickByTick(const IBComm::TickByTickTrade& trade) {
        Q_UNUSED(trade);
    }

signals:
    void signalGenerated(const Pipeline::Signal& signal);
    void errorOccurred(const QString& message);

protected:
    // Blocks that need deterministic time call: m_clock ? m_clock->now() : QDateTime::currentDateTime()
    IClock* m_clock = nullptr;
};

} // namespace Pipeline

#endif // PIPELINE_IALPHABLOCK_H
