#ifndef BLOCKS_MAXPOSITIONRISKBLOCK_H
#define BLOCKS_MAXPOSITIONRISKBLOCK_H

#include <QObject>
#include <cmath>
#include "../Pipeline/IRiskBlock.h"
#include "../Pipeline/BlockRegistry.h"

namespace Blocks {

class MaxPositionRiskBlock : public Pipeline::IRiskBlock {
    Q_OBJECT

public:
    explicit MaxPositionRiskBlock(QObject* parent = nullptr)
        : IRiskBlock(parent) {}

    QString id() const override { return "max-position-risk"; }
    QString name() const override { return "Max Position Risk"; }
    Pipeline::Scope scope() const override { return m_scope; }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["maxPositionSize"] = m_maxPositionSize;
        cfg["maxTotalExposure"] = m_maxTotalExposure;
        cfg["scope"] = static_cast<int>(m_scope);
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_maxPositionSize = config.value("maxPositionSize").toDouble(1000.0);
        m_maxTotalExposure = config.value("maxTotalExposure").toDouble(10000.0);
        m_scope = static_cast<Pipeline::Scope>(
            config.value("scope").toInt(static_cast<int>(Pipeline::Scope::Strategy)));
    }

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition& target,
        const QVector<Pipeline::TargetPosition>& allTargets,
        const QMap<QString, double>& /*currentPositions*/) override
    {
        if (std::abs(target.targetQuantity) > m_maxPositionSize) {
            double clampedDelta = (target.targetQuantity > 0)
                ? m_maxPositionSize - target.currentQuantity
                : -(m_maxPositionSize + target.currentQuantity);

            if (std::abs(clampedDelta) < 1.0) {
                return {Pipeline::RiskDecision::Action::Reject,
                        QString("Position %1 exceeds max %2, delta too small to trade")
                            .arg(target.targetQuantity).arg(m_maxPositionSize),
                        {}, id()};
            }

            return {Pipeline::RiskDecision::Action::Modify,
                    QString("Clamped from %1 to max %2")
                        .arg(target.targetQuantity).arg(m_maxPositionSize),
                    clampedDelta, id()};
        }

        double totalExposure = 0.0;
        for (const auto& t : allTargets) {
            totalExposure += std::abs(t.targetQuantity);
        }

        if (totalExposure > m_maxTotalExposure) {
            return {Pipeline::RiskDecision::Action::Reject,
                    QString("Total exposure %1 exceeds max %2")
                        .arg(totalExposure).arg(m_maxTotalExposure),
                    {}, id()};
        }

        return {Pipeline::RiskDecision::Action::Approve, "Within limits", {}, id()};
    }

private:
    double m_maxPositionSize = 1000.0;
    double m_maxTotalExposure = 10000.0;
    Pipeline::Scope m_scope = Pipeline::Scope::Strategy;
};

} // namespace Blocks

#endif // BLOCKS_MAXPOSITIONRISKBLOCK_H
