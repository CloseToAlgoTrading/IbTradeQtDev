#ifndef PIPELINE_STRATEGYPIPELINERUNNER_H
#define PIPELINE_STRATEGYPIPELINERUNNER_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <QDebug>
#include "Contracts.h"
#include "StrategyRuntimePolicy.h"
#include "ISelectionBlock.h"
#include "IAlphaBlock.h"
#include "IRebalanceBlock.h"
#include "IRiskBlock.h"
#include "IExecutionBlock.h"
#include "ISignalMergePolicy.h"
#include "Scope.h"
#include "SemanticModelDataMapper.h"
#include "SemanticPipelineChain.h"
#include "PipelineRuntimeContext.h"
#include "IMarketDataAccessor.h"
#include "../Ports/IOrderExecutionPort.h"
#include "../Ports/IPositionRepositoryPort.h"
#include "../Common/IClock.h"

namespace IBComm {
class MarketDataRouter;
}

namespace Pipeline {

struct BlockGraph {
    QVector<ISelectionBlock*> selectionBlocks;
    QVector<IAlphaBlock*> alphaBlocks;
    ISignalMergePolicy* mergePolicy = nullptr;

    struct LevelBlocks {
        IRebalanceBlock* rebalance = nullptr;
        QVector<IRiskBlock*> risks;
    };

    LevelBlocks strategyLevel;
    LevelBlocks portfolioLevel;
    LevelBlocks accountLevel;

    IExecutionBlock* executionBlock = nullptr;

    QJsonObject config;
};

// StrategyPipelineRunner orchestrates the five-layer pipeline:
//
//   Selection → Alpha → Rebalance → Risk → Execution
//
// Data flow per bar:
//   1. Selection: determines the tradeable universe from configured symbols.
//   2. Alpha:     each alpha block receives ticks and emits signals asynchronously.
//                 Signals are accumulated until the bar closes.
//   3. Rebalance: converts signals into target positions (e.g. AMD 100, NVDA 50).
//                 Receives the full universe so it can decide sizing across assets.
//   4. Risk:      approves/rejects/modifies each target.  Risk blocks also receive
//                 every tick and may emit proactive signals (stop-loss, trailing
//                 stop) that re-enter the pipeline immediately.
//   5. Execution: places market orders for approved intents.
//
// Runtime orchestration:
//   StrategyRuntimePolicy controls two independent cadences:
//     - Evaluation cadence: when selection + alpha run
//     - Rebalance cadence: when signals are converted into portfolio changes
//   Alpha signals can be accumulated between rebalance windows.
//   Risk blocks receive ticks independently and can trigger an emergency path
//   that bypasses normal evaluation/rebalance gating.
//
// Multi-asset synchronisation:
//   With daily bars, many symbols share the same bar-close timestamp. The replayer
//   emits one ohlcvBar per symbol (same timestamp). Alpha blocks must see onBarClose for
//   every symbol at T before runPipeline() for that day. We run the pipeline
//   when the timestamp advances (first bar of a new time) — completing the previous
//   timestamp — and once more at backtest end via flushBarClosePipeline() for the
//   final timestamp.
class StrategyPipelineRunner : public QObject {
    Q_OBJECT

public:
    explicit StrategyPipelineRunner(
        const BlockGraph& graph,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        QObject* parent = nullptr)
        : QObject(parent)
        , m_graph(graph)
        , m_executionPort(executionPort)
        , m_positionRepo(positionRepo)
    {}

    explicit StrategyPipelineRunner(
        const BlockGraph& graph,
        const StrategyRuntimePolicy& policy,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        IClock* clock = nullptr,
        QObject* parent = nullptr)
        : QObject(parent)
        , m_graph(graph)
        , m_runtimePolicy(policy)
        , m_executionPort(executionPort)
        , m_positionRepo(positionRepo)
        , m_clock(clock)
    {}

    /// Feed → runner only: any QObject emitting `tick` / `ohlcvBar` on pipeline domain types.
    template<typename FeedT>
    void connectMarketDataFeed(FeedT* feed) {
        connect(feed, &FeedT::tick, this, &StrategyPipelineRunner::ingestTick, Qt::DirectConnection);
        connect(feed, &FeedT::ohlcvBar, this, &StrategyPipelineRunner::ingestOhlcvBar, Qt::DirectConnection);
    }

    template<typename FeedT>
    void connectMarketDataFeedQueued(FeedT* feed) {
        connect(feed, &FeedT::tick, this, &StrategyPipelineRunner::ingestTick, Qt::QueuedConnection);
        connect(feed, &FeedT::ohlcvBar, this, &StrategyPipelineRunner::ingestOhlcvBar, Qt::QueuedConnection);
    }

    /// Optional tick-by-tick path (alphas only; same feed object as above).
    template<typename FeedT>
    void connectTickByTickFeed(FeedT* feed) {
        connect(feed, &FeedT::tickByTickTrade, this, &StrategyPipelineRunner::ingestTickByTick,
                Qt::DirectConnection);
    }

    template<typename FeedT>
    void connectTickByTickFeedQueued(FeedT* feed) {
        connect(feed, &FeedT::tickByTickTrade, this, &StrategyPipelineRunner::ingestTickByTick,
                Qt::QueuedConnection);
    }

    template<typename RouterT>
    void connectToAnyRouter(RouterT* router) {
        connectMarketDataFeed(router);
    }

    template<typename MockT>
    void connectToMockRouter(MockT* mock) { connectToAnyRouter(mock); }

    /// IB adapter: implementation in StrategyPipelineRunner.cpp (no IB headers in this file).
    void connectToMarketData(IBComm::MarketDataRouter* router);

    // Wire alpha signal collection, async semantic completion, risk proactive signals,
    // and semantic risk violations → runner riskRejection.
    void wireAlphaSignals() {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(alpha, &IAlphaBlock::signalGenerated,
                    this, &StrategyPipelineRunner::onAlphaSignal,
                    Qt::DirectConnection);
            connect(alpha, &IAlphaBlock::semanticReady,
                    this, &StrategyPipelineRunner::onSemanticAlphaReady,
                    Qt::QueuedConnection);
        }
        for (auto* risk : allRiskBlocks()) {
            connect(risk, &IRiskBlock::riskSignalGenerated,
                    this, &StrategyPipelineRunner::onRiskProactiveSignal,
                    Qt::DirectConnection);
            connect(risk, &IRiskBlock::riskViolation,
                    this, &StrategyPipelineRunner::riskRejection,
                    Qt::DirectConnection);
        }
    }

    void setUniverse(const QVector<QString>& universe) {
        m_universe = universe;
    }

    void setClock(IClock* clock) { m_clock = clock; }

    /// Injected ports for all pipeline blocks (realtime/historical/orders/positions). Same types in backtest and live.
    /// Backtest: context pre-populated with resolved policy. Live: fallback parses from graph config if not resolved.
    void setRuntimeContext(const PipelineRuntimeContext& ctx);

    /// Live: inject last-tick accessor (e.g. RouterMarketDataAccessor) after MarketDataRouter is connected.
    void setMarketDataAccessor(IMarketDataAccessor* accessor);

    void setRuntimePolicy(const StrategyRuntimePolicy& policy) {
        m_runtimePolicy = policy;
    }

    int strategyId() const {
        return m_graph.config.value("strategyId").toInt(0);
    }

    const BlockGraph& graph() const { return m_graph; }
    const StrategyRuntimePolicy& runtimePolicy() const { return m_runtimePolicy; }
    const RuntimeState& runtimeState() const { return m_runtimeState; }
    const QVector<Signal>& collectedSignals() const { return m_collectedSignals; }
    const QVector<ExecutionIntent>& lastIntents() const { return m_lastIntents; }

    QDateTime now() const {
        return m_clock ? m_clock->now() : QDateTime::currentDateTime();
    }

public slots:
    /// Sole ingress for quote ticks — dispatches to alpha + risk blocks in config order.
    void ingestTick(const Pipeline::MarketTick& tick) {
        for (auto* alpha : m_graph.alphaBlocks)
            alpha->onTick(tick);
        for (auto* risk : allRiskBlocks())
            risk->onTick(tick);
    }

    /// Authoritative completed bar — alphas receive full OHLCV; pipeline runs when time advances.
    void ingestOhlcvBar(const Pipeline::OHLCVBar& bar) {
        if (m_lastOhlcvBarTimestamp.isValid() && bar.timestamp != m_lastOhlcvBarTimestamp) {
            if (m_lastOhlcvBarTimestamp != m_lastPipelineCompletedForTimestamp) {
                // Replayer advances the tick clock through synthetic OHLC before ohlcvBar; by the
                // time we finalize the *previous* session, now() can already be the *next* day.
                // Simulated fills and historical fallbacks must use the bar period we close.
                syncSimulatedClockTo(m_lastOhlcvBarTimestamp);
                runPipeline();
                m_lastPipelineCompletedForTimestamp = m_lastOhlcvBarTimestamp;
            }
        }
        // Alphas / sizing for this bar see the bar's session time (not a future tick).
        syncSimulatedClockTo(bar.timestamp);
        for (auto* alpha : m_graph.alphaBlocks)
            alpha->onBarClose(bar);
        m_lastOhlcvBarTimestamp = bar.timestamp;
    }

    /// Call after the last ohlcvBar of a session (e.g. end of MarketDataReplayer::replay)
    /// so the final timestamp is evaluated/rebalanced.
    void flushBarClosePipeline() {
        if (!m_lastOhlcvBarTimestamp.isValid())
            return;
        if (m_lastOhlcvBarTimestamp != m_lastPipelineCompletedForTimestamp) {
            syncSimulatedClockTo(m_lastOhlcvBarTimestamp);
            runPipeline();
            m_lastPipelineCompletedForTimestamp = m_lastOhlcvBarTimestamp;
        }
    }

    void ingestTickByTick(const Pipeline::TickByTickTrade& trade) {
        for (auto* alpha : m_graph.alphaBlocks)
            alpha->onTickByTick(trade);
    }

    /// No-op used with BlockingQueuedConnection to drain queued ingress on the runner thread.
    void ping() {}

    /// Slot wrapper for cross-thread universe updates (QStringList is meta-type friendly).
    void setUniverseFromList(const QStringList& symbols) {
        QVector<QString> u;
        u.reserve(symbols.size());
        for (const QString& s : symbols)
            u.append(s);
        m_universe = u;
    }

    void onAlphaSignal(const Pipeline::Signal& signal) {
        m_collectedSignals.append(signal);
    }

    // Risk blocks may emit proactive signals (stop-loss, trailing stop) on any
    // tick.  These bypass the normal bar-close cycle and trigger an immediate
    // emergency pipeline run so the exit order reaches the market without delay.
    void onRiskProactiveSignal(const Pipeline::Signal& signal) {
        runEmergencyRiskPipeline(signal);
    }

    // Run the full pipeline with evaluation and rebalance gating.
    void runPipeline();

    // Inject signals externally (used by tests).
    void runPipelineWithSignals(const QVector<Signal>& inputSignals);

    // Dedicated emergency path for proactive risk signals. Bypasses evaluation
    // and rebalance gating. Does not run selection or alpha. Does not use
    // pending/accumulated alpha signals or standard rebalance allocation.
    // buildEmergencyTargets() produces pre-risk candidate targets, then
    // runMultiLevelRisk() validates/adjusts them exactly once.
    void runEmergencyRiskPipeline(const Pipeline::Signal& riskSignal);

signals:
    void pipelineCompleted(const QString& correlationId, int intentCount);
    void riskRejection(const QString& symbol, const QString& reason);
    void executionError(const QString& symbol, const QString& error);

private slots:
    void onSemanticAlphaReady(const Pipeline::ModelDataList& out, const QString& correlationId);

private:
    /// Backtest: SimulatedClock only. No-op for WallClock (live).
    void syncSimulatedClockTo(const QDateTime& t) {
        if (!m_clock || !t.isValid())
            return;
        if (auto* sc = dynamic_cast<SimulatedClock*>(m_clock))
            sc->setCurrentTime(t);
    }

    void applyRuntimeContextToBlocks();
    void beginPipelineSubscriptionEpoch();
    void endPipelineSubscriptionEpoch();
    void emitPipelineCompleted(const QString& correlationId, int intentCount);

    QVector<IRiskBlock*> allRiskBlocks() const {
        QVector<IRiskBlock*> all;
        all += m_graph.strategyLevel.risks;
        all += m_graph.portfolioLevel.risks;
        all += m_graph.accountLevel.risks;
        return all;
    }

    // 1. Selection layer
    QVector<QString> runSelection() {
        if (m_graph.selectionBlocks.isEmpty()) return m_universe;
        QVector<QString> candidates;
        for (auto* block : m_graph.selectionBlocks) {
            candidates += block->select(m_universe);
        }
        return candidates;
    }

    QVector<Signal> mergeSignals(const QVector<Signal>& alphaSignals, const QString& corrId) {
        if (m_graph.alphaBlocks.size() <= 1 || !m_graph.mergePolicy) {
            return alphaSignals;
        }

        QMap<QString, QVector<Signal>> bySymbol;
        for (const auto& sig : alphaSignals) {
            bySymbol[sig.symbol].append(sig);
        }

        QVector<Signal> merged;
        for (auto it = bySymbol.begin(); it != bySymbol.end(); ++it) {
            Signal s = m_graph.mergePolicy->merge(it.value());
            s.correlationId = corrId;
            merged.append(s);
        }
        return merged;
    }

    // 3. Rebalance layer
    QVector<TargetPosition> runMultiLevelRebalance(
        const QVector<Signal>& alphaSignals,
        const QVector<QString>& /*universe*/,
        const QMap<QString, double>& currentPos,
        const QString& corrId)
    {
        QVector<TargetPosition> targets;

        if (m_graph.strategyLevel.rebalance) {
            targets = m_graph.strategyLevel.rebalance->rebalance(alphaSignals, currentPos);
            for (auto& t : targets) t.correlationId = corrId;
        }
        if (m_graph.portfolioLevel.rebalance) {
            targets = m_graph.portfolioLevel.rebalance->rebalance(alphaSignals, currentPos);
            for (auto& t : targets) t.correlationId = corrId;
        }
        if (m_graph.accountLevel.rebalance) {
            targets = m_graph.accountLevel.rebalance->rebalance(alphaSignals, currentPos);
            for (auto& t : targets) t.correlationId = corrId;
        }

        return targets;
    }

    // 4. Risk layer
    QVector<ExecutionIntent> runMultiLevelRisk(
        const QVector<TargetPosition>& targets,
        const QMap<QString, double>& currentPos,
        const QString& corrId)
    {
        QVector<TargetPosition> approved = targets;

        for (auto* risk : m_graph.strategyLevel.risks)
            approved = applyRiskBlock(risk, approved, currentPos);
        for (auto* risk : m_graph.portfolioLevel.risks)
            approved = applyRiskBlock(risk, approved, currentPos);
        for (auto* risk : m_graph.accountLevel.risks)
            approved = applyRiskBlock(risk, approved, currentPos);

        const QDateTime ts = now();
        QVector<ExecutionIntent> intents;
        for (const auto& target : approved) {
            const double delta = target.deltaQuantity();
            if (qFuzzyIsNull(delta)) continue;

            ExecutionIntent intent;
            intent.symbol      = target.symbol;
            intent.quantity    = delta;
            intent.orderType   = ExecutionIntent::Market;
            intent.riskApproval = "Approved";
            intent.correlationId = target.correlationId.isEmpty() ? corrId : target.correlationId;
            intent.timestamp   = ts;
            intents.append(intent);
        }
        return intents;
    }

    QVector<TargetPosition> applyRiskBlock(
        IRiskBlock* risk,
        const QVector<TargetPosition>& targets,
        const QMap<QString, double>& currentPos)
    {
        QVector<TargetPosition> approved;
        for (const auto& target : targets) {
            auto decision = risk->evaluate(target, targets, currentPos);

            switch (decision.action) {
                case RiskDecision::Action::Approve:
                    approved.append(target);
                    break;
                case RiskDecision::Action::Reject:
                    emit riskRejection(target.symbol, decision.reason);
                    break;
                case RiskDecision::Action::Modify: {
                    auto modified = target;
                    if (decision.modifiedQuantity) {
                        modified.targetQuantity = modified.currentQuantity + *decision.modifiedQuantity;
                        modified.reason += QString(" [RiskModified: %1]").arg(decision.reason);
                    }
                    approved.append(modified);
                    break;
                }
            }
        }
        return approved;
    }

    // Build pre-risk candidate targets from a proactive risk signal.
    // The risk block decides what the override means; the runner does
    // not hardcode a single-symbol-to-zero model.
    QVector<TargetPosition> buildEmergencyTargets(
        const Signal& riskSignal,
        const QMap<QString, double>& currentPos,
        const QDateTime& ts,
        const QString& corrId)
    {
        QVector<TargetPosition> candidates;

        if (riskSignal.direction == Signal::Sell && !riskSignal.symbol.isEmpty()) {
            TargetPosition tp;
            tp.symbol = riskSignal.symbol;
            tp.targetQuantity = 0.0;
            tp.currentQuantity = currentPos.value(riskSignal.symbol, 0.0);
            tp.reason = "Emergency risk: " + riskSignal.alphaBlockId;
            tp.correlationId = corrId;
            tp.timestamp = ts;
            tp.emergencyOriginBlockId = riskSignal.alphaBlockId;
            candidates.append(tp);
        }

        return candidates;
    }

    // 5. Execution layer
    void executeIntents(const QVector<ExecutionIntent>& intents) {
        if (intents.isEmpty()) return;

        if (m_graph.executionBlock) {
            m_graph.executionBlock->execute(intents);
            return;
        }

        if (m_executionPort) {
            for (const auto& intent : intents) {
                auto result = m_executionPort->placeOrder(intent);
                if (!result) {
                    emit executionError(intent.symbol,
                        QString::fromStdString(result.error().message));
                }
            }
        }
    }

    QMap<QString, double> getCurrentPositions() {
        QMap<QString, double> positions;
        if (!m_positionRepo) return positions;
        auto result = m_positionRepo->getAllPositions(strategyId());
        if (result) {
            for (const auto& pos : *result) {
                const QString k = pos.symbol.trimmed().toUpper();
                positions[k] = pos.quantity;
            }
        }
        return positions;
    }

    /// Copies `getCurrentPositions()` into `m_runtimeContext.holdings` for blocks that read the context.
    void refreshHoldings() { m_runtimeContext.holdings = getCurrentPositions(); }

    void advanceSemanticAlphaChain();
    void finishPipelineAfterSemanticAlpha();
    void runSemanticModelPipeline(
        const ModelDataList& mdIn,
        const QMap<QString, double>& currentPos,
        const QString& corrId,
        const QDateTime& ts,
        const QVector<QString>& universe);

    BlockGraph                      m_graph;
    StrategyRuntimePolicy           m_runtimePolicy;
    RuntimeState                    m_runtimeState;
    QVector<QString>                m_universe;
    Ports::IOrderExecutionPort*     m_executionPort;
    Ports::IPositionRepositoryPort* m_positionRepo;
    IClock*                         m_clock = nullptr;
    PipelineRuntimeContext          m_runtimeContext;
    QVector<Signal>                 m_collectedSignals;
    QVector<ExecutionIntent>        m_lastIntents;
    QDateTime                       m_lastOhlcvBarTimestamp;
    QDateTime                       m_lastPipelineCompletedForTimestamp;

    QString                         m_semanticCorrId;
    ModelDataList                   m_semanticChain;
    int                             m_semanticIdx = 0;
    IAlphaBlock*                    m_pendingAsyncAlpha = nullptr;
    QDateTime                       m_pipelineEventTs;
    QVector<QString>                m_pipelineUniverse;
};

} // namespace Pipeline

#endif // PIPELINE_STRATEGYPIPELINERUNNER_H
