#include "MeanReversionAlphaBlock.h"

#include <QJsonObject>
#include <QUuid>
#include <cmath>

namespace Blocks {

MeanReversionAlphaBlock::MeanReversionAlphaBlock(QObject* parent)
    : IAlphaBlock(parent)
{}

QString MeanReversionAlphaBlock::id() const { return QStringLiteral("mean-reversion-alpha"); }
QString MeanReversionAlphaBlock::name() const { return QStringLiteral("Mean Reversion Alpha"); }

QString MeanReversionAlphaBlock::description() const
{
    return QStringLiteral("Generates signals when price deviates from rolling mean by configurable std deviations");
}

QJsonObject MeanReversionAlphaBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("period")] = m_period;
    cfg[QStringLiteral("stdDevThreshold")] = m_stdDevThreshold;
    return cfg;
}

void MeanReversionAlphaBlock::setConfig(const QJsonObject& config)
{
    m_period = config.value(QStringLiteral("period")).toInt(20);
    m_stdDevThreshold = config.value(QStringLiteral("stdDevThreshold")).toDouble(2.0);
}

void MeanReversionAlphaBlock::initialize() { m_priceHistory.clear(); }
void MeanReversionAlphaBlock::shutdown() { m_priceHistory.clear(); }

void MeanReversionAlphaBlock::onTick(const Pipeline::MarketTick& tick)
{
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
        signal.direction = (zScore > 0)
            ? Pipeline::Signal::Sell : Pipeline::Signal::Buy;
        signal.confidence = std::min(std::abs(zScore) / (m_stdDevThreshold * 2.0), 1.0);
        signal.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        signal.timestamp = tick.timestamp;
        signal.alphaBlockId = id();
        emit signalGenerated(signal);
    }
}

} // namespace Blocks
