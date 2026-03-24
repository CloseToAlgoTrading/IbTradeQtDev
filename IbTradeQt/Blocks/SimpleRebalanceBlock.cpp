#include "SimpleRebalanceBlock.h"

#include "../Common/IClock.h"
#include "../Pipeline/PipelineRuntimeContext.h"
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <cmath>
#include <optional>

namespace Blocks {

SimpleRebalanceBlock::SimpleRebalanceBlock(QObject* parent)
    : IRebalanceBlock(parent)
{}

QString SimpleRebalanceBlock::id() const { return QStringLiteral("simple-rebalance"); }
QString SimpleRebalanceBlock::name() const { return QStringLiteral("Simple Rebalance"); }

QJsonObject SimpleRebalanceBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("defaultQuantity")] = m_defaultQuantity;
    cfg[QStringLiteral("equalWeight")] = m_equalWeight;
    cfg[QStringLiteral("priceResolution")] = m_priceResolution;
    cfg[QStringLiteral("priceDataSourceId")] = m_priceDataSourceId;
    return cfg;
}

void SimpleRebalanceBlock::setConfig(const QJsonObject& config)
{
    m_defaultQuantity = config.value(QStringLiteral("defaultQuantity")).toDouble(100.0);
    m_equalWeight = config.value(QStringLiteral("equalWeight")).toBool(false);
    m_priceResolution = config.value(QStringLiteral("priceResolution")).toString(QStringLiteral("Day1"));
    m_priceDataSourceId = config.value(QStringLiteral("priceDataSourceId")).toString(QStringLiteral("yahoo"));
}

void SimpleRebalanceBlock::appendFullExitsForDroppedHoldings(
    QVector<Pipeline::TargetPosition>& targets,
    const QMap<QString, double>& currentPositions,
    const QSet<QString>& newLongSelectionUpper,
    const QString& correlationId,
    const QDateTime& eventTime) const
{
    QSet<QString> alreadyHandled;
    alreadyHandled.reserve(targets.size());
    for (const auto& t : targets)
        alreadyHandled.insert(t.symbol.trimmed().toUpper());

    for (auto it = currentPositions.constBegin(); it != currentPositions.constEnd(); ++it) {
        const QString symKey = it.key();
        const QString symUpper = symKey.trimmed().toUpper();
        if (qFuzzyIsNull(it.value()))
            continue;
        if (newLongSelectionUpper.contains(symUpper))
            continue;
        if (alreadyHandled.contains(symUpper))
            continue;

        Pipeline::TargetPosition tp;
        tp.symbol = symKey;
        tp.currentQuantity = it.value();
        tp.targetQuantity = 0.0;
        tp.reason = id();
        tp.correlationId = correlationId;
        tp.timestamp = eventTime;
        targets.append(tp);
    }
}

double SimpleRebalanceBlock::resolvePriceForSymbol(const QString& symbol) const
{
    if (!m_runtimeContext)
        return 0.0;
    const QString sym = symbol.trimmed().toUpper();

    auto historicalClose = [&](const QDateTime& toUtc) -> double {
        if (!m_runtimeContext->historical)
            return 0.0;
        const QDateTime from = toUtc.addYears(-2);
        const QVector<Pipeline::HistoricalBarSnapshot> bars =
            m_runtimeContext->historical->getBars(sym, m_priceResolution, m_priceDataSourceId, from, toUtc);
        if (!bars.isEmpty() && bars.last().close > 0.0)
            return bars.last().close;
        return 0.0;
    };

    // Backtest (SimulatedClock): use the same daily close the alpha used — do not prefer lastTick
    // first. Tick order can leave lastTick missing or stale for some names; equal-weight sizing
    // would then skip those rows (px<=0) and only one symbol would get a target.
    if (m_runtimeContext->clock) {
        const double hc = historicalClose(m_runtimeContext->clock->now().toUTC());
        if (hc > 0.0)
            return hc;
    }

    if (m_runtimeContext->marketData) {
        const std::optional<Pipeline::MarketTick> t = m_runtimeContext->marketData->lastTick(sym);
        if (t) {
            const double m = t->mid();
            if (m > 0.0)
                return m;
            if (t->bid > 0.0 && t->ask > 0.0)
                return (t->bid + t->ask) / 2.0;
        }
    }

    const QDateTime toUtc = m_runtimeContext->clock ? m_runtimeContext->clock->now().toUTC()
                                                    : QDateTime::currentDateTimeUtc();
    const double hc = historicalClose(toUtc);
    if (hc > 0.0)
        return hc;
    return 0.0;
}

QVector<Pipeline::TargetPosition> SimpleRebalanceBlock::rebalance(
    const QVector<Pipeline::Signal>& inputSignals,
    const QMap<QString, double>& currentPositions)
{
    const QMap<QString, double>& pos = Pipeline::holdingsForBlocks(m_runtimeContext, currentPositions);
    QVector<Pipeline::TargetPosition> targets;

    int buyCount = 0;
    QSet<QString> newLongSelectionUpper;
    for (const auto& sig : inputSignals) {
        if (sig.direction == Pipeline::Signal::Buy) {
            ++buyCount;
            newLongSelectionUpper.insert(sig.symbol.trimmed().toUpper());
        }
    }
    const double cap =
        m_runtimeContext ? m_runtimeContext->strategyAllocatedCapital : 0.0;
    const bool useEqual = m_equalWeight && cap > 0.0 && buyCount > 0;

    for (const auto& sig : inputSignals) {
        if (sig.direction == Pipeline::Signal::Hold) continue;

        Pipeline::TargetPosition tp;
        tp.symbol = sig.symbol;
        tp.currentQuantity = pos.value(sig.symbol, 0.0);

        if (useEqual && sig.direction == Pipeline::Signal::Buy) {
            const double px = resolvePriceForSymbol(sig.symbol);
            if (px <= 0.0)
                continue;
            const double notional = cap / static_cast<double>(buyCount);
            const double q = std::floor(notional / px);
            tp.targetQuantity = q;
        } else if (sig.suggestedQuantity > 0.0) {
            if (sig.direction == Pipeline::Signal::Buy)
                tp.targetQuantity = sig.suggestedQuantity;
            else {
                // Long-only: never sell more than held (avoid target < 0 → execution short).
                const double longQty = std::max(0.0, tp.currentQuantity);
                tp.targetQuantity = std::max(0.0, longQty - sig.suggestedQuantity);
                if (longQty <= 0.0)
                    continue;
            }
        } else {
            if (sig.direction == Pipeline::Signal::Buy)
                tp.targetQuantity = m_defaultQuantity;
            else {
                const double longQty = std::max(0.0, tp.currentQuantity);
                tp.targetQuantity = std::max(0.0, longQty - m_defaultQuantity);
                if (longQty <= 0.0)
                    continue;
            }
        }
        tp.reason = sig.alphaBlockId;
        tp.correlationId = sig.correlationId;
        tp.timestamp = sig.timestamp;
        tp.targetQuantity = std::max(0.0, tp.targetQuantity);
        targets.append(tp);
    }

    QDateTime eventTime = QDateTime::currentDateTimeUtc();
    for (const auto& sig : inputSignals) {
        if (sig.timestamp.isValid()) {
            eventTime = sig.timestamp;
            break;
        }
    }
    QString corr;
    for (const auto& sig : inputSignals) {
        if (!sig.correlationId.isEmpty()) {
            corr = sig.correlationId;
            break;
        }
    }
    appendFullExitsForDroppedHoldings(
        targets, pos, newLongSelectionUpper, corr, eventTime);
    return targets;
}

QVector<Pipeline::TargetPosition> SimpleRebalanceBlock::targetsFromModelRows(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId) const
{
    const QMap<QString, double>& pos = Pipeline::holdingsForBlocks(m_runtimeContext, currentPositions);
    QVector<Pipeline::TargetPosition> targets;
    if (!in)
        return targets;

    const QDateTime ts = QDateTime::currentDateTimeUtc();

    int buyCount = 0;
    QSet<QString> newLongSelectionUpper;
    for (const auto& row : *in) {
        if (row.direction == DIRECTION_UNDEFINED && row.amount == 0.0 && row.probability == 0.0)
            continue;
        if (row.direction == DIRECTION_UP) {
            ++buyCount;
            newLongSelectionUpper.insert(row.symbol.trimmed().toUpper());
        }
    }
    const double cap =
        m_runtimeContext ? m_runtimeContext->strategyAllocatedCapital : 0.0;
    const bool useEqual = m_equalWeight && cap > 0.0 && buyCount > 0;

    for (const auto& row : *in) {
        if (row.direction == DIRECTION_UNDEFINED && row.amount == 0.0 && row.probability == 0.0)
            continue;

        Pipeline::Signal::Direction dir = Pipeline::Signal::Hold;
        switch (row.direction) {
            case DIRECTION_UP: dir = Pipeline::Signal::Buy; break;
            case DIRECTION_DOWN: dir = Pipeline::Signal::Sell; break;
            default: dir = Pipeline::Signal::Hold; break;
        }
        if (dir == Pipeline::Signal::Hold)
            continue;

        Pipeline::TargetPosition tp;
        tp.symbol = row.symbol;
        tp.currentQuantity = pos.value(row.symbol, 0.0);

        if (useEqual && dir == Pipeline::Signal::Buy) {
            const double px = resolvePriceForSymbol(row.symbol);
            if (px <= 0.0)
                continue;
            const double notional = cap / static_cast<double>(buyCount);
            const double q = std::floor(notional / px);
            tp.targetQuantity = q;
        } else if (row.amount > 0.0) {
            // UP: alpha / absolute suggested size. DOWN: sell delta from alpha (not from rebalance delta-encoding).
            if (dir == Pipeline::Signal::Buy)
                tp.targetQuantity = row.amount;
            else {
                const double longQty = std::max(0.0, tp.currentQuantity);
                tp.targetQuantity = std::max(0.0, longQty - row.amount);
                if (longQty <= 0.0)
                    continue;
            }
        } else {
            if (dir == Pipeline::Signal::Buy)
                tp.targetQuantity = m_defaultQuantity;
            else {
                const double longQty = std::max(0.0, tp.currentQuantity);
                tp.targetQuantity = std::max(0.0, longQty - m_defaultQuantity);
                if (longQty <= 0.0)
                    continue;
            }
        }
        tp.reason = id();
        tp.correlationId = correlationId;
        tp.timestamp = ts;
        tp.targetQuantity = std::max(0.0, tp.targetQuantity);
        targets.append(tp);
    }

    appendFullExitsForDroppedHoldings(
        targets, pos, newLongSelectionUpper, correlationId, ts);
    return targets;
}

QVector<Pipeline::TargetPosition> SimpleRebalanceBlock::processSemanticTargets(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    return targetsFromModelRows(in, currentPositions, correlationId);
}

} // namespace Blocks
