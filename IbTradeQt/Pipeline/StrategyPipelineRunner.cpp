#include "StrategyPipelineRunner.h"

#include "../IBComm/MarketDataRouter.h"
#include "IDataSubscriptionPort.h"
#include "IMarketDataAccessor.h"
#include <QUuid>

namespace Pipeline {

void StrategyPipelineRunner::applyRuntimeContextToBlocks()
{
    for (auto* alpha : m_graph.alphaBlocks) {
        if (alpha)
            alpha->setRuntimeContext(&m_runtimeContext);
    }
    for (auto* sel : m_graph.selectionBlocks) {
        if (sel)
            sel->setRuntimeContext(&m_runtimeContext);
    }
    auto applyReb = [this](IRebalanceBlock* r) {
        if (r)
            r->setRuntimeContext(&m_runtimeContext);
    };
    applyReb(m_graph.strategyLevel.rebalance);
    applyReb(m_graph.portfolioLevel.rebalance);
    applyReb(m_graph.accountLevel.rebalance);
    for (auto* risk : allRiskBlocks()) {
        if (risk)
            risk->setRuntimeContext(&m_runtimeContext);
    }
    if (m_graph.executionBlock)
        m_graph.executionBlock->setRuntimeContext(&m_runtimeContext);
}

void StrategyPipelineRunner::beginPipelineSubscriptionEpoch()
{
    if (m_runtimeContext.subscription)
        m_runtimeContext.subscription->beginPipelineEvaluation();
}

void StrategyPipelineRunner::endPipelineSubscriptionEpoch()
{
    if (m_runtimeContext.subscription)
        m_runtimeContext.subscription->endPipelineEvaluation();
}

void StrategyPipelineRunner::emitPipelineCompleted(const QString& correlationId, int intentCount)
{
    endPipelineSubscriptionEpoch();
    emit pipelineCompleted(correlationId, intentCount);
}

void StrategyPipelineRunner::setMarketDataAccessor(IMarketDataAccessor* accessor)
{
    m_runtimeContext.marketData = accessor;
    applyRuntimeContextToBlocks();
}

void StrategyPipelineRunner::connectToMarketData(IBComm::MarketDataRouter* router)
{
    connectMarketDataFeedQueued(router);
    connectTickByTickFeedQueued(router);
}

void StrategyPipelineRunner::runPipeline()
{
    const QString corrId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QDateTime ts = now();

    m_runtimeState.totalBarsSeen++;

    m_runtimeState.barsSinceEvaluation++;
    if (!m_runtimePolicy.shouldEvaluateNow(m_runtimeState, ts)) {
        return;
    }
    m_runtimeState.barsSinceEvaluation = 0;
    m_runtimeState.lastEvaluationTime = ts;

    beginPipelineSubscriptionEpoch();

    const QVector<QString> universe = runSelection();

    const bool useSemantic =
        m_graph.config.value(QStringLiteral("semanticPipeline")).toBool(false);

    if (useSemantic) {
        m_semanticCorrId = corrId;
        m_semanticChain = SemanticMapping::buildModelDataFromSymbols(universe);
        m_semanticIdx = 0;
        m_pipelineEventTs = ts;
        m_pipelineUniverse = universe;
        advanceSemanticAlphaChain();
        return;
    }

    QVector<Signal> tickSigs = mergeSignals(m_collectedSignals, corrId);
    m_collectedSignals.clear();

    const QVector<Signal> alphaSignals = tickSigs;

    m_runtimeState.barsSinceRebalance++;
    if (!m_runtimePolicy.shouldRebalanceNow(m_runtimeState, ts)) {
        if (m_runtimePolicy.accumulateAlphaSignals) {
            for (const auto& sig : alphaSignals) {
                m_runtimeState.pendingSignals.append(
                    {sig, ts, m_runtimeState.totalBarsSeen});
            }
        }
        emitPipelineCompleted(corrId, 0);
        return;
    }
    m_runtimeState.barsSinceRebalance = 0;
    m_runtimeState.lastRebalanceTime = ts;

    QVector<Signal> allSignals;
    for (const auto& ps : m_runtimeState.pendingSignals) {
        if (m_runtimePolicy.signalExpiryBars > 0
            && (m_runtimeState.totalBarsSeen - ps.createdBarIndex) > m_runtimePolicy.signalExpiryBars)
            continue;
        allSignals.append(ps.signal);
    }
    m_runtimeState.pendingSignals.clear();
    allSignals += alphaSignals;

    refreshHoldings();
    const QMap<QString, double>& currentPos = m_runtimeContext.holdings;
    const QVector<TargetPosition> targets = runMultiLevelRebalance(
        allSignals, universe, currentPos, corrId);
    const QVector<ExecutionIntent> intents = runMultiLevelRisk(targets, currentPos, corrId);
    m_lastIntents = intents;

    executeIntents(intents);

    emitPipelineCompleted(corrId, intents.size());
}

void StrategyPipelineRunner::runPipelineWithSignals(const QVector<Signal>& inputSignals)
{
    const QString corrId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    beginPipelineSubscriptionEpoch();

    const QVector<QString> universe = runSelection();
    const QVector<Signal> alphaSignals = mergeSignals(inputSignals, corrId);
    refreshHoldings();
    const QMap<QString, double>& currentPos = m_runtimeContext.holdings;
    const QVector<TargetPosition> targets = runMultiLevelRebalance(alphaSignals, universe, currentPos, corrId);
    const QVector<ExecutionIntent> intents = runMultiLevelRisk(targets, currentPos, corrId);
    m_lastIntents = intents;

    executeIntents(intents);
    emitPipelineCompleted(corrId, intents.size());
}

void StrategyPipelineRunner::runEmergencyRiskPipeline(const Pipeline::Signal& riskSignal)
{
    const QString corrId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QDateTime ts = now();

    beginPipelineSubscriptionEpoch();

    if (m_runtimePolicy.riskCanCancelPendingOrders && m_executionPort)
        (void)m_executionPort->cancelAllPending();

    refreshHoldings();
    const QMap<QString, double>& currentPos = m_runtimeContext.holdings;
    const QVector<TargetPosition> candidates =
        buildEmergencyTargets(riskSignal, currentPos, ts, corrId);

    const QVector<ExecutionIntent> intents =
        runMultiLevelRisk(candidates, currentPos, corrId);
    m_lastIntents = intents;

    executeIntents(intents);
    emitPipelineCompleted(corrId, intents.size());
}

void StrategyPipelineRunner::advanceSemanticAlphaChain()
{
    while (m_semanticIdx < m_graph.alphaBlocks.size()) {
        IAlphaBlock* alpha = m_graph.alphaBlocks[m_semanticIdx];
        if (!alpha->semanticCompletionIsAsync()) {
            m_semanticChain = alpha->processSemantic(m_semanticChain, m_semanticCorrId);
            ++m_semanticIdx;
            continue;
        }
        m_pendingAsyncAlpha = alpha;
        alpha->processSemantic(m_semanticChain, m_semanticCorrId);
        return;
    }
    m_pendingAsyncAlpha = nullptr;
    finishPipelineAfterSemanticAlpha();
}

void StrategyPipelineRunner::onSemanticAlphaReady(const Pipeline::ModelDataList& out, const QString& correlationId)
{
    auto* alpha = qobject_cast<IAlphaBlock*>(sender());
    if (!alpha || alpha != m_pendingAsyncAlpha || correlationId != m_semanticCorrId)
        return;
    m_semanticChain = out ? out : SemanticMapping::buildModelDataFromSymbols({});
    ++m_semanticIdx;
    m_pendingAsyncAlpha = nullptr;
    advanceSemanticAlphaChain();
}

void StrategyPipelineRunner::finishPipelineAfterSemanticAlpha()
{
    const QString corrId = m_semanticCorrId;
    const QDateTime ts = m_pipelineEventTs;
    const QVector<QString> universe = m_pipelineUniverse;

    const bool semanticModelRebalance =
        m_graph.config.value(QStringLiteral("semanticModelRebalance")).toBool(true);
    const bool combine =
        m_graph.config.value(QStringLiteral("combineTickAndSemanticSignals")).toBool(true);

    QString lastAlphaId;
    if (!m_graph.alphaBlocks.isEmpty())
        lastAlphaId = m_graph.alphaBlocks.last()->id();

    QVector<Signal> tickSigs = mergeSignals(m_collectedSignals, corrId);
    m_collectedSignals.clear();

    ModelDataList mergedSemantic = mergeModelDataWithTickSignals(
        m_semanticChain,
        tickSigs,
        combine,
        corrId,
        lastAlphaId);

    const QVector<Signal> alphaSignals =
        SemanticMapping::signalsFromModelData(mergedSemantic, corrId, lastAlphaId);

    m_runtimeState.barsSinceRebalance++;
    if (!m_runtimePolicy.shouldRebalanceNow(m_runtimeState, ts)) {
        if (m_runtimePolicy.accumulateAlphaSignals) {
            for (const auto& sig : alphaSignals) {
                m_runtimeState.pendingSignals.append(
                    {sig, ts, m_runtimeState.totalBarsSeen});
            }
        }
        emitPipelineCompleted(corrId, 0);
        return;
    }
    m_runtimeState.barsSinceRebalance = 0;
    m_runtimeState.lastRebalanceTime = ts;

    QVector<Signal> pendingSignals;
    for (const auto& ps : m_runtimeState.pendingSignals) {
        if (m_runtimePolicy.signalExpiryBars > 0
            && (m_runtimeState.totalBarsSeen - ps.createdBarIndex) > m_runtimePolicy.signalExpiryBars)
            continue;
        pendingSignals.append(ps.signal);
    }
    m_runtimeState.pendingSignals.clear();

    const ModelDataList mdAfterPending = mergeModelDataWithTickSignals(
        mergedSemantic,
        pendingSignals,
        combine,
        corrId,
        lastAlphaId);

    refreshHoldings();
    const QMap<QString, double>& currentPos = m_runtimeContext.holdings;

    if (semanticModelRebalance) {
        runSemanticModelPipeline(mdAfterPending, currentPos, corrId, ts, universe);
        return;
    }

    QVector<Signal> allSignals;
    allSignals += pendingSignals;
    allSignals += alphaSignals;

    const QVector<TargetPosition> targets = runMultiLevelRebalance(
        allSignals, universe, currentPos, corrId);
    const QVector<ExecutionIntent> intents = runMultiLevelRisk(targets, currentPos, corrId);
    m_lastIntents = intents;

    executeIntents(intents);

    emitPipelineCompleted(corrId, intents.size());
}

void StrategyPipelineRunner::runSemanticModelPipeline(
    const ModelDataList& mdIn,
    const QMap<QString, double>& currentPos,
    const QString& corrId,
    const QDateTime& ts,
    const QVector<QString>& /*universe*/)
{
    Q_UNUSED(ts);

    const bool legacyMultiLevelRebalance =
        m_graph.portfolioLevel.rebalance != nullptr || m_graph.accountLevel.rebalance != nullptr;

    if (legacyMultiLevelRebalance) {
        ModelDataList md = mdIn;
        if (m_graph.strategyLevel.rebalance)
            md = m_graph.strategyLevel.rebalance->processSemantic(md, currentPos, corrId);
        if (m_graph.portfolioLevel.rebalance)
            md = m_graph.portfolioLevel.rebalance->processSemantic(md, currentPos, corrId);
        if (m_graph.accountLevel.rebalance)
            md = m_graph.accountLevel.rebalance->processSemantic(md, currentPos, corrId);

        for (auto* risk : m_graph.strategyLevel.risks)
            md = risk->processSemantic(md, currentPos, corrId);
        for (auto* risk : m_graph.portfolioLevel.risks)
            md = risk->processSemantic(md, currentPos, corrId);
        for (auto* risk : m_graph.accountLevel.risks)
            md = risk->processSemantic(md, currentPos, corrId);

        const QDateTime intentTs = now();
        m_lastIntents = SemanticMapping::executionIntentsFromModelData(md, currentPos, corrId, intentTs);

        if (m_graph.executionBlock)
            m_graph.executionBlock->executeSemantic(md, currentPos, corrId, intentTs);
        else
            executeIntents(m_lastIntents);

        emitPipelineCompleted(corrId, m_lastIntents.size());
        return;
    }

    QVector<TargetPosition> targets;
    if (m_graph.strategyLevel.rebalance)
        targets = m_graph.strategyLevel.rebalance->processSemanticTargets(mdIn, currentPos, corrId);
    else
        targets = SemanticMapping::targetPositionsFromModelData(mdIn, currentPos, corrId);

    targets = SemanticMapping::mergeTargetPositionsBySymbolLastWins(targets);

    for (auto* risk : m_graph.strategyLevel.risks)
        targets = risk->processSemanticTargets(targets, currentPos, corrId);
    for (auto* risk : m_graph.portfolioLevel.risks)
        targets = risk->processSemanticTargets(targets, currentPos, corrId);
    for (auto* risk : m_graph.accountLevel.risks)
        targets = risk->processSemanticTargets(targets, currentPos, corrId);

    targets = SemanticMapping::mergeTargetPositionsBySymbolLastWins(targets);

    const QDateTime intentTs = now();
    m_lastIntents =
        SemanticMapping::executionIntentsFromTargetPositions(targets, currentPos, corrId, intentTs);

    if (m_graph.executionBlock)
        m_graph.executionBlock->execute(m_lastIntents);
    else
        executeIntents(m_lastIntents);

    emitPipelineCompleted(corrId, m_lastIntents.size());
}

} // namespace Pipeline
