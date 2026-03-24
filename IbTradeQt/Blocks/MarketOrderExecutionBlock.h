#ifndef BLOCKS_MARKETORDEREXECUTIONBLOCK_H
#define BLOCKS_MARKETORDEREXECUTIONBLOCK_H

#include <QDateTime>
#include <QJsonObject>
#include "../Pipeline/IExecutionBlock.h"

namespace Pipeline {
struct PipelineRuntimeContext;
}
#include "../Ports/IOrderExecutionPort.h"

namespace Blocks {

class MarketOrderExecutionBlock : public Pipeline::IExecutionBlock {
    Q_OBJECT

public:
    explicit MarketOrderExecutionBlock(QObject* parent = nullptr);

    void setExecutionPort(Ports::IOrderExecutionPort* port);

    QString id() const override;
    QString name() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

public slots:
    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override;

    void executeSemantic(
        const Pipeline::ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId,
        const QDateTime& eventTime) override;

    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx) override;

private:
    Ports::IOrderExecutionPort* m_executionPort = nullptr;
    const Pipeline::PipelineRuntimeContext* m_runtimeContext = nullptr;
    double m_minQuantity = 1.0;
};

} // namespace Blocks

#endif // BLOCKS_MARKETORDEREXECUTIONBLOCK_H
