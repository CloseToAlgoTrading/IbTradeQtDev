#ifndef BLOCKS_MAXPOSITIONRISKBLOCK_H
#define BLOCKS_MAXPOSITIONRISKBLOCK_H

#include <QJsonObject>
#include <QMap>
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

private:
    double m_maxPositionSize = 1000.0;
    double m_maxTotalExposure = 10000.0;
    Pipeline::Scope m_scope = Pipeline::Scope::Strategy;
};

} // namespace Blocks

#endif // BLOCKS_MAXPOSITIONRISKBLOCK_H
