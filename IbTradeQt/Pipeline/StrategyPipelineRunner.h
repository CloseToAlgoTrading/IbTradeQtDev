#ifndef PIPELINE_STRATEGYPIPELINERUNNER_H
#define PIPELINE_STRATEGYPIPELINERUNNER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <QDebug>
#include "Contracts.h"
#include "ISelectionBlock.h"
#include "IAlphaBlock.h"
#include "IRebalanceBlock.h"
#include "IRiskBlock.h"
#include "IExecutionBlock.h"
#include "ISignalMergePolicy.h"
#include "Scope.h"
#include "../IBComm/MarketDataRouter.h"
#include "../Ports/IOrderExecutionPort.h"
#include "../Ports/IPositionRepositoryPort.h"

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
// Multi-asset synchronisation:
//   With daily bars, AMD and NVDA both close at the same timestamp.  The replayer
//   emits all ticks for a timestamp before any barClose, then emits one barClose
//   per symbol.  The runner deduplicates on timestamp: it runs the pipeline only
//   on the first barClose for each unique timestamp, by which point all alpha
//   blocks have already processed all ticks for that bar.
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

    // Wire live market data router (used in production).
    void connectToMarketData(IBComm::MarketDataRouter* router) {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(router, &IBComm::MarketDataRouter::tick,
                    alpha, &IAlphaBlock::onTick,
                    Qt::QueuedConnection);
            connect(router, &IBComm::MarketDataRouter::barClose,
                    alpha, &IAlphaBlock::onBarClose,
                    Qt::QueuedConnection);
        }
        for (auto* risk : allRiskBlocks()) {
            connect(router, &IBComm::MarketDataRouter::tick,
                    risk, [risk](const IBComm::MarketTick& t){ risk->onTick(t); },
                    Qt::QueuedConnection);
        }
        connect(router, &IBComm::MarketDataRouter::barClose,
                this, &StrategyPipelineRunner::onBarClose,
                Qt::QueuedConnection);
    }

    // Wire alpha signal collection and risk proactive signals.
    void wireAlphaSignals() {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(alpha, &IAlphaBlock::signalGenerated,
                    this, &StrategyPipelineRunner::onAlphaSignal,
                    Qt::DirectConnection);
        }
        for (auto* risk : allRiskBlocks()) {
            connect(risk, &IRiskBlock::riskSignalGenerated,
                    this, &StrategyPipelineRunner::onRiskProactiveSignal,
                    Qt::DirectConnection);
        }
    }

    void setUniverse(const QVector<QString>& universe) {
        m_universe = universe;
    }

    int strategyId() const {
        return m_graph.config.value("strategyId").toInt(0);
    }

    const BlockGraph& graph() const { return m_graph; }
    const QVector<Signal>& collectedSignals() const { return m_collectedSignals; }
    const QVector<ExecutionIntent>& lastIntents() const { return m_lastIntents; }

public slots:
    // Accumulate alpha signals as they arrive during the bar.
    void onAlphaSignal(const Pipeline::Signal& signal) {
        m_collectedSignals.append(signal);
    }

    // Risk blocks may emit proactive signals (stop-loss, trailing stop) on any
    // tick.  These bypass the normal bar-close cycle and trigger an immediate
    // pipeline run so the exit order reaches the market without delay.
    void onRiskProactiveSignal(const Pipeline::Signal& signal) {
        QVector<Signal> urgentSignals{signal};
        runPipelineWithSignals(urgentSignals);
    }

    // Called once per symbol per bar close.  We deduplicate on timestamp so the
    // pipeline runs exactly once per bar, after all alpha blocks have seen all
    // ticks for that timestamp.
    void onBarClose(const QString& symbol, const QDateTime& timestamp) {
        Q_UNUSED(symbol)

        if (timestamp == m_lastBarCloseTs) return;
        m_lastBarCloseTs = timestamp;

        runPipeline();
        m_collectedSignals.clear();
    }

    // Run the full pipeline with the currently accumulated signals.
    void runPipeline() {
        const QString corrId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        // 1. Selection: determine tradeable universe
        QVector<QString> universe = runSelection();

        // 2. Alpha signals are already in m_collectedSignals (accumulated via onAlphaSignal)
        QVector<Signal> alphaSignals = mergeSignals(m_collectedSignals, corrId);

        // 3. Rebalance: convert signals → target positions across the full universe
        QMap<QString, double> currentPos = getCurrentPositions();
        QVector<TargetPosition> targets = runMultiLevelRebalance(alphaSignals, universe, currentPos, corrId);

        // 4. Risk: approve / reject / modify targets; pass current positions for context
        QVector<ExecutionIntent> intents = runMultiLevelRisk(targets, currentPos, corrId);
        m_lastIntents = intents;

        // 5. Execution
        executeIntents(intents);

        emit pipelineCompleted(corrId, intents.size());
    }

    // Inject signals externally (used by tests and for risk proactive signals).
    void runPipelineWithSignals(const QVector<Signal>& inputSignals) {
        const QString corrId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        QVector<QString> universe = runSelection();
        QVector<Signal> alphaSignals = mergeSignals(inputSignals, corrId);
        QMap<QString, double> currentPos = getCurrentPositions();
        QVector<TargetPosition> targets = runMultiLevelRebalance(alphaSignals, universe, currentPos, corrId);
        QVector<ExecutionIntent> intents = runMultiLevelRisk(targets, currentPos, corrId);
        m_lastIntents = intents;

        executeIntents(intents);
        emit pipelineCompleted(corrId, intents.size());
    }

signals:
    void pipelineCompleted(const QString& correlationId, int intentCount);
    void riskRejection(const QString& symbol, const QString& reason);
    void executionError(const QString& symbol, const QString& error);

private:
    // Collect all risk blocks across all levels for convenience.
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

    // Merge multiple alpha signals for the same symbol into one (when multiple
    // alpha blocks are configured).
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

    // 3. Rebalance layer — each level gets the full universe so it can size
    //    positions across all assets, not just those that generated a signal.
    QVector<TargetPosition> runMultiLevelRebalance(
        const QVector<Signal>& alphaSignals,
        const QVector<QString>& /*universe*/,  // passed for future portfolio-level rebalancers
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

    // 4. Risk layer — passes current positions so risk blocks can compute
    //    exposure, drawdown, etc.
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

        QVector<ExecutionIntent> intents;
        for (const auto& target : approved) {
            const double delta = target.deltaQuantity();
            if (qFuzzyIsNull(delta)) continue;  // nothing to trade

            ExecutionIntent intent;
            intent.symbol      = target.symbol;
            intent.quantity    = delta;
            intent.orderType   = ExecutionIntent::Market;
            intent.riskApproval = "Approved";
            intent.correlationId = target.correlationId.isEmpty() ? corrId : target.correlationId;
            intent.timestamp   = QDateTime::currentDateTime();
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
                positions[pos.symbol] = pos.quantity;
            }
        }
        return positions;
    }

    BlockGraph                      m_graph;
    QVector<QString>                m_universe;
    Ports::IOrderExecutionPort*     m_executionPort;
    Ports::IPositionRepositoryPort* m_positionRepo;
    QVector<Signal>                 m_collectedSignals;
    QVector<ExecutionIntent>        m_lastIntents;
    QDateTime                       m_lastBarCloseTs;  // dedup: run once per timestamp
};

} // namespace Pipeline

#endif // PIPELINE_STRATEGYPIPELINERUNNER_H
