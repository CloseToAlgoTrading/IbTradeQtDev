#include "MaxPositionRiskBlock.h"

#include "../Pipeline/BlockSubscriptionUtils.h"
#include "../Pipeline/IDataSubscriptionPort.h"
#include "../Pipeline/JsonConfigValue.h"
#include "../Pipeline/PipelineRuntimeContext.h"
#include "../Ports/IPositionRepositoryPort.h"
#include <QUuid>
#include <cmath>

namespace Blocks {

MaxPositionRiskBlock::MaxPositionRiskBlock(QObject* parent)
    : IRiskBlock(parent)
{}

QString MaxPositionRiskBlock::id() const { return QStringLiteral("max-position-risk"); }
QString MaxPositionRiskBlock::name() const { return QStringLiteral("Max Position Risk"); }

Pipeline::Scope MaxPositionRiskBlock::scope() const { return m_scope; }

QJsonObject MaxPositionRiskBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("maxPositionSize")] = m_maxPositionSize;
    cfg[QStringLiteral("maxTotalExposure")] = m_maxTotalExposure;
    cfg[QStringLiteral("stopLossPercent")] = m_stopLossPercent;
    cfg[QStringLiteral("scope")] = static_cast<int>(m_scope);
    return cfg;
}

void MaxPositionRiskBlock::setConfig(const QJsonObject& config)
{
    m_maxPositionSize = Pipeline::jsonDouble(config.value(QStringLiteral("maxPositionSize")), 1000.0);
    m_maxTotalExposure = Pipeline::jsonDouble(config.value(QStringLiteral("maxTotalExposure")), 10000.0);
    m_stopLossPercent = Pipeline::jsonDouble(config.value(QStringLiteral("stopLossPercent")), 0.0);
    m_scope = static_cast<Pipeline::Scope>(
        Pipeline::jsonInt(config.value(QStringLiteral("scope")), static_cast<int>(Pipeline::Scope::Strategy)));
}

void MaxPositionRiskBlock::applySubscriptionUnion()
{
    if (!runtimeContext() || !runtimeContext()->subscription)
        return;

    const QString oid = Pipeline::subscriptionOwnerId(this, QStringLiteral("risk:"), id());
    QSet<QString> seen;
    QVector<QString> syms;
    for (const auto& t : m_cachedAllTargets) {
        const QString u = t.symbol.trimmed().toUpper();
        if (u.isEmpty() || seen.contains(u))
            continue;
        seen.insert(u);
        syms.append(u);
    }
    if (runtimeContext()->positions) {
        const auto r = runtimeContext()->positions->getAllPositions(runtimeContext()->strategyId);
        if (r) {
            for (const auto& p : *r) {
                if (std::abs(p.quantity) < 1e-9)
                    continue;
                const QString u = p.symbol.trimmed().toUpper();
                if (u.isEmpty() || seen.contains(u))
                    continue;
                seen.insert(u);
                syms.append(u);
            }
        }
    }
    if (syms.isEmpty())
        runtimeContext()->subscription->clearOwner(oid);
    else
        runtimeContext()->subscription->setDesiredSymbols(oid, syms);
}

Pipeline::RiskDecision MaxPositionRiskBlock::evaluate(
    const Pipeline::TargetPosition& target,
    const QVector<Pipeline::TargetPosition>& allTargets,
    const QMap<QString, double>& /*currentPositions*/)
{
    m_cachedAllTargets = allTargets;
    applySubscriptionUnion();

    if (std::abs(target.targetQuantity) > m_maxPositionSize) {
        double clampedDelta;
        if (target.targetQuantity > 0) {
            clampedDelta = m_maxPositionSize - target.currentQuantity;
        } else {
            // Negative target is invalid for long-only sizing; clamp toward flat (do not push deeper short).
            const double clampedTarget = 0.0;
            clampedDelta = clampedTarget - target.currentQuantity;
        }

        if (std::abs(clampedDelta) < 1.0) {
            return {Pipeline::RiskDecision::Action::Reject,
                    QStringLiteral("Position %1 exceeds max %2, delta too small to trade")
                        .arg(target.targetQuantity).arg(m_maxPositionSize),
                    {}, id()};
        }

        return {Pipeline::RiskDecision::Action::Modify,
                QStringLiteral("Clamped from %1 to max %2")
                    .arg(target.targetQuantity).arg(m_maxPositionSize),
                clampedDelta, id()};
    }

    double totalExposure = 0.0;
    for (const auto& t : allTargets) {
        totalExposure += std::abs(t.targetQuantity);
    }

    if (totalExposure > m_maxTotalExposure) {
        return {Pipeline::RiskDecision::Action::Reject,
                QStringLiteral("Total exposure %1 exceeds max %2")
                    .arg(totalExposure).arg(m_maxTotalExposure),
                {}, id()};
    }

    return {Pipeline::RiskDecision::Action::Approve, QStringLiteral("Within limits"), {}, id()};
}

void MaxPositionRiskBlock::onTick(const Pipeline::MarketTick& tick)
{
    if (m_stopLossPercent <= 0.0)
        return;
    if (!runtimeContext() || !runtimeContext()->positions)
        return;

    applySubscriptionUnion();

    const int sid = runtimeContext()->strategyId;
    const auto posResult = runtimeContext()->positions->getPosition(sid, tick.symbol);
    if (!posResult) {
        m_stopLossArmed.remove(tick.symbol);
        return;
    }
    if (posResult->quantity <= 1e-9) {
        m_stopLossArmed.remove(tick.symbol);
        return;
    }
    if (posResult->avgCost <= 0.0)
        return;

    double mark = tick.mid();
    if (mark <= 0.0 && tick.bid > 0.0 && tick.ask > 0.0)
        mark = (tick.bid + tick.ask) / 2.0;
    if (mark <= 0.0)
        return;

    const double drop = (mark - posResult->avgCost) / posResult->avgCost;
    if (drop <= -m_stopLossPercent / 100.0) {
        if (m_stopLossArmed.contains(tick.symbol))
            return;
        m_stopLossArmed.insert(tick.symbol);

        Pipeline::Signal s;
        s.symbol = tick.symbol;
        s.direction = Pipeline::Signal::Sell;
        s.alphaBlockId = id();
        s.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        s.timestamp = tick.timestamp;
        emit riskSignalGenerated(s);
    }
}

Pipeline::ModelDataList MaxPositionRiskBlock::processSemantic(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    return IRiskBlock::processSemantic(in, currentPositions, correlationId);
}

} // namespace Blocks
