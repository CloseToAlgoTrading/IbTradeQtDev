#include "PipelineFactory.h"

#include <QJsonArray>
#include "PipelineLog.h"
#include "SignalMergePolicies.h"
#include "BlockRegistry.h"
#include "../Supervision/StrategyRuntime.h"
#include "../Blocks/MomentumAlphaBlock.h"
#include "../Blocks/MeanReversionAlphaBlock.h"
#include "../Blocks/MovingAverageCrossoverAlphaBlock.h"
#include "../Blocks/MaxPositionRiskBlock.h"
#include "../Blocks/MarketOrderExecutionBlock.h"
#include "../Blocks/SimpleRebalanceBlock.h"
#include "../Blocks/PassAllSelectionBlock.h"
#include "../Blocks/LimitOrderExecutionBlock.h"
#include "../Blocks/StaticListSelectionBlock.h"
#include "../Ports/IOrderExecutionPort.h"
#include "../Ports/IPositionRepositoryPort.h"
#include "../IBComm/MarketDataRouter.h"
#include "IAlphaBlock.h"
#include "ISelectionBlock.h"
#include "IRebalanceBlock.h"
#include "IRiskBlock.h"
#include "IExecutionBlock.h"
#include "ISignalMergePolicy.h"

#include <QObject>

namespace Pipeline {

namespace {

QJsonArray selectionArrayFromConfig(const QJsonObject& config)
{
    const QJsonValue sel = config.value(Key::Selection);
    if (sel.isArray())
        return sel.toArray();
    if (sel.isObject())
        return QJsonArray{sel.toObject()};
    return {};
}

void applyPipelineInstanceId(QObject* block, const QJsonObject& blockCfg)
{
    if (!block)
        return;
    const QString iid = blockCfg.value(QStringLiteral("id")).toString().trimmed();
    if (!iid.isEmpty())
        block->setObjectName(iid);
}
} // namespace

BlockGraph PipelineFactory::buildGraph(
    const QJsonObject& config,
    Ports::IOrderExecutionPort* executionPort)
{
    BlockGraph graph;
    graph.config = config;

    const QJsonArray selectionConfigs = selectionArrayFromConfig(config);
    for (const auto& selVal : selectionConfigs) {
        QJsonObject selCfg = selVal.toObject();
        QString blockId = selCfg.value(Key::BlockId).toString();
        ISelectionBlock* sel = createSelectionBlock(blockId);
        if (sel) {
            applyPipelineInstanceId(sel, selCfg);
            sel->setConfig(selCfg.value(Key::Config).toObject());
            graph.selectionBlocks.append(sel);
        }
    }

    QJsonArray alphaConfigs = config.value(Key::Alphas).toArray();
    for (const auto& alphaVal : alphaConfigs) {
        QJsonObject alphaCfg = alphaVal.toObject();
        QString blockId = alphaCfg.value(Key::BlockId).toString();

        IAlphaBlock* alpha = createAlphaBlock(blockId);
        if (alpha) {
            applyPipelineInstanceId(alpha, alphaCfg);
            alpha->setConfig(alphaCfg.value(Key::Config).toObject());
            graph.alphaBlocks.append(alpha);
        }
    }

    QString rebalanceId = config.value(Key::Rebalance).toObject().value(Key::BlockId).toString();
    graph.strategyLevel.rebalance = createRebalanceBlock(rebalanceId);
    if (graph.strategyLevel.rebalance) {
        applyPipelineInstanceId(graph.strategyLevel.rebalance, config.value(Key::Rebalance).toObject());
        graph.strategyLevel.rebalance->setConfig(
            config.value(Key::Rebalance).toObject().value(Key::Config).toObject());
    }

    QJsonArray riskConfigs = config.value(Key::Risks).toArray();
    for (const auto& riskVal : riskConfigs) {
        QJsonObject riskCfg = riskVal.toObject();
        QString riskId = riskCfg.value(Key::BlockId).toString();
        IRiskBlock* risk = createRiskBlock(riskId);
        if (risk) {
            applyPipelineInstanceId(risk, riskCfg);
            risk->setConfig(riskCfg.value(Key::Config).toObject());
            graph.strategyLevel.risks.append(risk);
        }
    }

    QString execId = config.value(Key::Execution).toObject().value(Key::BlockId).toString();
    auto* execBlock = createExecutionBlock(execId);
    if (execBlock) {
        applyPipelineInstanceId(execBlock, config.value(Key::Execution).toObject());
        execBlock->setConfig(
            config.value(Key::Execution).toObject().value(Key::Config).toObject());
        if (executionPort) {
            auto* marketExec = qobject_cast<Blocks::MarketOrderExecutionBlock*>(execBlock);
            if (marketExec) {
                marketExec->setExecutionPort(executionPort);
            }
        }
        graph.executionBlock = execBlock;
    }

    QString mergeId = config.value(QStringLiteral("mergePolicy")).toString();
    graph.mergePolicy = createMergePolicy(mergeId);

    return graph;
}

PipelineDefinition PipelineFactory::buildDefinition(
    const QJsonObject& config,
    Ports::IOrderExecutionPort* executionPort)
{
    PipelineDefinition def;
    def.graph = buildGraph(config, executionPort);
    def.runtimePolicy = StrategyRuntimePolicy::fromJson(
        config.value(QStringLiteral("runtimePolicy")).toObject());
    return def;
}

Supervision::StrategyRuntime* PipelineFactory::createRuntime(
    const QString& name,
    const QJsonObject& config,
    Ports::IOrderExecutionPort* executionPort,
    Ports::IPositionRepositoryPort* positionRepo,
    IBComm::MarketDataRouter* router,
    IDataSubscriptionPort* subscriptionPort)
{
    PipelineDefinition def = buildDefinition(config, executionPort);

    auto* runtime = new Supervision::StrategyRuntime(
        name, std::move(def), executionPort, positionRepo, subscriptionPort);

    if (router) {
        runtime->connectToMarketData(router);
    }

    return runtime;
}

ISelectionBlock* PipelineFactory::createSelectionBlock(const QString& blockId)
{
    if (blockId.isEmpty()
        || blockId == QStringLiteral("pass-all-selection")
        || blockId == QStringLiteral("pass-all")) {
        return new Blocks::PassAllSelectionBlock();
    }
    if (blockId == QStringLiteral("static-list-selection") || blockId == QStringLiteral("static-list")) {
        return new Blocks::StaticListSelectionBlock();
    }
    auto result = BlockRegistry::instance().createBlock(blockId);
    if (result) {
        return qobject_cast<ISelectionBlock*>(*result);
    }
    qCWarning(pipelineFactoryLog) << "PipelineFactory: unknown selection block:" << blockId;
    return nullptr;
}

IAlphaBlock* PipelineFactory::createAlphaBlock(const QString& blockId)
{
    if (blockId == QStringLiteral("momentum-alpha")) return new Blocks::MomentumAlphaBlock();
    if (blockId == QStringLiteral("mean-reversion-alpha")) return new Blocks::MeanReversionAlphaBlock();
    if (blockId == QStringLiteral("ma-crossover-alpha")) return new Blocks::MovingAverageCrossoverAlphaBlock();
    auto result = BlockRegistry::instance().createBlock(blockId);
    if (result) {
        return qobject_cast<IAlphaBlock*>(*result);
    }
    qCWarning(pipelineFactoryLog) << "PipelineFactory: unknown alpha block:" << blockId;
    return nullptr;
}

IRebalanceBlock* PipelineFactory::createRebalanceBlock(const QString& blockId)
{
    if (blockId == QStringLiteral("simple-rebalance") || blockId.isEmpty())
        return new Blocks::SimpleRebalanceBlock();
    auto result = BlockRegistry::instance().createBlock(blockId);
    if (result) {
        return qobject_cast<IRebalanceBlock*>(*result);
    }
    qCWarning(pipelineFactoryLog) << "PipelineFactory: unknown rebalance block:" << blockId;
    return nullptr;
}

IRiskBlock* PipelineFactory::createRiskBlock(const QString& blockId)
{
    if (blockId == QStringLiteral("max-position-risk")) return new Blocks::MaxPositionRiskBlock();
    auto result = BlockRegistry::instance().createBlock(blockId);
    if (result) return qobject_cast<IRiskBlock*>(*result);
    qCWarning(pipelineFactoryLog) << "PipelineFactory: unknown risk block:" << blockId;
    return nullptr;
}

IExecutionBlock* PipelineFactory::createExecutionBlock(const QString& blockId)
{
    if (blockId == QStringLiteral("market-order-execution") || blockId.isEmpty())
        return new Blocks::MarketOrderExecutionBlock();
    if (blockId == QStringLiteral("limit-order-execution"))
        return new Blocks::LimitOrderExecutionBlock();
    auto result = BlockRegistry::instance().createBlock(blockId);
    if (result) {
        return qobject_cast<IExecutionBlock*>(*result);
    }
    qCWarning(pipelineFactoryLog) << "PipelineFactory: unknown execution block:" << blockId;
    return nullptr;
}

ISignalMergePolicy* PipelineFactory::createMergePolicy(const QString& policyId)
{
    if (policyId == QStringLiteral("weighted-vote")) return new WeightedVoteMerge();
    if (policyId == QStringLiteral("max-confidence")) return new MaxConfidenceMerge();
    if (policyId == QStringLiteral("consensus")) return new ConsensusMerge();
    return nullptr;
}

} // namespace Pipeline
