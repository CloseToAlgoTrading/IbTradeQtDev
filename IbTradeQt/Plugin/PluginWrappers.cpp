#include "PluginWrappers.h"

#include <cstring>
#include <mutex>
#include <vector>

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QPointer>

#include "../Common/IClock.h"
#include "../Pipeline/IDataSubscriptionPort.h"
#include "../Ports/IOrderExecutionPort.h"
#include "../plugins/api/ibtrade_plugin_api.h"
#include "PluginDataBridge.h"

Q_LOGGING_CATEGORY(lcPluginWrappers, "plugin.wrappers")

namespace Plugin {

namespace {

QString jsonToString(const QJsonValue& value)
{
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if (value.isString()) {
        return value.toString();
    }
    return QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact));
}

QString errorMessageFromPayload(const QJsonValue& payload)
{
    if (payload.isObject()) {
        return payload.toObject().value(QStringLiteral("message")).toString();
    }
    if (payload.isString()) {
        return payload.toString();
    }
    return QStringLiteral("Plugin reported an error");
}

ibtrade_owned_string_v1 makeOwnedString(const QString& value)
{
    QByteArray utf8 = value.toUtf8();
    char* raw = new char[utf8.size() + 1];
    std::memcpy(raw, utf8.constData(), static_cast<size_t>(utf8.size()));
    raw[utf8.size()] = '\0';

    ibtrade_owned_string_v1 out{};
    out.data = raw;
    out.size = static_cast<size_t>(utf8.size());
    out.release = [](const char* data, size_t, void*) {
        delete[] data;
    };
    out.user_data = nullptr;
    return out;
}

QString ownedStringToQString(ibtrade_owned_string_v1 value)
{
    const QString out = value.data
        ? QString::fromUtf8(value.data, static_cast<int>(value.size))
        : QString();
    if (value.release && value.data) {
        value.release(value.data, value.size, value.user_data);
    }
    return out;
}

Error makePluginError(const QString& message, const char* context)
{
    return Error{
        ErrorCode::ConfigurationError,
        message.toStdString(),
        context
    };
}

} // namespace

class PluginInstanceFacade;

struct PluginCallbackContext {
    std::atomic<PluginInstanceFacade*> facade{nullptr};
    std::atomic<PluginEventSink*> sink{nullptr};
};

class PluginInstanceFacade {
public:
    PluginInstanceFacade(const PluginRuntimePtr& runtime,
                         const ExtensionManifest& extension,
                         PluginEventSink* sink)
        : m_runtime(runtime)
        , m_extension(extension)
        , m_sink(sink)
        , m_callbackContext(std::make_shared<PluginCallbackContext>())
    {
        m_callbackContext->facade.store(this, std::memory_order_release);
        m_callbackContext->sink.store(sink, std::memory_order_release);
    }

    ~PluginInstanceFacade()
    {
        destroyInstance();
    }

    void setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
    {
        Q_UNUSED(ctx);
    }

    QJsonObject config() const
    {
        return m_config;
    }

    void setCachedState(const QJsonObject& state)
    {
        m_lastState = state;
    }

    QJsonObject lastState() const
    {
        return m_lastState;
    }

    bool supportsAsyncSemantic() const
    {
        return m_extension.supportsAsyncSemantic;
    }

    Expected<void, Error> setConfig(const QJsonObject& config)
    {
        m_config = config;
        auto ready = ensureInstanceCreated();
        if (!ready) {
            return ready;
        }
        if (!m_runtime->api->set_config_json) {
            return {};
        }
        ibtrade_owned_string_v1 errorOut{};
        const QString configJson = QString::fromUtf8(
            QJsonDocument(config).toJson(QJsonDocument::Compact));
        const int ok = m_runtime->api->set_config_json(
            m_instance,
            configJson.toUtf8().constData(),
            &errorOut);
        if (!ok) {
            return make_unexpected(makePluginError(
                ownedStringToQString(errorOut),
                "PluginInstanceFacade::setConfig"));
        }
        return {};
    }

    Expected<void, Error> initialize()
    {
        auto ready = ensureInstanceCreated();
        if (!ready) {
            return ready;
        }
        if (m_initialized || !m_runtime->api->initialize) {
            m_initialized = true;
            return {};
        }
        ibtrade_owned_string_v1 errorOut{};
        const int ok = m_runtime->api->initialize(m_instance, &errorOut);
        if (!ok) {
            return make_unexpected(makePluginError(
                ownedStringToQString(errorOut),
                "PluginInstanceFacade::initialize"));
        }
        m_initialized = true;
        refreshState();
        return {};
    }

    void shutdown()
    {
        if (m_instance && m_initialized && m_runtime->api->shutdown) {
            m_runtime->api->shutdown(m_instance);
        }
        m_initialized = false;
    }

    Expected<QJsonValue, Error> invoke(const QString& operation,
                                       const QJsonValue& input = QJsonObject())
    {
        auto init = ensureInitialized();
        if (!init) {
            return make_unexpected(init.error());
        }

        const QString inputJson = jsonToString(input);
        ibtrade_owned_string_v1 errorOut{};
        ibtrade_owned_string_v1 result = m_runtime->api->invoke_json(
            m_instance,
            operation.toUtf8().constData(),
            inputJson.toUtf8().constData(),
            &errorOut);

        const QString errorMsg = ownedStringToQString(errorOut);
        if (!errorMsg.isEmpty()) {
            return make_unexpected(makePluginError(
                errorMsg,
                "PluginInstanceFacade::invoke"));
        }

        const QString resultJson = ownedStringToQString(result);
        if (resultJson.trimmed().isEmpty()) {
            refreshState();
            return QJsonObject();
        }

        const QJsonDocument doc = QJsonDocument::fromJson(resultJson.toUtf8());
        refreshState();
        if (doc.isObject()) {
            return doc.object();
        }
        if (doc.isArray()) {
            return doc.array();
        }
        return QJsonValue(resultJson);
    }

private:
    static void retainCallbackContextUntilProcessExit(
        const std::shared_ptr<PluginCallbackContext>& context)
    {
        static std::mutex mutex;
        static std::vector<std::shared_ptr<PluginCallbackContext>> retiredContexts;

        std::lock_guard<std::mutex> lock(mutex);
        retiredContexts.push_back(context);
    }

    static PluginCallbackContext* callbackContext(void* userData)
    {
        return static_cast<PluginCallbackContext*>(userData);
    }

    static PluginInstanceFacade* self(void* userData)
    {
        auto* context = callbackContext(userData);
        if (!context) {
            return nullptr;
        }
        return context->facade.load(std::memory_order_acquire);
    }

    static PluginEventSink* sink(void* userData)
    {
        auto* context = callbackContext(userData);
        if (!context) {
            return nullptr;
        }
        return context->sink.load(std::memory_order_acquire);
    }

    Expected<void, Error> ensureInstanceCreated()
    {
        if (m_instance) {
            return {};
        }

        m_hostServices = {};
        m_hostServices.abi_version = IBTRADE_PLUGIN_ABI_VERSION_V1;
        m_hostServices.user_data = m_callbackContext.get();
        m_hostServices.current_time_iso8601 = &PluginInstanceFacade::currentTimeIso8601;
        m_hostServices.read_market_data_json = &PluginInstanceFacade::readMarketDataJson;
        m_hostServices.read_historical_bars_json = &PluginInstanceFacade::readHistoricalBarsJson;
        m_hostServices.read_holdings_json = &PluginInstanceFacade::readHoldingsJson;
        m_hostServices.set_subscription_request_json = &PluginInstanceFacade::setSubscriptionRequestJson;
        m_hostServices.place_order_json = &PluginInstanceFacade::placeOrderJson;
        m_hostServices.cancel_all_pending = &PluginInstanceFacade::cancelAllPending;
        m_hostServices.log_message = &PluginInstanceFacade::logMessage;
        m_hostServices.emit_event_json = &PluginInstanceFacade::emitEventJson;

        ibtrade_owned_string_v1 errorOut{};
        m_instance = m_runtime->api->create_extension(
            m_extension.extensionId.toUtf8().constData(),
            &m_hostServices,
            &errorOut);
        if (!m_instance) {
            return make_unexpected(makePluginError(
                ownedStringToQString(errorOut),
                "PluginInstanceFacade::ensureInstanceCreated"));
        }
        ++m_runtime->activeInstances;

        if (!m_config.isEmpty()) {
            auto applied = setConfig(m_config);
            if (!applied) {
                return applied;
            }
        }
        return {};
    }

    Expected<void, Error> ensureInitialized()
    {
        auto created = ensureInstanceCreated();
        if (!created) {
            return created;
        }
        if (m_initialized) {
            return {};
        }
        return initialize();
    }

    void destroyInstance()
    {
        if (!m_instance) {
            return;
        }
        shutdown();
        if (m_runtime->api->destroy_extension) {
            m_runtime->api->destroy_extension(m_instance);
        }
        m_instance = nullptr;
        --m_runtime->activeInstances;
        if (m_callbackContext) {
            m_callbackContext->sink.store(nullptr, std::memory_order_release);
            m_callbackContext->facade.store(nullptr, std::memory_order_release);
            retainCallbackContextUntilProcessExit(m_callbackContext);
            m_callbackContext.reset();
        }
    }

    void refreshState()
    {
        if (!m_instance || !m_runtime->api->get_state_json) {
            return;
        }
        const QString raw = ownedStringToQString(m_runtime->api->get_state_json(m_instance));
        if (raw.trimmed().isEmpty()) {
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
        if (doc.isObject()) {
            m_lastState = doc.object();
        }
    }

    QString ownerId() const
    {
        return m_sink ? m_sink->pluginOwnerId() : m_extension.extensionId;
    }

    const Pipeline::PipelineRuntimeContext* ctx() const
    {
        return m_sink ? m_sink->pluginRuntimeContext() : nullptr;
    }

    static ibtrade_owned_string_v1 currentTimeIso8601(void* userData)
    {
        const PluginInstanceFacade* me = self(userData);
        if (!me) {
            return makeOwnedString(QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
        }
        const auto* runtimeCtx = me->ctx();
        const QDateTime now = (runtimeCtx && runtimeCtx->clock)
            ? runtimeCtx->clock->now()
            : QDateTime::currentDateTime();
        return makeOwnedString(now.toString(Qt::ISODateWithMs));
    }

    static ibtrade_owned_string_v1 readMarketDataJson(void* userData, const char* symbol)
    {
        const PluginInstanceFacade* me = self(userData);
        if (!me) {
            return makeOwnedString(QStringLiteral("{}"));
        }
        const auto* runtimeCtx = me->ctx();
        if (!runtimeCtx || !runtimeCtx->marketData || !symbol) {
            return makeOwnedString(QStringLiteral("{}"));
        }
        const auto tick = runtimeCtx->marketData->lastTick(QString::fromUtf8(symbol));
        if (!tick) {
            return makeOwnedString(QStringLiteral("{}"));
        }
        return makeOwnedString(QString::fromUtf8(
            QJsonDocument(PluginDataBridge::toJson(*tick)).toJson(QJsonDocument::Compact)));
    }

    static ibtrade_owned_string_v1 readHistoricalBarsJson(void* userData,
                                                          const char* requestJson)
    {
        const PluginInstanceFacade* me = self(userData);
        if (!me) {
            return makeOwnedString(QStringLiteral("[]"));
        }
        const auto* runtimeCtx = me->ctx();
        if (!runtimeCtx || !runtimeCtx->historical || !requestJson) {
            return makeOwnedString(QStringLiteral("[]"));
        }

        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(requestJson));
        const QJsonObject req = doc.object();
        QVector<Pipeline::HistoricalBarSnapshot> bars =
            runtimeCtx->historical->getBars(
                req.value(QStringLiteral("symbol")).toString(),
                req.value(QStringLiteral("resolution")).toString(),
                req.value(QStringLiteral("dataSourceId")).toString(),
                QDateTime::fromString(req.value(QStringLiteral("from")).toString(), Qt::ISODateWithMs),
                QDateTime::fromString(req.value(QStringLiteral("to")).toString(), Qt::ISODateWithMs),
                runtimeCtx->historicalReadPolicyDefault);

        QJsonArray out;
        for (const auto& bar : bars) {
            QJsonObject obj;
            obj[QStringLiteral("timestamp")] = bar.timestamp.toString(Qt::ISODateWithMs);
            obj[QStringLiteral("open")] = bar.open;
            obj[QStringLiteral("high")] = bar.high;
            obj[QStringLiteral("low")] = bar.low;
            obj[QStringLiteral("close")] = bar.close;
            obj[QStringLiteral("volume")] = bar.volume;
            out.push_back(obj);
        }
        return makeOwnedString(QString::fromUtf8(QJsonDocument(out).toJson(QJsonDocument::Compact)));
    }

    static ibtrade_owned_string_v1 readHoldingsJson(void* userData)
    {
        const PluginInstanceFacade* me = self(userData);
        if (!me) {
            return makeOwnedString(QStringLiteral("{}"));
        }
        const auto* runtimeCtx = me->ctx();
        return makeOwnedString(QString::fromUtf8(
            QJsonDocument(PluginDataBridge::toJson(runtimeCtx ? runtimeCtx->holdings : QMap<QString, double>{}))
                .toJson(QJsonDocument::Compact)));
    }

    static int setSubscriptionRequestJson(void* userData,
                                          const char* ownerId,
                                          const char* requestJson,
                                          ibtrade_owned_string_v1* errorOut)
    {
        PluginInstanceFacade* me = self(userData);
        if (!me) {
            if (errorOut) {
                *errorOut = makeOwnedString(QStringLiteral("Plugin instance is no longer active"));
            }
            return 0;
        }
        const auto* runtimeCtx = me->ctx();
        if (!runtimeCtx || !runtimeCtx->subscription) {
            return 1;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(requestJson ? requestJson : ""));
        const QJsonObject req = doc.object();
        const QVector<QString> symbols =
            PluginDataBridge::stringVectorFromJson(req.value(QStringLiteral("symbols")).toArray());
        const quint32 kindMask =
            static_cast<quint32>(req.value(QStringLiteral("kindMask")).toInt(1));
        runtimeCtx->subscription->setDesiredSymbolsWithKinds(
            ownerId && *ownerId ? QString::fromUtf8(ownerId) : me->ownerId(),
            symbols,
            kindMask);
        if (errorOut) {
            *errorOut = {};
        }
        return 1;
    }

    static int placeOrderJson(void* userData,
                              const char* intentJson,
                              ibtrade_owned_string_v1* errorOut)
    {
        PluginInstanceFacade* me = self(userData);
        if (!me) {
            if (errorOut) {
                *errorOut = makeOwnedString(QStringLiteral("Plugin instance is no longer active"));
            }
            return 0;
        }
        const auto* runtimeCtx = me->ctx();
        if (!runtimeCtx || !runtimeCtx->execution || !intentJson) {
            if (errorOut) {
                *errorOut = makeOwnedString(QStringLiteral("Execution service is not available"));
            }
            return 0;
        }

        const Pipeline::ExecutionIntent intent =
            PluginDataBridge::intentFromJson(QJsonDocument::fromJson(QByteArray(intentJson)).object());
        auto result = runtimeCtx->execution->placeOrder(intent);
        if (!result) {
            if (errorOut) {
                *errorOut = makeOwnedString(QString::fromStdString(result.error().message));
            }
            return 0;
        }
        if (errorOut) {
            *errorOut = {};
        }
        return 1;
    }

    static int cancelAllPending(void* userData, ibtrade_owned_string_v1* errorOut)
    {
        PluginInstanceFacade* me = self(userData);
        if (!me) {
            if (errorOut) {
                *errorOut = makeOwnedString(QStringLiteral("Plugin instance is no longer active"));
            }
            return 0;
        }
        const auto* runtimeCtx = me->ctx();
        if (!runtimeCtx || !runtimeCtx->execution) {
            if (errorOut) {
                *errorOut = makeOwnedString(QStringLiteral("Execution service is not available"));
            }
            return 0;
        }
        auto result = runtimeCtx->execution->cancelAllPending();
        if (!result) {
            if (errorOut) {
                *errorOut = makeOwnedString(QString::fromStdString(result.error().message));
            }
            return 0;
        }
        if (errorOut) {
            *errorOut = {};
        }
        return 1;
    }

    static void logMessage(void* userData,
                           ibtrade_log_level_v1 level,
                           const char* message)
    {
        Q_UNUSED(userData);
        const QString msg = QString::fromUtf8(message ? message : "");
        switch (level) {
        case IBTRADE_LOG_DEBUG_V1: qCDebug(lcPluginWrappers) << msg; break;
        case IBTRADE_LOG_INFO_V1: qCInfo(lcPluginWrappers) << msg; break;
        case IBTRADE_LOG_WARNING_V1: qCWarning(lcPluginWrappers) << msg; break;
        case IBTRADE_LOG_ERROR_V1: qCCritical(lcPluginWrappers) << msg; break;
        }
    }

    static void emitEventJson(void* userData,
                              const char* eventName,
                              const char* payloadJson)
    {
        PluginEventSink* eventSink = sink(userData);
        if (!eventSink) {
            return;
        }

        const QString name = QString::fromUtf8(eventName ? eventName : "");
        const QByteArray raw = QByteArray(payloadJson ? payloadJson : "");
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        const QJsonValue payload = doc.isObject()
            ? QJsonValue(doc.object())
            : (doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(QString::fromUtf8(raw)));

        if (auto* sinkObject = dynamic_cast<QObject*>(eventSink)) {
            QPointer<QObject> sinkGuard(sinkObject);
            QMetaObject::invokeMethod(
                sinkObject,
                [sinkGuard, name, payload]() {
                    if (!sinkGuard) {
                        return;
                    }
                    if (auto* liveSink = dynamic_cast<PluginEventSink*>(sinkGuard.data())) {
                        liveSink->handlePluginEvent(name, payload);
                    }
                },
                Qt::QueuedConnection);
        } else {
            eventSink->handlePluginEvent(name, payload);
        }
    }

private:
    PluginRuntimePtr m_runtime;
    ExtensionManifest m_extension;
    PluginEventSink* m_sink = nullptr;
    void* m_instance = nullptr;
    bool m_initialized = false;
    std::shared_ptr<PluginCallbackContext> m_callbackContext;
    ibtrade_host_services_v1 m_hostServices{};
    QJsonObject m_config;
    QJsonObject m_lastState;
};

PluginSelectionBlockWrapper::PluginSelectionBlockWrapper(const PluginRuntimePtr& runtime,
                                                         const ExtensionManifest& extension,
                                                         QObject* parent)
    : ISelectionBlock(parent)
    , m_runtime(runtime)
    , m_extension(extension)
    , m_facade(std::make_unique<PluginInstanceFacade>(runtime, extension, this))
{
    setConfig(extension.defaultConfig);
}

PluginSelectionBlockWrapper::~PluginSelectionBlockWrapper() = default;

QString PluginSelectionBlockWrapper::id() const { return m_extension.extensionId; }
QString PluginSelectionBlockWrapper::name() const { return m_extension.name; }
QString PluginSelectionBlockWrapper::description() const { return m_extension.description; }
QJsonObject PluginSelectionBlockWrapper::config() const { return m_facade->config(); }
void PluginSelectionBlockWrapper::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    Pipeline::ISelectionBlock::setRuntimeContext(ctx);
    m_runtimeContext = ctx;
    m_facade->setRuntimeContext(ctx);
}

void PluginSelectionBlockWrapper::setConfig(const QJsonObject& config)
{
    auto result = m_facade->setConfig(config);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PluginSelectionBlockWrapper::initialize()
{
    auto result = m_facade->initialize();
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PluginSelectionBlockWrapper::shutdown()
{
    m_facade->shutdown();
}

QVector<QString> PluginSelectionBlockWrapper::select(const QVector<QString>& universe)
{
    QJsonObject input;
    QJsonArray arr;
    for (const QString& symbol : universe) {
        arr.push_back(symbol);
    }
    input[QStringLiteral("universe")] = arr;
    auto result = m_facade->invoke(QStringLiteral("select"), input);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return {};
    }

    QVector<QString> selected =
        PluginDataBridge::stringVectorFromJson(result->toObject().value(QStringLiteral("selected")).toArray());
    emit selectionComplete(selected);
    return selected;
}

const Pipeline::PipelineRuntimeContext* PluginSelectionBlockWrapper::pluginRuntimeContext() const
{
    return runtimeContext();
}

QString PluginSelectionBlockWrapper::pluginOwnerId() const
{
    return objectName().isEmpty() ? id() : objectName();
}

void PluginSelectionBlockWrapper::handlePluginEvent(const QString& eventName, const QJsonValue& payload)
{
    if (eventName == QStringLiteral("plugin.error")) {
        emit errorOccurred(errorMessageFromPayload(payload));
    }
}

PluginAlphaBlockWrapper::PluginAlphaBlockWrapper(const PluginRuntimePtr& runtime,
                                                 const ExtensionManifest& extension,
                                                 QObject* parent)
    : IAlphaBlock(parent)
    , m_runtime(runtime)
    , m_extension(extension)
    , m_facade(std::make_unique<PluginInstanceFacade>(runtime, extension, this))
{
    setConfig(extension.defaultConfig);
}

PluginAlphaBlockWrapper::~PluginAlphaBlockWrapper() = default;

QString PluginAlphaBlockWrapper::id() const { return m_extension.extensionId; }
QString PluginAlphaBlockWrapper::name() const { return m_extension.name; }
QString PluginAlphaBlockWrapper::description() const { return m_extension.description; }
QJsonObject PluginAlphaBlockWrapper::config() const { return m_facade->config(); }
bool PluginAlphaBlockWrapper::semanticCompletionIsAsync() const
{
    return m_facade->supportsAsyncSemantic();
}

void PluginAlphaBlockWrapper::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    Pipeline::IAlphaBlock::setRuntimeContext(ctx);
    m_runtimeContext = ctx;
    m_facade->setRuntimeContext(ctx);
}

void PluginAlphaBlockWrapper::setConfig(const QJsonObject& config)
{
    auto result = m_facade->setConfig(config);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PluginAlphaBlockWrapper::initialize()
{
    auto result = m_facade->initialize();
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PluginAlphaBlockWrapper::shutdown()
{
    m_facade->shutdown();
}

Pipeline::ModelDataList PluginAlphaBlockWrapper::processSemantic(const Pipeline::ModelDataList& in,
                                                                 const QString& correlationId)
{
    QJsonObject input;
    input[QStringLiteral("correlationId")] = correlationId;
    input[QStringLiteral("modelData")] = PluginDataBridge::toJson(in);

    auto result = m_facade->invoke(QStringLiteral("process_semantic"), input);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return createDataList();
    }

    if (result->isObject()) {
        const QJsonObject obj = result->toObject();
        return PluginDataBridge::modelDataFromJson(obj.value(QStringLiteral("modelData")).toArray());
    }
    if (result->isArray()) {
        return PluginDataBridge::modelDataFromJson(result->toArray());
    }
    return createDataList();
}

void PluginAlphaBlockWrapper::onTick(const Pipeline::MarketTick& tick)
{
    QJsonObject input;
    input[QStringLiteral("tick")] = PluginDataBridge::toJson(tick);
    auto result = m_facade->invoke(QStringLiteral("on_tick"), input);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PluginAlphaBlockWrapper::onBarClose(const Pipeline::OHLCVBar& bar)
{
    QJsonObject input;
    input[QStringLiteral("bar")] = PluginDataBridge::toJson(bar);
    auto result = m_facade->invoke(QStringLiteral("on_bar_close"), input);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PluginAlphaBlockWrapper::onTickByTick(const Pipeline::TickByTickTrade& trade)
{
    QJsonObject input;
    input[QStringLiteral("trade")] = PluginDataBridge::toJson(trade);
    auto result = m_facade->invoke(QStringLiteral("on_tick_by_tick"), input);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

const Pipeline::PipelineRuntimeContext* PluginAlphaBlockWrapper::pluginRuntimeContext() const
{
    return runtimeContext();
}

QString PluginAlphaBlockWrapper::pluginOwnerId() const
{
    return objectName().isEmpty() ? id() : objectName();
}

void PluginAlphaBlockWrapper::handlePluginEvent(const QString& eventName, const QJsonValue& payload)
{
    if (eventName == QStringLiteral("alpha.signal_generated") && payload.isObject()) {
        emit signalGenerated(PluginDataBridge::signalFromJson(payload.toObject()));
        return;
    }
    if (eventName == QStringLiteral("alpha.semantic_ready") && payload.isObject()) {
        const QJsonObject obj = payload.toObject();
        emit semanticReady(
            PluginDataBridge::modelDataFromJson(obj.value(QStringLiteral("modelData")).toArray()),
            obj.value(QStringLiteral("correlationId")).toString());
        return;
    }
    if (eventName == QStringLiteral("plugin.state") && payload.isObject()) {
        m_facade->setCachedState(payload.toObject());
        return;
    }
    if (eventName == QStringLiteral("plugin.error")) {
        emit errorOccurred(errorMessageFromPayload(payload));
    }
}

PluginRebalanceBlockWrapper::PluginRebalanceBlockWrapper(const PluginRuntimePtr& runtime,
                                                         const ExtensionManifest& extension,
                                                         QObject* parent)
    : IRebalanceBlock(parent)
    , m_runtime(runtime)
    , m_extension(extension)
    , m_facade(std::make_unique<PluginInstanceFacade>(runtime, extension, this))
{
    setConfig(extension.defaultConfig);
}

PluginRebalanceBlockWrapper::~PluginRebalanceBlockWrapper() = default;

QString PluginRebalanceBlockWrapper::id() const { return m_extension.extensionId; }
QString PluginRebalanceBlockWrapper::name() const { return m_extension.name; }
QJsonObject PluginRebalanceBlockWrapper::config() const { return m_facade->config(); }

void PluginRebalanceBlockWrapper::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    Pipeline::IRebalanceBlock::setRuntimeContext(ctx);
    m_runtimeContext = ctx;
    m_facade->setRuntimeContext(ctx);
}

void PluginRebalanceBlockWrapper::setConfig(const QJsonObject& config)
{
    auto result = m_facade->setConfig(config);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

QVector<Pipeline::TargetPosition> PluginRebalanceBlockWrapper::rebalance(
    const QVector<Pipeline::Signal>& inputSignals,
    const QMap<QString, double>& currentPositions)
{
    QJsonObject input;
    input[QStringLiteral("signals")] = PluginDataBridge::toJson(inputSignals);
    input[QStringLiteral("currentPositions")] = PluginDataBridge::toJson(currentPositions);

    auto result = m_facade->invoke(QStringLiteral("rebalance"), input);
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return {};
    }

    QVector<Pipeline::TargetPosition> targets =
        PluginDataBridge::targetsFromJson(result->toObject().value(QStringLiteral("targets")).toArray());
    emit rebalanceComplete(targets);
    return targets;
}

const Pipeline::PipelineRuntimeContext* PluginRebalanceBlockWrapper::pluginRuntimeContext() const
{
    return runtimeContext();
}

QString PluginRebalanceBlockWrapper::pluginOwnerId() const
{
    return objectName().isEmpty() ? id() : objectName();
}

void PluginRebalanceBlockWrapper::handlePluginEvent(const QString& eventName, const QJsonValue& payload)
{
    if (eventName == QStringLiteral("plugin.error")) {
        emit errorOccurred(errorMessageFromPayload(payload));
    }
}

PluginRiskBlockWrapper::PluginRiskBlockWrapper(const PluginRuntimePtr& runtime,
                                               const ExtensionManifest& extension,
                                               QObject* parent)
    : IRiskBlock(parent)
    , m_runtime(runtime)
    , m_extension(extension)
    , m_facade(std::make_unique<PluginInstanceFacade>(runtime, extension, this))
{
    setConfig(extension.defaultConfig);
}

PluginRiskBlockWrapper::~PluginRiskBlockWrapper() = default;

QString PluginRiskBlockWrapper::id() const { return m_extension.extensionId; }
QString PluginRiskBlockWrapper::name() const { return m_extension.name; }
Pipeline::Scope PluginRiskBlockWrapper::scope() const { return m_extension.scope; }
QJsonObject PluginRiskBlockWrapper::config() const { return m_facade->config(); }

void PluginRiskBlockWrapper::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    Pipeline::IRiskBlock::setRuntimeContext(ctx);
    m_runtimeContext = ctx;
    m_facade->setRuntimeContext(ctx);
}

void PluginRiskBlockWrapper::setConfig(const QJsonObject& config)
{
    auto result = m_facade->setConfig(config);
    if (!result) {
        emit riskViolation(id(), QString::fromStdString(result.error().message));
    }
}

void PluginRiskBlockWrapper::onTick(const Pipeline::MarketTick& tick)
{
    QJsonObject input;
    input[QStringLiteral("tick")] = PluginDataBridge::toJson(tick);
    auto result = m_facade->invoke(QStringLiteral("on_tick"), input);
    if (!result) {
        emit riskViolation(tick.symbol, QString::fromStdString(result.error().message));
    }
}

Pipeline::RiskDecision PluginRiskBlockWrapper::evaluate(
    const Pipeline::TargetPosition& target,
    const QVector<Pipeline::TargetPosition>& allTargets,
    const QMap<QString, double>& currentPositions)
{
    QJsonObject input;
    input[QStringLiteral("target")] = PluginDataBridge::toJson(target);
    input[QStringLiteral("allTargets")] = PluginDataBridge::toJson(allTargets);
    input[QStringLiteral("currentPositions")] = PluginDataBridge::toJson(currentPositions);

    auto result = m_facade->invoke(QStringLiteral("evaluate"), input);
    if (!result) {
        emit riskViolation(target.symbol, QString::fromStdString(result.error().message));
        Pipeline::RiskDecision rejected;
        rejected.action = Pipeline::RiskDecision::Action::Reject;
        rejected.reason = QString::fromStdString(result.error().message);
        rejected.blockId = id();
        return rejected;
    }

    const QJsonObject obj = result->toObject();
    Pipeline::RiskDecision decision;
    const QString action = obj.value(QStringLiteral("action")).toString(QStringLiteral("Approve"));
    if (action == QStringLiteral("Reject")) {
        decision.action = Pipeline::RiskDecision::Action::Reject;
    } else if (action == QStringLiteral("Modify")) {
        decision.action = Pipeline::RiskDecision::Action::Modify;
        if (obj.contains(QStringLiteral("modifiedQuantity"))) {
            decision.modifiedQuantity = obj.value(QStringLiteral("modifiedQuantity")).toDouble();
        }
    } else {
        decision.action = Pipeline::RiskDecision::Action::Approve;
    }
    decision.reason = obj.value(QStringLiteral("reason")).toString();
    decision.blockId = id();
    return decision;
}

const Pipeline::PipelineRuntimeContext* PluginRiskBlockWrapper::pluginRuntimeContext() const
{
    return runtimeContext();
}

QString PluginRiskBlockWrapper::pluginOwnerId() const
{
    return objectName().isEmpty() ? id() : objectName();
}

void PluginRiskBlockWrapper::handlePluginEvent(const QString& eventName, const QJsonValue& payload)
{
    if (eventName == QStringLiteral("risk.signal_generated") && payload.isObject()) {
        emit riskSignalGenerated(PluginDataBridge::signalFromJson(payload.toObject()));
        return;
    }
    if (eventName == QStringLiteral("risk.violation") && payload.isObject()) {
        const QJsonObject obj = payload.toObject();
        emit riskViolation(obj.value(QStringLiteral("symbol")).toString(),
                           obj.value(QStringLiteral("reason")).toString());
        return;
    }
    if (eventName == QStringLiteral("plugin.error")) {
        emit riskViolation(id(), errorMessageFromPayload(payload));
    }
}

PluginExecutionBlockWrapper::PluginExecutionBlockWrapper(const PluginRuntimePtr& runtime,
                                                         const ExtensionManifest& extension,
                                                         QObject* parent)
    : IExecutionBlock(parent)
    , m_runtime(runtime)
    , m_extension(extension)
    , m_facade(std::make_unique<PluginInstanceFacade>(runtime, extension, this))
{
    setConfig(extension.defaultConfig);
}

PluginExecutionBlockWrapper::~PluginExecutionBlockWrapper() = default;

QString PluginExecutionBlockWrapper::id() const { return m_extension.extensionId; }
QString PluginExecutionBlockWrapper::name() const { return m_extension.name; }
QJsonObject PluginExecutionBlockWrapper::config() const { return m_facade->config(); }

void PluginExecutionBlockWrapper::setRuntimeContext(const Pipeline::PipelineRuntimeContext* ctx)
{
    m_runtimeContext = ctx;
    m_facade->setRuntimeContext(ctx);
}

void PluginExecutionBlockWrapper::setConfig(const QJsonObject& config)
{
    auto result = m_facade->setConfig(config);
    if (!result) {
        emit executionError(id(), QString::fromStdString(result.error().message));
    }
}

void PluginExecutionBlockWrapper::execute(const QVector<Pipeline::ExecutionIntent>& intents)
{
    QJsonObject input;
    input[QStringLiteral("intents")] = PluginDataBridge::toJson(intents);
    auto result = m_facade->invoke(QStringLiteral("execute"), input);
    if (!result) {
        emit executionError(id(), QString::fromStdString(result.error().message));
    }
}

const Pipeline::PipelineRuntimeContext* PluginExecutionBlockWrapper::pluginRuntimeContext() const
{
    return m_runtimeContext;
}

QString PluginExecutionBlockWrapper::pluginOwnerId() const
{
    return objectName().isEmpty() ? id() : objectName();
}

void PluginExecutionBlockWrapper::handlePluginEvent(const QString& eventName,
                                                    const QJsonValue& payload)
{
    if (eventName == QStringLiteral("execution.order_placed") && payload.isObject()) {
        const QJsonObject obj = payload.toObject();
        emit orderPlaced(obj.value(QStringLiteral("symbol")).toString(),
                         obj.value(QStringLiteral("orderId")).toString());
        return;
    }
    if (eventName == QStringLiteral("execution.error") && payload.isObject()) {
        const QJsonObject obj = payload.toObject();
        emit executionError(obj.value(QStringLiteral("symbol")).toString(),
                            obj.value(QStringLiteral("error")).toString());
        return;
    }
    if (eventName == QStringLiteral("plugin.error")) {
        emit executionError(id(), errorMessageFromPayload(payload));
    }
}

} // namespace Plugin
