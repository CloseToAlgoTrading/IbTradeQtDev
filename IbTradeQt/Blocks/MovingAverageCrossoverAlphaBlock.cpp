#include "MovingAverageCrossoverAlphaBlock.h"

#include <QJsonObject>
#include <QUuid>
#include <cmath>
namespace Blocks {

MovingAverageCrossoverAlphaBlock::MovingAverageCrossoverAlphaBlock(QObject* parent)
    : IAlphaBlock(parent)
{}

QString MovingAverageCrossoverAlphaBlock::id() const { return QStringLiteral("ma-crossover-alpha"); }
QString MovingAverageCrossoverAlphaBlock::name() const { return QStringLiteral("Moving Average Crossover"); }

QString MovingAverageCrossoverAlphaBlock::description() const
{
    return QStringLiteral("Generates signals when a fast MA crosses above/below a slow MA");
}

QJsonObject MovingAverageCrossoverAlphaBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("fastPeriod")] = m_fastPeriod;
    cfg[QStringLiteral("slowPeriod")] = m_slowPeriod;
    return cfg;
}

void MovingAverageCrossoverAlphaBlock::setConfig(const QJsonObject& config)
{
    m_fastPeriod = config.value(QStringLiteral("fastPeriod")).toInt(10);
    m_slowPeriod = config.value(QStringLiteral("slowPeriod")).toInt(30);
}

void MovingAverageCrossoverAlphaBlock::initialize() { m_closes.clear(); m_prevFastAboveSlow.clear(); }
void MovingAverageCrossoverAlphaBlock::shutdown() { m_closes.clear(); m_prevFastAboveSlow.clear(); }

void MovingAverageCrossoverAlphaBlock::onTick(const Pipeline::MarketTick& tick)
{
    auto& history = m_closes[tick.symbol];
    history.append(tick.mid());
    if (history.size() > m_slowPeriod + 1)
        history.removeFirst();
    checkCrossover(tick.symbol, tick.timestamp);
}

void MovingAverageCrossoverAlphaBlock::onHistoricalBars(const QVector<Pipeline::OHLCVBar>& bars)
{
    if (bars.isEmpty())
        return;
    const QString& symbol = bars.first().symbol;
    auto& history = m_closes[symbol];
    for (const auto& bar : bars) {
        history.append(bar.close);
        while (history.size() > m_slowPeriod + 1)
            history.removeFirst();
        checkCrossover(symbol, bar.timestamp);
    }
}

void MovingAverageCrossoverAlphaBlock::checkCrossover(const QString& symbol, const QDateTime& ts)
{
    const auto& history = m_closes[symbol];
    if (history.size() < m_slowPeriod) return;

    double fastMA = sma(history, m_fastPeriod);
    double slowMA = sma(history, m_slowPeriod);
    bool fastAbove = fastMA > slowMA;
    bool hadPrev = m_prevFastAboveSlow.contains(symbol);
    bool prevAbove = m_prevFastAboveSlow.value(symbol, false);
    m_prevFastAboveSlow[symbol] = fastAbove;

    if (!hadPrev) return;

    if (fastAbove && !prevAbove) {
        emitSignal(symbol, Pipeline::Signal::Buy, fastMA, slowMA, ts);
    } else if (!fastAbove && prevAbove) {
        emitSignal(symbol, Pipeline::Signal::Sell, fastMA, slowMA, ts);
    }
}

void MovingAverageCrossoverAlphaBlock::emitSignal(const QString& symbol, Pipeline::Signal::Direction dir,
                double fastMA, double slowMA, const QDateTime& ts)
{
    Pipeline::Signal sig;
    sig.symbol = symbol;
    sig.direction = dir;
    sig.confidence = std::min(std::abs(fastMA - slowMA) / slowMA * 100.0, 1.0);
    sig.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    sig.timestamp = ts;
    sig.alphaBlockId = id();
    emit Pipeline::IAlphaBlock::signalGenerated(sig);
}

double MovingAverageCrossoverAlphaBlock::sma(const QVector<double>& data, int period)
{
    if (data.size() < period) return 0.0;
    double sum = 0.0;
    for (int i = data.size() - period; i < data.size(); ++i)
        sum += data[i];
    return sum / period;
}

Pipeline::ModelDataList MovingAverageCrossoverAlphaBlock::processSemantic(
    const Pipeline::ModelDataList& in,
    const QString& correlationId)
{
    Q_UNUSED(correlationId);
    // Crossover state is updated in onTick / onHistoricalBars; runner merges tick signals.
    return in;
}

} // namespace Blocks
