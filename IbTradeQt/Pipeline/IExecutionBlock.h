#ifndef PIPELINE_IEXECUTIONBLOCK_H
#define PIPELINE_IEXECUTIONBLOCK_H

#include <QObject>
#include <QVector>
#include <QJsonObject>
#include "Contracts.h"

namespace Pipeline {

class IExecutionBlock : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~IExecutionBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

public slots:
    virtual void execute(const QVector<ExecutionIntent>& intents) = 0;

signals:
    void orderPlaced(const QString& symbol, const QString& orderId);
    void executionError(const QString& symbol, const QString& error);
};

} // namespace Pipeline

#endif // PIPELINE_IEXECUTIONBLOCK_H
