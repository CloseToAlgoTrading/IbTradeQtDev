#ifndef BLOCKS_MEANREVERSIONALPHABLOCK_H
#define BLOCKS_MEANREVERSIONALPHABLOCK_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QUuid>
#include <cmath>
#include "../Pipeline/IAlphaBlock.h"

namespace Blocks {

class MeanReversionAlphaBlock : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit MeanReversionAlphaBlock(QObject* parent = nullptr)
        : IAlphaBlock(parent) {}

    QString id() const override { return "mean-reversion-alpha"; }
    QString name() const override { return "Mean Reversion Alpha"; }
    QString description() const override {
        return "Generates signals when price deviates from rolling mean by configurable std deviations";
    }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["period"] = m_period;
        cfg["stdDevThreshold"] = m_stdDevThreshold;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_period = config.value("period").toInt(20);
        m_stdDevThreshold = config.value("stdDevThreshold").toDouble(2.0);
    }

    void initialize() override { m_priceHistory.clear(); }
    void shutdown() override { m_priceHistory.clear(); }

public slots:
    void onTick(const IBComm::MarketTick& tick) override {
        auto& history = m_priceHistory[tick.symbol];
        history.append(tick.mid());
        if (history.size() > m_period + 1) {
            history.removeFirst();
        }

        if (history.size() < m_period) return;

        double mean = 0.0;
        for (double p : history) mean += p;
        mean /= history.size();

        double variance = 0.0;
        for (double p : history) variance += (p - mean) * (p - mean);
        variance /= history.size();
        double stdDev = std::sqrt(variance);

        if (stdDev < 1e-10) return;

        double currentPrice = history.last();
        double zScore = (currentPrice - mean) / stdDev;

        if (std::abs(zScore) > m_stdDevThreshold) {
            Pipeline::Signal signal;
            signal.symbol = tick.symbol;
            // Mean reversion: sell when above mean, buy when below
            signal.direction = (zScore > 0)
                ? Pipeline::Signal::Sell : Pipeline::Signal::Buy;
            signal.confidence = std::min(std::abs(zScore) / (m_stdDevThreshold * 2.0), 1.0);
            signal.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            signal.timestamp = tick.timestamp;
            signal.alphaBlockId = id();
            emit signalGenerated(signal);
        }
    }

private:
    int m_period = 20;
    double m_stdDevThreshold = 2.0;
    QMap<QString, QVector<double>> m_priceHistory;
};

} // namespace Blocks

#endif // BLOCKS_MEANREVERSIONALPHABLOCK_H
