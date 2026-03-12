#ifndef PIPELINE_IRISKBLOCK_H
#define PIPELINE_IRISKBLOCK_H

#include <QObject>
#include <QVector>
#include <QJsonObject>
#include <optional>
#include "Contracts.h"
#include "Scope.h"

namespace Pipeline {

struct RiskDecision {
    enum class Action { Approve, Reject, Modify };

    Action action;
    QString reason;
    std::optional<double> modifiedQuantity; // If Modify: new DELTA quantity (not absolute target)
    QString blockId;
};

class IRiskBlock : public QObject {
    Q_OBJECT
    Q_PROPERTY(Pipeline::Scope scope READ scope CONSTANT)

public:
    using QObject::QObject;
    virtual ~IRiskBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual Scope scope() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual RiskDecision evaluate(
        const TargetPosition& target,
        const QVector<TargetPosition>& otherTargets
    ) = 0;

signals:
    void riskViolation(const QString& symbol, const QString& reason);
};

} // namespace Pipeline

#endif // PIPELINE_IRISKBLOCK_H
