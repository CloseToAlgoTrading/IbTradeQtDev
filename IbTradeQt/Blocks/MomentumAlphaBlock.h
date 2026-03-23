#ifndef BLOCKS_MOMENTUMALPHABLOCK_H
#define BLOCKS_MOMENTUMALPHABLOCK_H

#include <QJsonObject>
#include "../Pipeline/IAlphaBlock.h"

namespace Blocks {

/// Momentum from **period return on daily (or configured) bars** via `IHistoricalRead` in `processSemantic`.
/// Does not use tick prices for ranking (`onTick` is intentionally empty).
class MomentumAlphaBlock : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit MomentumAlphaBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    void initialize() override;
    void shutdown() override;

    Pipeline::ModelDataList processSemantic(
        const Pipeline::ModelDataList& in,
        const QString& correlationId) override;

public slots:
    /// Ranking and sizing use `processSemantic` + `IHistoricalRead` only; ticks are ignored.
    void onTick(const Pipeline::MarketTick& tick) override;

private:
    int m_period = 20;
    double m_threshold = 0.02;
    int m_topN = 3;
    double m_positionSize = 100.0;
    QString m_resolution = QStringLiteral("Day1");
    QString m_dataSourceId = QStringLiteral("yahoo");
    int m_lookbackYears = 1;
};

} // namespace Blocks

#endif // BLOCKS_MOMENTUMALPHABLOCK_H
