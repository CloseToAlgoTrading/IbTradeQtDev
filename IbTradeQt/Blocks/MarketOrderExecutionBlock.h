#ifndef BLOCKS_MARKETORDEREXECUTIONBLOCK_H
#define BLOCKS_MARKETORDEREXECUTIONBLOCK_H

#include <QDateTime>
#include <QJsonObject>
#include "../Pipeline/IExecutionBlock.h"
#include "../Pipeline/IRebalanceBlock.h"
#include "../Pipeline/ISelectionBlock.h"
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

private:
    Ports::IOrderExecutionPort* m_executionPort = nullptr;
    double m_minQuantity = 1.0;
};

class SimpleRebalanceBlock : public Pipeline::IRebalanceBlock {
    Q_OBJECT

public:
    explicit SimpleRebalanceBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    QVector<Pipeline::TargetPosition> rebalance(
        const QVector<Pipeline::Signal>& inputSignals,
        const QMap<QString, double>& currentPositions) override;

    Pipeline::ModelDataList processSemantic(
        const Pipeline::ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId) override;

private:
    QVector<Pipeline::TargetPosition> targetsFromModelRows(
        const Pipeline::ModelDataList& in,
        const QMap<QString, double>& currentPositions,
        const QString& correlationId) const;

    double m_defaultQuantity = 100.0;
};

class PassAllSelectionBlock : public Pipeline::ISelectionBlock {
    Q_OBJECT

public:
    explicit PassAllSelectionBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void initialize() override;
    void shutdown() override;

    QVector<QString> select(const QVector<QString>& universe) override;
};

} // namespace Blocks

#endif // BLOCKS_MARKETORDEREXECUTIONBLOCK_H
