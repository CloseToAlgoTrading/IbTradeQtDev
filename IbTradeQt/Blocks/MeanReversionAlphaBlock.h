#ifndef BLOCKS_MEANREVERSIONALPHABLOCK_H
#define BLOCKS_MEANREVERSIONALPHABLOCK_H

#include <QJsonObject>
#include <QMap>
#include <QVector>
#include "../Pipeline/IAlphaBlock.h"

namespace Blocks {

class MeanReversionAlphaBlock : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit MeanReversionAlphaBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    void initialize() override;
    void shutdown() override;

public slots:
    void onTick(const Pipeline::MarketTick& tick) override;

private:
    int m_period = 20;
    double m_stdDevThreshold = 2.0;
    QMap<QString, QVector<double>> m_priceHistory;
};

} // namespace Blocks

#endif // BLOCKS_MEANREVERSIONALPHABLOCK_H
