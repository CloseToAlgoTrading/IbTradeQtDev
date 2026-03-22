#ifndef BLOCKS_MOVINGAVERAGECROSSOVERALPHABLOCK_H
#define BLOCKS_MOVINGAVERAGECROSSOVERALPHABLOCK_H

#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QVector>
#include "../Pipeline/IAlphaBlock.h"
#include "../Pipeline/Contracts.h"

namespace Blocks {

class MovingAverageCrossoverAlphaBlock : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit MovingAverageCrossoverAlphaBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    void initialize() override;
    void shutdown() override;

    void onHistoricalBars(const QVector<Pipeline::OHLCVBar>& bars);

public slots:
    void onTick(const Pipeline::MarketTick& tick) override;

private:
    void checkCrossover(const QString& symbol, const QDateTime& ts);
    void emitSignal(const QString& symbol, Pipeline::Signal::Direction dir,
                    double fastMA, double slowMA, const QDateTime& ts);
    static double sma(const QVector<double>& data, int period);

    int m_fastPeriod = 10;
    int m_slowPeriod = 30;
    QMap<QString, QVector<double>> m_closes;
    QMap<QString, bool> m_prevFastAboveSlow;
};

} // namespace Blocks

#endif // BLOCKS_MOVINGAVERAGECROSSOVERALPHABLOCK_H
