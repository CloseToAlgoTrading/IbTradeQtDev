#include "MarketOrderExecutionBlock.h"

#include "../Pipeline/PipelineRuntimeContext.h"
#include "../Pipeline/SemanticModelDataMapper.h"
#include <QDateTime>
#include <QLoggingCategory>
#include <QMap>
#include <QJsonObject>
#include <cmath>

namespace Blocks {

Q_LOGGING_CATEGORY(lcMarketExec, "pipeline.market_exec")

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

void MarketOrderExecutionBlock::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    m_runtimeContext = ctx;
}

void MarketOrderExecutionBlock::execute(const QVector<Pipeline::ExecutionIntent>& intents)
{
    // Running long-only clamp across this batch: intents are built from one snapshot, but sells are
    // ordered first; without updating "current" per fill, duplicate or mis-sized sells can open shorts
    // from flat. Track simulated positions as we go (matches immediate fills in backtest).
    QMap<QString, double> simPos;
    if (m_runtimeContext) {
        for (auto it = m_runtimeContext->holdings.constBegin(); it != m_runtimeContext->holdings.constEnd();
             ++it) {
            const QString k = it.key().trimmed().toUpper();
            simPos[k] = it.value();
        }
    }

    for (Pipeline::ExecutionIntent intent : intents) {
        const QString sym = intent.symbol.trimmed().toUpper();
        intent.symbol = sym;
        if (intent.quantity < 0.0) {
            const double longQty = std::max(0.0, simPos.value(sym, 0.0));
            const double before = intent.quantity;
            intent.quantity = std::max(intent.quantity, -longQty);
            if (before < 0.0 && qFuzzyIsNull(intent.quantity))
                qCDebug(lcMarketExec) << "clamped sell-from-flat to 0" << sym << "raw" << before << "longQty" << longQty;
        }
        if (std::abs(intent.quantity) < m_minQuantity) continue;

        if (m_executionPort) {
            auto result = m_executionPort->placeOrder(intent);
            if (result.has_value()) {
                simPos[sym] = simPos.value(sym, 0.0) + intent.quantity;
                emit orderPlaced(intent.symbol,
                    QString::number(result->orderId));
            } else {
                emit executionError(intent.symbol,
                    QString::fromStdString(result.error().message));
            }
        } else {
            simPos[sym] = simPos.value(sym, 0.0) + intent.quantity;
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
    const QMap<QString, double>& pos = Pipeline::holdingsForBlocks(m_runtimeContext, currentPositions);
    const QVector<Pipeline::ExecutionIntent> intents =
        Pipeline::SemanticMapping::executionIntentsFromModelData(
            in, pos, correlationId, eventTime);
    execute(intents);
}

} // namespace Blocks
