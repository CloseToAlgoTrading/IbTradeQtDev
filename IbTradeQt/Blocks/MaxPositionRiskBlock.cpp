#include "MaxPositionRiskBlock.h"

#include "../Pipeline/BlockSubscriptionUtils.h"
#include "../Pipeline/IDataSubscriptionPort.h"
#include "../Pipeline/PipelineRuntimeContext.h"
#include <QJsonObject>
#include <QSet>
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
    cfg[QStringLiteral("scope")] = static_cast<int>(m_scope);
    return cfg;
}

void MaxPositionRiskBlock::setConfig(const QJsonObject& config)
{
    m_maxPositionSize = config.value(QStringLiteral("maxPositionSize")).toDouble(1000.0);
    m_maxTotalExposure = config.value(QStringLiteral("maxTotalExposure")).toDouble(10000.0);
    m_scope = static_cast<Pipeline::Scope>(
        config.value(QStringLiteral("scope")).toInt(static_cast<int>(Pipeline::Scope::Strategy)));
}

Pipeline::RiskDecision MaxPositionRiskBlock::evaluate(
    const Pipeline::TargetPosition& target,
    const QVector<Pipeline::TargetPosition>& allTargets,
    const QMap<QString, double>& /*currentPositions*/)
{
    if (runtimeContext() && runtimeContext()->subscription) {
        const QString oid = Pipeline::subscriptionOwnerId(this, QStringLiteral("risk:"), id());
        QSet<QString> seen;
        QVector<QString> syms;
        for (const auto& t : allTargets) {
            const QString u = t.symbol.trimmed().toUpper();
            if (u.isEmpty() || seen.contains(u))
                continue;
            seen.insert(u);
            syms.append(u);
        }
        if (syms.isEmpty())
            runtimeContext()->subscription->clearOwner(oid);
        else
            runtimeContext()->subscription->setDesiredSymbols(oid, syms);
    }

    if (std::abs(target.targetQuantity) > m_maxPositionSize) {
        double clampedDelta = (target.targetQuantity > 0)
            ? m_maxPositionSize - target.currentQuantity
            : -(m_maxPositionSize + target.currentQuantity);

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

Pipeline::ModelDataList MaxPositionRiskBlock::processSemantic(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    return IRiskBlock::processSemantic(in, currentPositions, correlationId);
}

} // namespace Blocks
