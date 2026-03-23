#ifndef BLOCKS_MAXPOSITIONRISKBLOCK_H
#define BLOCKS_MAXPOSITIONRISKBLOCK_H

#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QVector>
#include "../Pipeline/IRiskBlock.h"
#include "../Pipeline/Scope.h"

namespace Blocks {

class MaxPositionRiskBlock : public Pipeline::IRiskBlock {
    Q_OBJECT

public:
    explicit MaxPositionRiskBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    Pipeline::Scope scope() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition& target,
        const QVector<Pipeline::TargetPosition>& allTargets,
        const QMap<QString, double>& currentPositions) override;

    Pipeline::ModelDataList processSemantic(
        const Pipeline::ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId) override;

    void onTick(const Pipeline::MarketTick& tick) override;

private:
    void applySubscriptionUnion();

    double m_maxPositionSize = 1000.0;
    double m_maxTotalExposure = 10000.0;
    double m_stopLossPercent = 0.0;
    Pipeline::Scope m_scope = Pipeline::Scope::Strategy;
    QVector<Pipeline::TargetPosition> m_cachedAllTargets;
    QSet<QString> m_stopLossArmed;
};

} // namespace Blocks

#endif // BLOCKS_MAXPOSITIONRISKBLOCK_H
