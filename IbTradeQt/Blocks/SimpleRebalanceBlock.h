#ifndef BLOCKS_SIMPLEREBALANCEBLOCK_H
#define BLOCKS_SIMPLEREBALANCEBLOCK_H

#include <QDateTime>
#include <QJsonObject>
#include <QSet>
#include "../Pipeline/IRebalanceBlock.h"

namespace Blocks {

class SimpleRebalanceBlock : public Pipeline::IRebalanceBlock {
    Q_OBJECT

public:
    explicit SimpleRebalanceBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    QVector<Pipeline::TargetPosition> rebalance(
        const QVector<Pipeline::Signal>& inputSignals,
        const QMap<QString, double>& currentPositions) override;

    QVector<Pipeline::TargetPosition> processSemanticTargets(
        const Pipeline::ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId) override;

private:
    QVector<Pipeline::TargetPosition> targetsFromModelRows(
        const Pipeline::ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId) const;

    /// For symbols with a position but not in \a newLongSelectionUpper, set target to 0 (unless
    /// already present in \a targets, e.g. explicit sell row).
    void appendFullExitsForDroppedHoldings(
        QVector<Pipeline::TargetPosition>& targets,
        const QMap<QString, double>& currentPositions,
        const QSet<QString>& newLongSelectionUpper,
        const QString& correlationId,
        const QDateTime& eventTime) const;

    double resolvePriceForSymbol(const QString& symbol) const;

    bool m_equalWeight = false;
    QString m_priceResolution = QStringLiteral("Day1");
    QString m_priceDataSourceId = QStringLiteral("yahoo");
    double m_defaultQuantity = 100.0;
};

} // namespace Blocks

#endif // BLOCKS_SIMPLEREBALANCEBLOCK_H
