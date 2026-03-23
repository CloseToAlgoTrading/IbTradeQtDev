#include "LimitOrderExecutionBlock.h"

#include "../Pipeline/PipelineRuntimeContext.h"
#include "../Pipeline/SemanticModelDataMapper.h"
#include <QDateTime>
#include <QJsonObject>
#include <cmath>
#include <optional>

namespace Blocks {

LimitOrderExecutionBlock::LimitOrderExecutionBlock(QObject* parent)
    : IExecutionBlock(parent)
{}

void LimitOrderExecutionBlock::setExecutionPort(Ports::IOrderExecutionPort* port)
{
    m_executionPort = port;
}

void LimitOrderExecutionBlock::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    m_runtimeContext = ctx;
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
        if (!intent.limitPrice.has_value() || intent.limitPrice.value() <= 0.0) {
            double ref = 0.0;
            if (m_runtimeContext && m_runtimeContext->marketData) {
                const std::optional<Pipeline::MarketTick> t =
                    m_runtimeContext->marketData->lastTick(intent.symbol);
                if (t) {
                    ref = t->mid();
                    if (ref <= 0.0 && t->bid > 0.0 && t->ask > 0.0)
                        ref = (t->bid + t->ask) / 2.0;
                }
            }
            if (ref <= 0.0) {
                emit executionError(intent.symbol,
                    QStringLiteral("Limit order needs limitPrice or last tick for limitOffset"));
                continue;
            }
            const double off = m_limitOffset;
            if (intent.quantity > 0.0)
                intent.limitPrice = ref * (1.0 - off);
            else
                intent.limitPrice = ref * (1.0 + off);
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
    const QMap<QString, double>& pos = Pipeline::holdingsForBlocks(m_runtimeContext, currentPositions);
    const QVector<Pipeline::ExecutionIntent> intents =
        Pipeline::SemanticMapping::executionIntentsFromModelData(
            in, pos, correlationId, eventTime);
    execute(intents);
}

} // namespace Blocks
