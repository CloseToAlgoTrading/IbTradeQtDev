#include "LimitOrderExecutionBlock.h"

#include "../Pipeline/SemanticModelDataMapper.h"
#include <QDateTime>
#include <QJsonObject>
#include <cmath>

namespace Blocks {

LimitOrderExecutionBlock::LimitOrderExecutionBlock(QObject* parent)
    : IExecutionBlock(parent)
{}

void LimitOrderExecutionBlock::setExecutionPort(Ports::IOrderExecutionPort* port)
{
    m_executionPort = port;
}

QString LimitOrderExecutionBlock::id() const { return QStringLiteral("limit-order-execution"); }
QString LimitOrderExecutionBlock::name() const { return QStringLiteral("Limit Order Execution"); }

QJsonObject LimitOrderExecutionBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("minQuantity")] = m_minQuantity;
    cfg[QStringLiteral("limitOffset")] = m_limitOffset;
    return cfg;
}

void LimitOrderExecutionBlock::setConfig(const QJsonObject& config)
{
    m_minQuantity = config.value(QStringLiteral("minQuantity")).toDouble(1.0);
    m_limitOffset = config.value(QStringLiteral("limitOffset")).toDouble(0.01);
}

void LimitOrderExecutionBlock::execute(const QVector<Pipeline::ExecutionIntent>& intents)
{
    for (auto intent : intents) {
        if (std::abs(intent.quantity) < m_minQuantity) continue;

        intent.orderType = Pipeline::ExecutionIntent::Limit;
        if (!intent.limitPrice.has_value()) {
            intent.limitPrice = 0.0;
        }

        if (m_executionPort) {
            auto result = m_executionPort->placeOrder(intent);
            if (result.has_value()) {
                emit orderPlaced(intent.symbol, QString::number(result->orderId));
            } else {
                emit executionError(intent.symbol,
                    QString::fromStdString(result.error().message));
            }
        } else {
            emit orderPlaced(intent.symbol, QStringLiteral("dry-run-limit"));
        }
    }
}

void LimitOrderExecutionBlock::executeSemantic(
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

} // namespace Blocks
