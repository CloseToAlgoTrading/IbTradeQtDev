#ifndef BLOCKS_MOMENTUMALPHABLOCK_H
#define BLOCKS_MOMENTUMALPHABLOCK_H

#include <QJsonObject>
#include <QMap>
#include <QVector>
#include "../Pipeline/IAlphaBlock.h"

namespace Blocks {

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
    void onTick(const Pipeline::MarketTick& tick) override;

private:
    int m_period = 20;
    double m_threshold = 0.02;
    int m_topN = 3;
    double m_positionSize = 100.0;
    QString m_resolution = QStringLiteral("Day1");
    QString m_dataSourceId = QStringLiteral("yahoo");
    int m_lookbackYears = 1;
    QMap<QString, QVector<double>> m_priceHistory;
};

} // namespace Blocks

#endif // BLOCKS_MOMENTUMALPHABLOCK_H
