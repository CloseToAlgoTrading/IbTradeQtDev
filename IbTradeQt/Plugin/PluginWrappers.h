#ifndef PLUGIN_PLUGINWRAPPERS_H
#define PLUGIN_PLUGINWRAPPERS_H

#include <memory>

#include <QJsonValue>
#include <QObject>

#include "../Pipeline/IAlphaBlock.h"
#include "../Pipeline/IExecutionBlock.h"
#include "../Pipeline/IRebalanceBlock.h"
#include "../Pipeline/IRiskBlock.h"
#include "../Pipeline/ISelectionBlock.h"
#include "PluginManifest.h"
#include "PluginRuntime.h"

namespace Plugin {

class PluginEventSink {
public:
    virtual ~PluginEventSink() = default;

    virtual const Pipeline::PipelineRuntimeContext* pluginRuntimeContext() const = 0;
    virtual QString pluginOwnerId() const = 0;
    virtual void handlePluginEvent(const QString& eventName, const QJsonValue& payload) = 0;
};

class PluginInstanceFacade;

class PluginSelectionBlockWrapper : public Pipeline::ISelectionBlock, public PluginEventSink {
    Q_OBJECT

public:
    explicit PluginSelectionBlockWrapper(const PluginRuntimePtr& runtime,
                                         const ExtensionManifest& extension,
                                         QObject* parent = nullptr);
    ~PluginSelectionBlockWrapper() override;

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void initialize() override;
    void shutdown() override;
    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx) override;
    QVector<QString> select(const QVector<QString>& universe) override;

    const Pipeline::PipelineRuntimeContext* pluginRuntimeContext() const override;
    QString pluginOwnerId() const override;
    void handlePluginEvent(const QString& eventName, const QJsonValue& payload) override;

private:
    PluginRuntimePtr m_runtime;
    ExtensionManifest m_extension;
    std::unique_ptr<PluginInstanceFacade> m_facade;
    const Pipeline::PipelineRuntimeContext* m_runtimeContext = nullptr;
};

class PluginAlphaBlockWrapper : public Pipeline::IAlphaBlock, public PluginEventSink {
    Q_OBJECT

public:
    explicit PluginAlphaBlockWrapper(const PluginRuntimePtr& runtime,
                                     const ExtensionManifest& extension,
                                     QObject* parent = nullptr);
    ~PluginAlphaBlockWrapper() override;

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void initialize() override;
    void shutdown() override;
    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx) override;
    Pipeline::ModelDataList processSemantic(const Pipeline::ModelDataList& in,
                                            const QString& correlationId) override;
    bool semanticCompletionIsAsync() const override;

public slots:
    void onTick(const Pipeline::MarketTick& tick) override;
    void onBarClose(const Pipeline::OHLCVBar& bar) override;
    void onTickByTick(const Pipeline::TickByTickTrade& trade) override;

public:
    const Pipeline::PipelineRuntimeContext* pluginRuntimeContext() const override;
    QString pluginOwnerId() const override;
    void handlePluginEvent(const QString& eventName, const QJsonValue& payload) override;

private:
    PluginRuntimePtr m_runtime;
    ExtensionManifest m_extension;
    std::unique_ptr<PluginInstanceFacade> m_facade;
    const Pipeline::PipelineRuntimeContext* m_runtimeContext = nullptr;
};

class PluginRebalanceBlockWrapper : public Pipeline::IRebalanceBlock, public PluginEventSink {
    Q_OBJECT

public:
    explicit PluginRebalanceBlockWrapper(const PluginRuntimePtr& runtime,
                                         const ExtensionManifest& extension,
                                         QObject* parent = nullptr);
    ~PluginRebalanceBlockWrapper() override;

    QString id() const override;
    QString name() const override;
    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx) override;
    QVector<Pipeline::TargetPosition> rebalance(
        const QVector<Pipeline::Signal>& inputSignals,
        const QMap<QString, double>& currentPositions) override;

    const Pipeline::PipelineRuntimeContext* pluginRuntimeContext() const override;
    QString pluginOwnerId() const override;
    void handlePluginEvent(const QString& eventName, const QJsonValue& payload) override;

private:
    PluginRuntimePtr m_runtime;
    ExtensionManifest m_extension;
    std::unique_ptr<PluginInstanceFacade> m_facade;
    const Pipeline::PipelineRuntimeContext* m_runtimeContext = nullptr;
};

class PluginRiskBlockWrapper : public Pipeline::IRiskBlock, public PluginEventSink {
    Q_OBJECT

public:
    explicit PluginRiskBlockWrapper(const PluginRuntimePtr& runtime,
                                    const ExtensionManifest& extension,
                                    QObject* parent = nullptr);
    ~PluginRiskBlockWrapper() override;

    QString id() const override;
    QString name() const override;
    Pipeline::Scope scope() const override;
    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx) override;
    void onTick(const Pipeline::MarketTick& tick) override;
    Pipeline::RiskDecision evaluate(const Pipeline::TargetPosition& target,
                                    const QVector<Pipeline::TargetPosition>& allTargets,
                                    const QMap<QString, double>& currentPositions) override;

    const Pipeline::PipelineRuntimeContext* pluginRuntimeContext() const override;
    QString pluginOwnerId() const override;
    void handlePluginEvent(const QString& eventName, const QJsonValue& payload) override;

private:
    PluginRuntimePtr m_runtime;
    ExtensionManifest m_extension;
    std::unique_ptr<PluginInstanceFacade> m_facade;
    const Pipeline::PipelineRuntimeContext* m_runtimeContext = nullptr;
};

class PluginExecutionBlockWrapper : public Pipeline::IExecutionBlock, public PluginEventSink {
    Q_OBJECT

public:
    explicit PluginExecutionBlockWrapper(const PluginRuntimePtr& runtime,
                                         const ExtensionManifest& extension,
                                         QObject* parent = nullptr);
    ~PluginExecutionBlockWrapper() override;

    QString id() const override;
    QString name() const override;
    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx) override;

public slots:
    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override;

public:
    const Pipeline::PipelineRuntimeContext* pluginRuntimeContext() const override;
    QString pluginOwnerId() const override;
    void handlePluginEvent(const QString& eventName, const QJsonValue& payload) override;

private:
    PluginRuntimePtr m_runtime;
    ExtensionManifest m_extension;
    std::unique_ptr<PluginInstanceFacade> m_facade;
    const Pipeline::PipelineRuntimeContext* m_runtimeContext = nullptr;
};

} // namespace Plugin

#endif // PLUGIN_PLUGINWRAPPERS_H
