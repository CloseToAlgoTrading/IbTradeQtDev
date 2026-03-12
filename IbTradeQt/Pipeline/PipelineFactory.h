#ifndef PIPELINE_PIPELINEFACTORY_H
#define PIPELINE_PIPELINEFACTORY_H

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "StrategyPipelineRunner.h"
#include "BlockRegistry.h"
#include "BlockGraphSerializer.h"
#include "../Supervision/StrategyRuntime.h"
#include "../Blocks/MomentumAlphaBlock.h"
#include "../Blocks/MeanReversionAlphaBlock.h"
#include "../Blocks/MovingAverageCrossoverAlphaBlock.h"
#include "../Blocks/MaxPositionRiskBlock.h"
#include "../Blocks/MarketOrderExecutionBlock.h"
#include "../Blocks/LimitOrderExecutionBlock.h"
#include "../Blocks/StaticListSelectionBlock.h"

namespace Pipeline {

class PipelineFactory {
public:

    static BlockGraph buildGraph(
        const QJsonObject& config,
        Ports::IOrderExecutionPort* executionPort = nullptr)
    {
        BlockGraph graph;
        graph.config = config;

        QJsonArray selectionConfigs = config.value("selection").toArray();
        for (const auto& selVal : selectionConfigs) {
            QJsonObject selCfg = selVal.toObject();
            QString blockId = selCfg.value("blockId").toString();
            ISelectionBlock* sel = createSelectionBlock(blockId);
            if (sel) {
                sel->setConfig(selCfg.value("config").toObject());
                graph.selectionBlocks.append(sel);
            }
        }

        QJsonArray alphaConfigs = config.value("alphas").toArray();
        for (const auto& alphaVal : alphaConfigs) {
            QJsonObject alphaCfg = alphaVal.toObject();
            QString blockId = alphaCfg.value("blockId").toString();

            IAlphaBlock* alpha = createAlphaBlock(blockId);
            if (alpha) {
                alpha->setConfig(alphaCfg.value("config").toObject());
                graph.alphaBlocks.append(alpha);
            }
        }

        QString rebalanceId = config.value("rebalance").toObject().value("blockId").toString();
        graph.strategyLevel.rebalance = createRebalanceBlock(rebalanceId);
        if (graph.strategyLevel.rebalance) {
            graph.strategyLevel.rebalance->setConfig(
                config.value("rebalance").toObject().value("config").toObject());
        }

        QJsonArray riskConfigs = config.value("risks").toArray();
        for (const auto& riskVal : riskConfigs) {
            QJsonObject riskCfg = riskVal.toObject();
            QString riskId = riskCfg.value("blockId").toString();
            IRiskBlock* risk = createRiskBlock(riskId);
            if (risk) {
                risk->setConfig(riskCfg.value("config").toObject());
                graph.strategyLevel.risks.append(risk);
            }
        }

        QString execId = config.value("execution").toObject().value("blockId").toString();
        auto* execBlock = createExecutionBlock(execId);
        if (execBlock) {
            execBlock->setConfig(
                config.value("execution").toObject().value("config").toObject());
            if (executionPort) {
                auto* marketExec = qobject_cast<Blocks::MarketOrderExecutionBlock*>(execBlock);
                if (marketExec) {
                    marketExec->setExecutionPort(executionPort);
                }
            }
            graph.executionBlock = execBlock;
        }

        QString mergeId = config.value("mergePolicy").toString();
        graph.mergePolicy = createMergePolicy(mergeId);

        return graph;
    }

    static Supervision::StrategyRuntime* createRuntime(
        const QString& name,
        const QJsonObject& config,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        IBComm::MarketDataRouter* router = nullptr)
    {
        BlockGraph graph = buildGraph(config, executionPort);

        auto* runtime = new Supervision::StrategyRuntime(
            name, graph, executionPort, positionRepo);

        if (router) {
            runtime->connectToMarketData(router);
        }

        return runtime;
    }

private:
    static ISelectionBlock* createSelectionBlock(const QString& blockId) {
        if (blockId == "pass-all-selection" || blockId == "pass-all")
            return new Blocks::PassAllSelectionBlock();
        if (blockId == "static-list-selection" || blockId == "static-list")
            return new Blocks::StaticListSelectionBlock();
        auto result = BlockRegistry::instance().createBlock(blockId);
        if (result) return qobject_cast<ISelectionBlock*>(*result);
        return new Blocks::PassAllSelectionBlock();
    }

    static IAlphaBlock* createAlphaBlock(const QString& blockId) {
        if (blockId == "momentum-alpha") return new Blocks::MomentumAlphaBlock();
        if (blockId == "mean-reversion-alpha") return new Blocks::MeanReversionAlphaBlock();
        if (blockId == "ma-crossover-alpha") return new Blocks::MovingAverageCrossoverAlphaBlock();
        // extensible: check BlockRegistry
        auto result = BlockRegistry::instance().createBlock(blockId);
        if (result) {
            return qobject_cast<IAlphaBlock*>(*result);
        }
        qWarning() << "PipelineFactory: unknown alpha block:" << blockId;
        return nullptr;
    }

    static IRebalanceBlock* createRebalanceBlock(const QString& blockId) {
        if (blockId == "simple-rebalance" || blockId.isEmpty())
            return new Blocks::SimpleRebalanceBlock();
        auto result = BlockRegistry::instance().createBlock(blockId);
        if (result) return qobject_cast<IRebalanceBlock*>(*result);
        return new Blocks::SimpleRebalanceBlock();
    }

    static IRiskBlock* createRiskBlock(const QString& blockId) {
        if (blockId == "max-position-risk") return new Blocks::MaxPositionRiskBlock();
        auto result = BlockRegistry::instance().createBlock(blockId);
        if (result) return qobject_cast<IRiskBlock*>(*result);
        qWarning() << "PipelineFactory: unknown risk block:" << blockId;
        return nullptr;
    }

    static IExecutionBlock* createExecutionBlock(const QString& blockId) {
        if (blockId == "market-order-execution" || blockId.isEmpty())
            return new Blocks::MarketOrderExecutionBlock();
        if (blockId == "limit-order-execution")
            return new Blocks::LimitOrderExecutionBlock();
        auto result = BlockRegistry::instance().createBlock(blockId);
        if (result) return qobject_cast<IExecutionBlock*>(*result);
        return new Blocks::MarketOrderExecutionBlock();
    }

    static ISignalMergePolicy* createMergePolicy(const QString& policyId) {
        if (policyId == "weighted-vote") return new WeightedVoteMerge();
        if (policyId == "max-confidence") return new MaxConfidenceMerge();
        if (policyId == "consensus") return new ConsensusMerge();
        return nullptr; // single alpha doesn't need merge
    }
};

} // namespace Pipeline

#endif // PIPELINE_PIPELINEFACTORY_H
