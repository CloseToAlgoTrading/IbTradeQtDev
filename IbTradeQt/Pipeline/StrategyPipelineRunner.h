#ifndef PIPELINE_STRATEGYPIPELINERUNNER_H
#define PIPELINE_STRATEGYPIPELINERUNNER_H

#include <QObject>
#include <QVector>
#include <QMap>
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

    void connectToMarketData(IBComm::MarketDataRouter* router) {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(router, &IBComm::MarketDataRouter::tick,
                    alpha, &IAlphaBlock::onTick,
                    Qt::QueuedConnection);
            connect(router, &IBComm::MarketDataRouter::barClose,
                    alpha, &IAlphaBlock::onBarClose,
                    Qt::QueuedConnection);
        }
        connect(router, &IBComm::MarketDataRouter::barClose,
                this, &StrategyPipelineRunner::onBarClose,
                Qt::QueuedConnection);
    }

    void connectToMockRouter(QObject* mockRouter) {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(mockRouter, SIGNAL(tick(IBComm::MarketTick)),
                    alpha, SLOT(onTick(IBComm::MarketTick)),
                    Qt::DirectConnection);
        }
    }

    void wireAlphaSignals() {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(alpha, &IAlphaBlock::signalGenerated,
                    this, &StrategyPipelineRunner::onAlphaSignal,
                    Qt::DirectConnection);
        }
    }

    void setUniverse(const QVector<QString>& universe) {
        m_universe = universe;
    }

    int strategyId() const {
        return m_graph.config.value("strategyId").toInt(0);
    }

    const QVector<Signal>& collectedSignals() const { return m_collectedSignals; }
    const QVector<ExecutionIntent>& lastIntents() const { return m_lastIntents; }

public slots:
    void onAlphaSignal(const Pipeline::Signal& signal) {
        m_collectedSignals.append(signal);
    }

    void onBarClose(const QString& symbol, const QDateTime& timestamp) {
        Q_UNUSED(symbol)
        Q_UNUSED(timestamp)
        runPipeline();
        m_collectedSignals.clear();
    }

    void runPipeline() {
        QString correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        QVector<QString> candidates = runSelection();
        QVector<Signal> mergedSignals = mergeSignals(m_collectedSignals, correlationId);
        QVector<TargetPosition> targets = runMultiLevelRebalance(mergedSignals, correlationId);
        QVector<ExecutionIntent> intents = runMultiLevelRisk(targets, correlationId);
        m_lastIntents = intents;

        executeIntents(intents);

        emit pipelineCompleted(correlationId, intents.size());
    }

    void runPipelineWithSignals(const QVector<Signal>& inputSignals) {
        m_collectedSignals = inputSignals;
        runPipeline();
        m_collectedSignals.clear();
    }

signals:
    void pipelineCompleted(const QString& correlationId, int intentCount);
    void riskRejection(const QString& symbol, const QString& reason);
    void executionError(const QString& symbol, const QString& error);

private:
    QVector<QString> runSelection() {
        QVector<QString> allCandidates;
        for (auto* block : m_graph.selectionBlocks) {
            allCandidates.append(block->select(m_universe));
        }
        return allCandidates;
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
            Signal mergedSignal = m_graph.mergePolicy->merge(it.value());
            mergedSignal.correlationId = corrId;
            merged.append(mergedSignal);
        }
        return merged;
    }

    QVector<TargetPosition> runMultiLevelRebalance(
        const QVector<Signal>& inputSignals,
        const QString& corrId)
    {
        QVector<TargetPosition> targets;
        QMap<QString, double> currentPos = getCurrentPositions();

        if (m_graph.strategyLevel.rebalance) {
            targets = m_graph.strategyLevel.rebalance->rebalance(inputSignals, currentPos);
            for (auto& t : targets) t.correlationId = corrId;
        }
        if (m_graph.portfolioLevel.rebalance) {
            targets = m_graph.portfolioLevel.rebalance->rebalance(inputSignals, currentPos);
            for (auto& t : targets) t.correlationId = corrId;
        }
        if (m_graph.accountLevel.rebalance) {
            targets = m_graph.accountLevel.rebalance->rebalance(inputSignals, currentPos);
            for (auto& t : targets) t.correlationId = corrId;
        }

        return targets;
    }

    QVector<ExecutionIntent> runMultiLevelRisk(
        const QVector<TargetPosition>& targets,
        const QString& correlationId)
    {
        QVector<TargetPosition> approved = targets;

        for (auto* risk : m_graph.strategyLevel.risks) {
            approved = applyRiskBlock(risk, approved);
        }
        for (auto* risk : m_graph.portfolioLevel.risks) {
            approved = applyRiskBlock(risk, approved);
        }
        for (auto* risk : m_graph.accountLevel.risks) {
            approved = applyRiskBlock(risk, approved);
        }

        QVector<ExecutionIntent> intents;
        for (const auto& target : approved) {
            ExecutionIntent intent;
            intent.symbol = target.symbol;
            intent.quantity = target.deltaQuantity();
            intent.orderType = ExecutionIntent::Market;
            intent.riskApproval = "Approved";
            intent.correlationId = target.correlationId.isEmpty()
                ? correlationId : target.correlationId;
            intent.timestamp = QDateTime::currentDateTime();
            intents.append(intent);
        }
        return intents;
    }

    QVector<TargetPosition> applyRiskBlock(
        IRiskBlock* risk,
        const QVector<TargetPosition>& targets)
    {
        QVector<TargetPosition> approved;
        for (const auto& target : targets) {
            auto decision = risk->evaluate(target, targets);

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

    BlockGraph m_graph;
    QVector<QString> m_universe;
    Ports::IOrderExecutionPort* m_executionPort;
    Ports::IPositionRepositoryPort* m_positionRepo;
    QVector<Signal> m_collectedSignals;
    QVector<ExecutionIntent> m_lastIntents;
};

} // namespace Pipeline

#endif // PIPELINE_STRATEGYPIPELINERUNNER_H
