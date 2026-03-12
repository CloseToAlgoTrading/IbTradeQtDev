#ifndef BLOCKS_MOMENTUMALPHABLOCK_H
#define BLOCKS_MOMENTUMALPHABLOCK_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QUuid>
#include <cmath>
#include "../Pipeline/IAlphaBlock.h"
#include "../Pipeline/BlockRegistry.h"

namespace Blocks {

class MomentumAlphaBlock : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit MomentumAlphaBlock(QObject* parent = nullptr)
        : IAlphaBlock(parent) {}

    QString id() const override { return "momentum-alpha"; }
    QString name() const override { return "Momentum Alpha"; }
    QString description() const override {
        return "Generates Buy/Sell signals based on price momentum over a configurable window";
    }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["period"] = m_period;
        cfg["threshold"] = m_threshold;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_period = config.value("period").toInt(20);
        m_threshold = config.value("threshold").toDouble(0.02);
    }

    void initialize() override {
        m_priceHistory.clear();
    }

    void shutdown() override {
        m_priceHistory.clear();
    }

public slots:
    void onTick(const IBComm::MarketTick& tick) override {
        auto& history = m_priceHistory[tick.symbol];
        history.append(tick.mid());
        if (history.size() > m_period + 1) {
            history.removeFirst();
        }

        if (history.size() < 2) return;

        double oldPrice = history.first();
        double newPrice = history.last();
        if (oldPrice <= 0.0) return;

        double momentum = (newPrice - oldPrice) / oldPrice;

        if (std::abs(momentum) > m_threshold) {
            Pipeline::Signal signal;
            signal.symbol = tick.symbol;
            signal.direction = (momentum > 0)
                ? Pipeline::Signal::Buy : Pipeline::Signal::Sell;
            signal.confidence = std::min(std::abs(momentum) / m_threshold, 1.0);
            signal.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            signal.timestamp = tick.timestamp;
            signal.alphaBlockId = id();
            emit signalGenerated(signal);
        }
    }

private:
    int m_period = 20;
    double m_threshold = 0.02;
    QMap<QString, QVector<double>> m_priceHistory;
};

} // namespace Blocks

#endif // BLOCKS_MOMENTUMALPHABLOCK_H
