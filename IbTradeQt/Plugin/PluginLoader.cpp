#include "PluginLoader.h"

#include <QDebug>
#include <QDir>
#include <QLibrary>
#include "../Pipeline/BlockRegistry.h"

namespace Plugin {

Expected<void, Error> PluginLoader::loadPlugin(const QString& path)
{
    auto lib = new QLibrary(path);

    if (!lib->load()) {
        auto errStr = lib->errorString().toStdString();
        delete lib;
        return make_unexpected(Error{
            ErrorCode::ConfigurationError,
            errStr,
            "PluginLoader::loadPlugin"
        });
    }

    auto createFn = reinterpret_cast<IBlockPlugin*(*)()>(
        lib->resolve("createBlockPlugin"));
    auto destroyFn = reinterpret_cast<void(*)(IBlockPlugin*)>(
        lib->resolve("destroyBlockPlugin"));

    if (!createFn || !destroyFn) {
        lib->unload();
        delete lib;
        return make_unexpected(Error{
            ErrorCode::ConfigurationError,
            "Plugin missing required exports (createBlockPlugin/destroyBlockPlugin)",
            "PluginLoader::loadPlugin"
        });
    }

    IBlockPlugin* plugin = createFn();
    auto meta = plugin->metadata();

    auto& registry = Pipeline::BlockRegistry::instance();
    for (const auto& descriptor : plugin->blockDescriptors()) {
        registry.registerBlock(descriptor);
    }

    m_plugins[meta.name] = PluginInfo{plugin, destroyFn, lib, meta};
    qInfo() << "Loaded block plugin:" << meta.name << "v" << meta.version;
    return {};
}

Expected<void, Error> PluginLoader::loadPluginDir(const QString& dirPath,
                                     const QStringList& filters)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return make_unexpected(Error{
            ErrorCode::ConfigurationError,
            "Plugin directory does not exist: " + dirPath.toStdString(),
            "PluginLoader::loadPluginDir"
        });
    }

    for (const auto& file : dir.entryList(filters, QDir::Files)) {
        loadPlugin(dir.filePath(file));
    }
    return {};
}

QVector<PluginMetadata> PluginLoader::listPlugins() const
{
    QVector<PluginMetadata> result;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        result.push_back(it.value().metadata);
    }
    return result;
}

int PluginLoader::pluginCount() const { return m_plugins.size(); }

PluginLoader::~PluginLoader()
{
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        it.value().destroyFn(it.value().plugin);
        it.value().library->unload();
        delete it.value().library;
    }
}

} // namespace Plugin
