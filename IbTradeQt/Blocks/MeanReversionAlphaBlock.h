#ifndef BLOCKS_MEANREVERSIONALPHABLOCK_H
#define BLOCKS_MEANREVERSIONALPHABLOCK_H

#include <QJsonObject>
#include <QMap>
#include <QVector>
#include "../Pipeline/IAlphaBlock.h"

namespace Blocks {

/// Rolling z-score vs mean: **`onTick`** fills price history and may emit `Pipeline::Signal`.
/// **`processSemantic`** uses the same z-score when tick history exists; otherwise it uses **`IHistoricalRead`**
/// (see `resolution` / `dataSourceId` / `lookbackYears` in config) so backtests without ticks still work.
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

    Pipeline::ModelDataList processSemantic(const Pipeline::ModelDataList& in,
                                           const QString& correlationId) override;

public slots:
    void onTick(const Pipeline::MarketTick& tick) override;

private:
    double zScoreFromCloseHistory(const QVector<double>& history) const;
    double zScoreForSymbol(const QString& symbol) const;
    double zScoreFromHistoricalBars(const QString& symbol) const;

    int m_period = 20;
    double m_stdDevThreshold = 2.0;
    QString m_resolution = QStringLiteral("Day1");
    QString m_dataSourceId = QStringLiteral("yahoo");
    int m_lookbackYears = 1;
    QMap<QString, QVector<double>> m_priceHistory;
};

} // namespace Blocks

#endif // BLOCKS_MEANREVERSIONALPHABLOCK_H
