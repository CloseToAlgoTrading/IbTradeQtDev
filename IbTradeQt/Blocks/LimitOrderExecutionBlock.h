#ifndef BLOCKS_LIMITORDEREXECUTIONBLOCK_H
#define BLOCKS_LIMITORDEREXECUTIONBLOCK_H

#include <QJsonObject>
#include "../Pipeline/IExecutionBlock.h"
#include "../Ports/IOrderExecutionPort.h"

namespace Blocks {

class LimitOrderExecutionBlock : public Pipeline::IExecutionBlock {
    Q_OBJECT

public:
    explicit LimitOrderExecutionBlock(QObject* parent = nullptr);

    void setExecutionPort(Ports::IOrderExecutionPort* port);

    QString id() const override;
    QString name() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

public slots:
    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override;

private:
    Ports::IOrderExecutionPort* m_executionPort = nullptr;
    double m_minQuantity = 1.0;
    double m_limitOffset = 0.01;
};

} // namespace Blocks

#endif // BLOCKS_LIMITORDEREXECUTIONBLOCK_H
