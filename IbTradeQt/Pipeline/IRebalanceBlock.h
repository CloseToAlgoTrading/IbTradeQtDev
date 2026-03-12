#ifndef PIPELINE_IREBALANCEBLOCK_H
#define PIPELINE_IREBALANCEBLOCK_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include "Contracts.h"

namespace Pipeline {

class IRebalanceBlock : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~IRebalanceBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual QVector<TargetPosition> rebalance(
        const QVector<Signal>& inputSignals,
        const QMap<QString, double>& currentPositions
    ) = 0;

signals:
    void rebalanceComplete(const QVector<TargetPosition>& targets);
    void errorOccurred(const QString& message);
};

} // namespace Pipeline

#endif // PIPELINE_IREBALANCEBLOCK_H
