#include "MomentumAlphaBlock.h"

#include <QJsonObject>
#include <QUuid>
#include <cmath>

namespace Blocks {

MomentumAlphaBlock::MomentumAlphaBlock(QObject* parent)
    : IAlphaBlock(parent)
{}

QString MomentumAlphaBlock::id() const { return QStringLiteral("momentum-alpha"); }
QString MomentumAlphaBlock::name() const { return QStringLiteral("Momentum Alpha"); }

QString MomentumAlphaBlock::description() const
{
    return QStringLiteral("Generates Buy/Sell signals based on price momentum over a configurable window");
}

QJsonObject MomentumAlphaBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("period")] = m_period;
    cfg[QStringLiteral("threshold")] = m_threshold;
    return cfg;
}

void MomentumAlphaBlock::setConfig(const QJsonObject& config)
{
    m_period = config.value(QStringLiteral("period")).toInt(20);
    m_threshold = config.value(QStringLiteral("threshold")).toDouble(0.02);
}

void MomentumAlphaBlock::initialize() { m_priceHistory.clear(); }
void MomentumAlphaBlock::shutdown() { m_priceHistory.clear(); }

void MomentumAlphaBlock::onTick(const Pipeline::MarketTick& tick)
{
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

} // namespace Blocks
