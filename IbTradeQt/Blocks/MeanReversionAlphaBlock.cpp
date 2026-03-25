#include "MeanReversionAlphaBlock.h"

#include "../Pipeline/BlockSubscriptionUtils.h"
#include "../Pipeline/IHistoricalRead.h"
#include "../Pipeline/IDataSubscriptionPort.h"
#include "../Pipeline/PipelineRuntimeContext.h"
#include "UnifiedModelData.h"
#include <QDateTime>
#include <QJsonObject>
#include <QUuid>
#include <cmath>
#include <limits>

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
    cfg[QStringLiteral("resolution")] = m_resolution;
    cfg[QStringLiteral("dataSourceId")] = m_dataSourceId;
    cfg[QStringLiteral("lookbackYears")] = m_lookbackYears;
    return cfg;
}

void MeanReversionAlphaBlock::setConfig(const QJsonObject& config)
{
    m_period = config.value(QStringLiteral("period")).toInt(20);
    m_stdDevThreshold = config.value(QStringLiteral("stdDevThreshold")).toDouble(2.0);
    m_resolution = config.value(QStringLiteral("resolution")).toString(QStringLiteral("Day1"));
    m_dataSourceId = config.value(QStringLiteral("dataSourceId")).toString(QStringLiteral("yahoo"));
    m_lookbackYears = config.value(QStringLiteral("lookbackYears")).toInt(1);
}

void MeanReversionAlphaBlock::initialize() { m_priceHistory.clear(); }
void MeanReversionAlphaBlock::shutdown() { m_priceHistory.clear(); }

double MeanReversionAlphaBlock::zScoreFromCloseHistory(const QVector<double>& history) const
{
    if (history.size() < m_period)
        return std::numeric_limits<double>::quiet_NaN();

    double mean = 0.0;
    for (double p : history)
        mean += p;
    mean /= history.size();

    double variance = 0.0;
    for (double p : history)
        variance += (p - mean) * (p - mean);
    variance /= history.size();
    const double stdDev = std::sqrt(variance);

    if (stdDev < 1e-10)
        return std::numeric_limits<double>::quiet_NaN();

    const double currentPrice = history.last();
    return (currentPrice - mean) / stdDev;
}

double MeanReversionAlphaBlock::zScoreForSymbol(const QString& symbol) const
{
    const auto it = m_priceHistory.constFind(symbol);
    if (it == m_priceHistory.constEnd() || it->size() < m_period)
        return std::numeric_limits<double>::quiet_NaN();
    return zScoreFromCloseHistory(*it);
}

double MeanReversionAlphaBlock::zScoreFromHistoricalBars(const QString& symbol) const
{
    if (!runtimeContext() || !runtimeContext()->historical)
        return std::numeric_limits<double>::quiet_NaN();
    const QString sym = symbol.trimmed().toUpper();
    if (sym.isEmpty())
        return std::numeric_limits<double>::quiet_NaN();

    const QDateTime to = QDateTime::currentDateTimeUtc();
    const QDateTime from = to.addYears(-m_lookbackYears);
    const QVector<Pipeline::HistoricalBarSnapshot> bars =
        runtimeContext()->historical->getBars(sym, m_resolution, m_dataSourceId, from, to,
                                              runtimeContext()->historicalReadPolicyDefault);
    if (bars.size() < m_period)
        return std::numeric_limits<double>::quiet_NaN();
    const int take = qMin(m_period + 1, bars.size());
    QVector<double> closes;
    closes.reserve(take);
    for (int i = bars.size() - take; i < bars.size(); ++i)
        closes.append(bars[i].close);
    return zScoreFromCloseHistory(closes);
}

void MeanReversionAlphaBlock::onTick(const Pipeline::MarketTick& tick)
{
    auto& history = m_priceHistory[tick.symbol];
    history.append(tick.mid());
    if (history.size() > m_period + 1) {
        history.removeFirst();
    }

    const double zScore = zScoreForSymbol(tick.symbol);
    if (std::isnan(zScore) || std::abs(zScore) <= m_stdDevThreshold)
        return;

    Pipeline::Signal signal;
    signal.symbol = tick.symbol;
    signal.direction = (zScore > 0) ? Pipeline::Signal::Sell : Pipeline::Signal::Buy;
    signal.confidence = std::min(std::abs(zScore) / (m_stdDevThreshold * 2.0), 1.0);
    signal.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    signal.timestamp = tick.timestamp;
    signal.alphaBlockId = id();
    emit signalGenerated(signal);
}

Pipeline::ModelDataList MeanReversionAlphaBlock::processSemantic(
    const Pipeline::ModelDataList& in,
    const QString& correlationId)
{
    Q_UNUSED(correlationId);
    if (!in || in->isEmpty())
        return in;

    Pipeline::ModelDataList out = createDataList();
    for (const auto& row : *in) {
        double zScore = zScoreForSymbol(row.symbol);
        if (std::isnan(zScore))
            zScore = zScoreFromHistoricalBars(row.symbol);
        if (std::isnan(zScore) || std::abs(zScore) <= m_stdDevThreshold)
            continue;

        out->append(UnifiedModelData(
            row.symbol,
            zScore > 0 ? DIRECTION_DOWN : DIRECTION_UP,
            std::min(std::abs(zScore) / (m_stdDevThreshold * 2.0), 1.0),
            0.0,
            0.0));
    }
    if (runtimeContext() && runtimeContext()->subscription) {
        const QString oid = Pipeline::subscriptionOwnerId(this, QStringLiteral("alpha:"), id());
        if (out->isEmpty()) {
            runtimeContext()->subscription->clearOwner(oid);
        } else {
            QVector<QString> syms;
            syms.reserve(out->size());
            for (const auto& row : *out)
                syms.append(row.symbol);
            runtimeContext()->subscription->setDesiredSymbols(oid, syms);
        }
    }
    return out;
}

} // namespace Blocks
