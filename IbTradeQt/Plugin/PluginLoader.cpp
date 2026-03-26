#include "PluginLoader.h"

#include <cstddef>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QLoggingCategory>
#include <QSet>

#include "../Pipeline/BlockRegistry.h"
#include "../Pipeline/PipelineConstants.h"
#include "PluginManifest.h"
#include "PluginWrappers.h"

Q_LOGGING_CATEGORY(lcPluginLoader, "plugin.loader")

namespace Plugin {

namespace {

constexpr size_t kRequiredPluginApiSize =
    offsetof(ibtrade_plugin_api_v1, invoke_json) + sizeof(((ibtrade_plugin_api_v1*)nullptr)->invoke_json);

QString categoryForExtensionPoint(const QString& extensionPointId)
{
    if (extensionPointId == QStringLiteral("pipeline.selection")) {
        return QString(Pipeline::Category::Selection);
    }
    if (extensionPointId == QStringLiteral("pipeline.alpha")) {
        return QString(Pipeline::Category::Alpha);
    }
    if (extensionPointId == QStringLiteral("pipeline.rebalance")) {
        return QString(Pipeline::Category::Rebalance);
    }
    if (extensionPointId == QStringLiteral("pipeline.risk")) {
        return QString(Pipeline::Category::Risk);
    }
    if (extensionPointId == QStringLiteral("pipeline.execution")) {
        return QString(Pipeline::Category::Execution);
    }
    return {};
}

Error makeLoaderError(const QString& message, const QString& context)
{
    return Error{
        ErrorCode::ConfigurationError,
        message.toStdString(),
        context.toStdString()
    };
}

QString manifestPathFor(const QString& path)
{
    QFileInfo info(path);
    if (info.isDir()) {
        return QDir(path).filePath(QStringLiteral("plugin.json"));
    }
    if (info.fileName() == QStringLiteral("plugin.json")) {
        return info.absoluteFilePath();
    }
    return {};
}

Expected<PluginRuntimePtr, Error> loadRuntime(const PluginMetadata& meta)
{
    auto runtime = std::make_shared<PluginRuntime>();
    runtime->metadata = meta;
    runtime->library = std::make_unique<QLibrary>(meta.libraryPath);
    runtime->library->setLoadHints(QLibrary::PreventUnloadHint);

    if (!runtime->library->load()) {
        return make_unexpected(makeLoaderError(
            QStringLiteral("Failed to load plugin library '%1': %2")
                .arg(meta.libraryPath, runtime->library->errorString()),
            QStringLiteral("PluginLoader::loadRuntime")));
    }

    auto fn = reinterpret_cast<ibtrade_get_plugin_api_v1_fn>(
        runtime->library->resolve("ibtrade_get_plugin_api_v1"));
    if (!fn) {
        return make_unexpected(makeLoaderError(
            QStringLiteral("Plugin '%1' is missing ibtrade_get_plugin_api_v1 export").arg(meta.pluginId),
            QStringLiteral("PluginLoader::loadRuntime")));
    }

    runtime->api = fn();
    if (!runtime->api || runtime->api->abi_version != IBTRADE_PLUGIN_ABI_VERSION_V1) {
        return make_unexpected(makeLoaderError(
            QStringLiteral("Plugin '%1' returned an incompatible plugin API").arg(meta.pluginId),
            QStringLiteral("PluginLoader::loadRuntime")));
    }
    if (runtime->api->struct_size < kRequiredPluginApiSize) {
        return make_unexpected(makeLoaderError(
            QStringLiteral("Plugin '%1' returned a truncated plugin API table").arg(meta.pluginId),
            QStringLiteral("PluginLoader::loadRuntime")));
    }
    if (!runtime->api->create_extension
        || !runtime->api->destroy_extension
        || !runtime->api->invoke_json) {
        return make_unexpected(makeLoaderError(
            QStringLiteral("Plugin '%1' is missing required plugin API functions").arg(meta.pluginId),
            QStringLiteral("PluginLoader::loadRuntime")));
    }

    return runtime;
}

std::function<QObject*()> makeFactory(const PluginRuntimePtr& runtime,
                                      const ExtensionManifest& extension)
{
    if (extension.extensionPointId == QStringLiteral("pipeline.selection")) {
        return [runtime, extension]() -> QObject* {
            return new PluginSelectionBlockWrapper(runtime, extension);
        };
    }
    if (extension.extensionPointId == QStringLiteral("pipeline.alpha")) {
        return [runtime, extension]() -> QObject* {
            return new PluginAlphaBlockWrapper(runtime, extension);
        };
    }
    if (extension.extensionPointId == QStringLiteral("pipeline.rebalance")) {
        return [runtime, extension]() -> QObject* {
            return new PluginRebalanceBlockWrapper(runtime, extension);
        };
    }
    if (extension.extensionPointId == QStringLiteral("pipeline.risk")) {
        return [runtime, extension]() -> QObject* {
            return new PluginRiskBlockWrapper(runtime, extension);
        };
    }
    if (extension.extensionPointId == QStringLiteral("pipeline.execution")) {
        return [runtime, extension]() -> QObject* {
            return new PluginExecutionBlockWrapper(runtime, extension);
        };
    }
    return {};
}

} // namespace

Expected<void, Error> PluginLoader::loadPlugin(const QString& path)
{
    const QString manifestPath = manifestPathFor(path);
    if (manifestPath.isEmpty()) {
        const QString message =
            QStringLiteral("Plugin path must point to a package directory or plugin.json: %1").arg(path);
        recordFailure(path, QString(), message);
        return make_unexpected(makeLoaderError(message, QStringLiteral("PluginLoader::loadPlugin")));
    }

    auto meta = loadPluginManifest(manifestPath);
    if (!meta) {
        recordFailure(path, QString(), QString::fromStdString(meta.error().message));
        return make_unexpected(meta.error());
    }

    if (meta->manifestVersion != static_cast<int>(IBTRADE_PLUGIN_MANIFEST_VERSION_V1)) {
        const QString message = QStringLiteral("Plugin '%1' has unsupported manifest version %2")
                                    .arg(meta->pluginId)
                                    .arg(meta->manifestVersion);
        recordFailure(path, meta->pluginId, message);
        return make_unexpected(makeLoaderError(message, QStringLiteral("PluginLoader::loadPlugin")));
    }
    if (meta->hostApiVersion != static_cast<int>(IBTRADE_PLUGIN_ABI_VERSION_V1)) {
        const QString message = QStringLiteral("Plugin '%1' targets unsupported host API version %2")
                                    .arg(meta->pluginId)
                                    .arg(meta->hostApiVersion);
        recordFailure(path, meta->pluginId, message);
        return make_unexpected(makeLoaderError(message, QStringLiteral("PluginLoader::loadPlugin")));
    }
    if (m_plugins.contains(meta->pluginId)) {
        const QString message = QStringLiteral("Duplicate plugin id: %1").arg(meta->pluginId);
        recordFailure(path, meta->pluginId, message);
        return make_unexpected(makeLoaderError(message, QStringLiteral("PluginLoader::loadPlugin")));
    }

    QSet<QString> manifestExtensionIds;
    for (const ExtensionManifest& extension : meta->extensions) {
        if (manifestExtensionIds.contains(extension.extensionId)) {
            const QString message = QStringLiteral("Plugin '%1' declares duplicate extension id: %2")
                                        .arg(meta->pluginId, extension.extensionId);
            recordFailure(path, meta->pluginId, message);
            return make_unexpected(makeLoaderError(message, QStringLiteral("PluginLoader::loadPlugin")));
        }
        manifestExtensionIds.insert(extension.extensionId);

        if (m_extensionRegistry.contains(extension.extensionId)
            || Pipeline::BlockRegistry::instance().contains(extension.extensionId)) {
            const QString message =
                QStringLiteral("Duplicate extension/block id: %1").arg(extension.extensionId);
            recordFailure(path, meta->pluginId, message);
            return make_unexpected(makeLoaderError(message, QStringLiteral("PluginLoader::loadPlugin")));
        }
    }

    auto runtime = loadRuntime(*meta);
    if (!runtime) {
        recordFailure(path, meta->pluginId, QString::fromStdString(runtime.error().message));
        return make_unexpected(runtime.error());
    }

    auto registered = registerExtensions(*runtime);
    if (!registered) {
        recordFailure(path, meta->pluginId, QString::fromStdString(registered.error().message));
        return registered;
    }

    m_plugins.insert(meta->pluginId, *runtime);
    qCInfo(lcPluginLoader) << "Loaded plugin package" << meta->pluginId
                           << "with" << meta->extensions.size() << "extensions";
    return {};
}

Expected<void, Error> PluginLoader::registerExtensions(const PluginRuntimePtr& runtime)
{
    QVector<QString> registeredExtensionIds;
    QVector<QString> registeredBlockIds;

    for (const ExtensionManifest& extension : runtime->metadata.extensions) {
        auto reg = m_extensionRegistry.registerExtension(runtime->metadata, extension);
        if (!reg) {
            for (const QString& blockId : registeredBlockIds) {
                Pipeline::BlockRegistry::instance().unregisterBlock(blockId);
            }
            for (const QString& extensionId : registeredExtensionIds) {
                m_extensionRegistry.unregisterExtension(extensionId);
            }
            return reg;
        }
        registeredExtensionIds.push_back(extension.extensionId);

        const QString category = categoryForExtensionPoint(extension.extensionPointId);
        if (category.isEmpty()) {
            continue;
        }

        Pipeline::BlockDescriptor descriptor;
        descriptor.id = extension.extensionId;
        descriptor.name = extension.name;
        descriptor.category = category;
        descriptor.description = extension.description;
        descriptor.scope = extension.scope;
        descriptor.defaultConfig = extension.defaultConfig;
        descriptor.factory = makeFactory(runtime, extension);
        Pipeline::BlockRegistry::instance().registerBlock(descriptor);
        registeredBlockIds.push_back(extension.extensionId);
    }
    return {};
}

Expected<void, Error> PluginLoader::loadPluginDir(const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return make_unexpected(makeLoaderError(
            QStringLiteral("Plugin directory does not exist: %1").arg(dirPath),
            QStringLiteral("PluginLoader::loadPluginDir")));
    }

    if (QFileInfo(dir.filePath(QStringLiteral("plugin.json"))).exists()) {
        return loadPlugin(dir.absolutePath());
    }

    for (const QFileInfo& child : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString childManifest = QDir(child.absoluteFilePath()).filePath(QStringLiteral("plugin.json"));
        if (QFileInfo(childManifest).exists()) {
            auto result = loadPlugin(child.absoluteFilePath());
            if (!result) {
                qCWarning(lcPluginLoader) << QString::fromStdString(result.error().message);
            }
        }
    }
    return {};
}

QVector<PluginMetadata> PluginLoader::listPlugins() const
{
    QVector<PluginMetadata> result;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        result.push_back(it.value()->metadata);
    }
    return result;
}

QVector<PluginLoadFailure> PluginLoader::loadFailures() const
{
    return m_failures;
}

int PluginLoader::pluginCount() const
{
    return m_plugins.size();
}

int PluginLoader::extensionCount() const
{
    int count = 0;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        count += it.value()->metadata.extensions.size();
    }
    return count;
}

const ExtensionRegistry& PluginLoader::extensionRegistry() const
{
    return m_extensionRegistry;
}

void PluginLoader::recordFailure(const QString& sourcePath,
                                 const QString& pluginId,
                                 const QString& message)
{
    PluginLoadFailure failureRecord;
    failureRecord.sourcePath = sourcePath;
    failureRecord.pluginId = pluginId;
    failureRecord.message = message;
    m_failures.push_back(failureRecord);
}

void PluginLoader::clear()
{
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        for (const ExtensionManifest& extension : it.value()->metadata.extensions) {
            Pipeline::BlockRegistry::instance().unregisterBlock(extension.extensionId);
        }
    }
    m_plugins.clear();
    m_extensionRegistry.clear();
    m_failures.clear();
}

PluginLoader::~PluginLoader()
{
    clear();
}

} // namespace Plugin
