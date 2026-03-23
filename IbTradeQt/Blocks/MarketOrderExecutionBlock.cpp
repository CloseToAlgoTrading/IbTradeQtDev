#include "MarketOrderExecutionBlock.h"

#include "../Pipeline/SemanticModelDataMapper.h"
#include <QDateTime>
#include <QJsonObject>
#include <cmath>

namespace Blocks {

MarketOrderExecutionBlock::MarketOrderExecutionBlock(QObject* parent)
    : IExecutionBlock(parent)
{}

void MarketOrderExecutionBlock::setExecutionPort(Ports::IOrderExecutionPort* port)
{
    m_executionPort = port;
}

QString MarketOrderExecutionBlock::id() const { return QStringLiteral("market-order-execution"); }
QString MarketOrderExecutionBlock::name() const { return QStringLiteral("Market Order Execution"); }

QJsonObject MarketOrderExecutionBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("minQuantity")] = m_minQuantity;
    return cfg;
}

void MarketOrderExecutionBlock::setConfig(const QJsonObject& config)
{
    m_minQuantity = config.value(QStringLiteral("minQuantity")).toDouble(1.0);
}

void MarketOrderExecutionBlock::execute(const QVector<Pipeline::ExecutionIntent>& intents)
{
    for (const auto& intent : intents) {
        if (std::abs(intent.quantity) < m_minQuantity) continue;

        if (m_executionPort) {
            auto result = m_executionPort->placeOrder(intent);
            if (result.has_value()) {
                emit orderPlaced(intent.symbol,
                    QString::number(result->orderId));
            } else {
                emit executionError(intent.symbol,
                    QString::fromStdString(result.error().message));
            }
        } else {
            emit orderPlaced(intent.symbol, QStringLiteral("dry-run"));
        }
    }
}

void MarketOrderExecutionBlock::executeSemantic(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime)
{
    const QVector<Pipeline::ExecutionIntent> intents =
        Pipeline::SemanticMapping::executionIntentsFromModelData(
            in, currentPositions, correlationId, eventTime);
    execute(intents);
}

SimpleRebalanceBlock::SimpleRebalanceBlock(QObject* parent)
    : IRebalanceBlock(parent)
{}

QString SimpleRebalanceBlock::id() const { return QStringLiteral("simple-rebalance"); }
QString SimpleRebalanceBlock::name() const { return QStringLiteral("Simple Rebalance"); }

QJsonObject SimpleRebalanceBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("defaultQuantity")] = m_defaultQuantity;
    return cfg;
}

void SimpleRebalanceBlock::setConfig(const QJsonObject& config)
{
    m_defaultQuantity = config.value(QStringLiteral("defaultQuantity")).toDouble(100.0);
}

QVector<Pipeline::TargetPosition> SimpleRebalanceBlock::rebalance(
    const QVector<Pipeline::Signal>& inputSignals,
    const QMap<QString, double>& currentPositions)
{
    QVector<Pipeline::TargetPosition> targets;
    for (const auto& sig : inputSignals) {
        if (sig.direction == Pipeline::Signal::Hold) continue;

        Pipeline::TargetPosition tp;
        tp.symbol = sig.symbol;
        tp.currentQuantity = currentPositions.value(sig.symbol, 0.0);
        if (sig.suggestedQuantity > 0.0) {
            tp.targetQuantity = (sig.direction == Pipeline::Signal::Buy)
                ? sig.suggestedQuantity : -sig.suggestedQuantity;
        } else {
            tp.targetQuantity = (sig.direction == Pipeline::Signal::Buy)
                ? m_defaultQuantity : -m_defaultQuantity;
        }
        tp.reason = sig.alphaBlockId;
        tp.correlationId = sig.correlationId;
        tp.timestamp = sig.timestamp;
        targets.append(tp);
    }
    return targets;
}

QVector<Pipeline::TargetPosition> SimpleRebalanceBlock::targetsFromModelRows(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId) const
{
    QVector<Pipeline::TargetPosition> targets;
    if (!in)
        return targets;

    const QDateTime ts = QDateTime::currentDateTimeUtc();

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
        tp.currentQuantity = currentPositions.value(row.symbol, 0.0);
        if (row.amount > 0.0) {
            tp.targetQuantity = (dir == Pipeline::Signal::Buy) ? row.amount : -row.amount;
        } else {
            tp.targetQuantity = (dir == Pipeline::Signal::Buy) ? m_defaultQuantity : -m_defaultQuantity;
        }
        tp.reason = id();
        tp.correlationId = correlationId;
        tp.timestamp = ts;
        targets.append(tp);
    }
    return targets;
}

Pipeline::ModelDataList SimpleRebalanceBlock::processSemantic(
    const Pipeline::ModelDataList& in,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId)
{
    const QVector<Pipeline::TargetPosition> targets =
        targetsFromModelRows(in, currentPositions, correlationId);
    return Pipeline::SemanticMapping::modelDataFromTargetPositions(targets);
}

PassAllSelectionBlock::PassAllSelectionBlock(QObject* parent)
    : ISelectionBlock(parent)
{}

QString PassAllSelectionBlock::id() const { return QStringLiteral("pass-all-selection"); }
QString PassAllSelectionBlock::name() const { return QStringLiteral("Pass All Selection"); }

QString PassAllSelectionBlock::description() const
{
    return QStringLiteral("Passes all symbols from universe");
}

QJsonObject PassAllSelectionBlock::config() const { return {}; }
void PassAllSelectionBlock::setConfig(const QJsonObject&) {}
void PassAllSelectionBlock::initialize() {}
void PassAllSelectionBlock::shutdown() {}

QVector<QString> PassAllSelectionBlock::select(const QVector<QString>& universe)
{
    return universe;
}

} // namespace Blocks
