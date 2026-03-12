#ifndef BLOCKS_LIMITORDEREXECUTIONBLOCK_H
#define BLOCKS_LIMITORDEREXECUTIONBLOCK_H

#include "../Pipeline/IExecutionBlock.h"
#include "../Ports/IOrderExecutionPort.h"
#include <cmath>

namespace Blocks {

class LimitOrderExecutionBlock : public Pipeline::IExecutionBlock {
    Q_OBJECT

public:
    explicit LimitOrderExecutionBlock(QObject* parent = nullptr)
        : IExecutionBlock(parent) {}

    void setExecutionPort(Ports::IOrderExecutionPort* port) {
        m_executionPort = port;
    }

    QString id() const override { return "limit-order-execution"; }
    QString name() const override { return "Limit Order Execution"; }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["minQuantity"] = m_minQuantity;
        cfg["limitOffset"] = m_limitOffset;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_minQuantity = config.value("minQuantity").toDouble(1.0);
        m_limitOffset = config.value("limitOffset").toDouble(0.01);
    }

public slots:
    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override {
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
                emit orderPlaced(intent.symbol, "dry-run-limit");
            }
        }
    }

private:
    Ports::IOrderExecutionPort* m_executionPort = nullptr;
    double m_minQuantity = 1.0;
    double m_limitOffset = 0.01;
};

} // namespace Blocks

#endif // BLOCKS_LIMITORDEREXECUTIONBLOCK_H
