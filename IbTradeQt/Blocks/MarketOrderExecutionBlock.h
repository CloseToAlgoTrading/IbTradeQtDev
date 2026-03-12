#ifndef BLOCKS_MARKETORDEREXECUTIONBLOCK_H
#define BLOCKS_MARKETORDEREXECUTIONBLOCK_H

#include <QObject>
#include "../Pipeline/IExecutionBlock.h"
#include "../Pipeline/IRebalanceBlock.h"
#include "../Pipeline/ISelectionBlock.h"
#include "../Pipeline/BlockRegistry.h"
#include "../Ports/IOrderExecutionPort.h"

namespace Blocks {

class MarketOrderExecutionBlock : public Pipeline::IExecutionBlock {
    Q_OBJECT

public:
    explicit MarketOrderExecutionBlock(QObject* parent = nullptr)
        : IExecutionBlock(parent) {}

    void setExecutionPort(Ports::IOrderExecutionPort* port) {
        m_executionPort = port;
    }

    QString id() const override { return "market-order-execution"; }
    QString name() const override { return "Market Order Execution"; }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["minQuantity"] = m_minQuantity;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_minQuantity = config.value("minQuantity").toDouble(1.0);
    }

public slots:
    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override {
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
                emit orderPlaced(intent.symbol, "dry-run");
            }
        }
    }

private:
    Ports::IOrderExecutionPort* m_executionPort = nullptr;
    double m_minQuantity = 1.0;
};

class SimpleRebalanceBlock : public Pipeline::IRebalanceBlock {
    Q_OBJECT

public:
    explicit SimpleRebalanceBlock(QObject* parent = nullptr)
        : IRebalanceBlock(parent) {}

    QString id() const override { return "simple-rebalance"; }
    QString name() const override { return "Simple Rebalance"; }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["defaultQuantity"] = m_defaultQuantity;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_defaultQuantity = config.value("defaultQuantity").toDouble(100.0);
    }

    QVector<Pipeline::TargetPosition> rebalance(
        const QVector<Pipeline::Signal>& inputSignals,
        const QMap<QString, double>& currentPositions) override
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

private:
    double m_defaultQuantity = 100.0;
};

class PassAllSelectionBlock : public Pipeline::ISelectionBlock {
    Q_OBJECT

public:
    explicit PassAllSelectionBlock(QObject* parent = nullptr)
        : ISelectionBlock(parent) {}

    QString id() const override { return "pass-all-selection"; }
    QString name() const override { return "Pass All Selection"; }
    QString description() const override { return "Passes all symbols from universe"; }

    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}
    void initialize() override {}
    void shutdown() override {}

    QVector<QString> select(const QVector<QString>& universe) override {
        return universe;
    }
};

} // namespace Blocks

#endif // BLOCKS_MARKETORDEREXECUTIONBLOCK_H
