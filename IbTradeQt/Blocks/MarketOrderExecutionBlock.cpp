#include "MarketOrderExecutionBlock.h"

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
        tp.targetQuantity = (sig.direction == Pipeline::Signal::Buy)
            ? m_defaultQuantity : -m_defaultQuantity;
        tp.reason = sig.alphaBlockId;
        tp.correlationId = sig.correlationId;
        tp.timestamp = sig.timestamp;
        targets.append(tp);
    }
    return targets;
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
