#ifndef PIPELINE_ISELECTIONBLOCK_H
#define PIPELINE_ISELECTIONBLOCK_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

namespace Pipeline {
struct PipelineRuntimeContext;

class ISelectionBlock : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~ISelectionBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual void initialize() = 0;
    virtual void shutdown() = 0;

    /// Injected ports (same bundle as alpha/rebalance/risk/execution) — backtest vs live differ only in adapters.
    virtual void setRuntimeContext(const PipelineRuntimeContext* ctx) { m_runtimeContext = ctx; }
    const PipelineRuntimeContext* runtimeContext() const { return m_runtimeContext; }

    virtual QVector<QString> select(const QVector<QString>& universe) = 0;

signals:
    void selectionComplete(const QVector<QString>& candidates);
    void errorOccurred(const QString& message);

protected:
    const PipelineRuntimeContext* m_runtimeContext = nullptr;
};

} // namespace Pipeline

#endif // PIPELINE_ISELECTIONBLOCK_H
