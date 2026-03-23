#ifndef PIPELINE_IALPHABLOCK_H
#define PIPELINE_IALPHABLOCK_H

#include <QObject>
#include <QJsonObject>
#include "Contracts.h"
#include "PipelineRuntimeContext.h"
#include "SemanticTypes.h"
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

    /// Injected ports (realtime, historical, orders, positions) — same for backtest/live.
    virtual void setRuntimeContext(const PipelineRuntimeContext* ctx) { m_runtimeContext = ctx; }
    const PipelineRuntimeContext* runtimeContext() const { return m_runtimeContext; }

    /// Consume prior stage output, return updated list. Concrete blocks choose one primary path (see class docs):
    /// **Historical / bar semantic** — rank or score from `IHistoricalRead` (or bar replay); `onTick` may be empty.
    /// **Tick / streaming** — `onTick` updates rolling state and may emit `signalGenerated`; `processSemantic` may
    /// mirror that state or pass through, depending on the block.
    virtual ModelDataList processSemantic(const ModelDataList& in, const QString& correlationId);

    /// When true, `processSemantic` may complete asynchronously; the runner waits for `semanticReady`.
    virtual bool semanticCompletionIsAsync() const { return false; }

public slots:
    /// Required by the interface; implementations that only use `processSemantic` + historical data typically no-op.
    virtual void onTick(const Pipeline::MarketTick& tick) = 0;

    virtual void onBarClose(const Pipeline::OHLCVBar& bar) {
        Q_UNUSED(bar);
    }

    virtual void onTickByTick(const Pipeline::TickByTickTrade& trade) {
        Q_UNUSED(trade);
    }

signals:
    void signalGenerated(const Pipeline::Signal& signal);
    void semanticReady(const Pipeline::ModelDataList& out, const QString& correlationId);
    void errorOccurred(const QString& message);

protected:
    IClock* m_clock = nullptr;
    const PipelineRuntimeContext* m_runtimeContext = nullptr;
};

} // namespace Pipeline

#endif // PIPELINE_IALPHABLOCK_H
