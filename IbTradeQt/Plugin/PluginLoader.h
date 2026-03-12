#ifndef PLUGIN_PLUGINLOADER_H
#define PLUGIN_PLUGINLOADER_H

#include <QLibrary>
#include <QDir>
#include <QMap>
#include <QDebug>
#include "BlockPlugin.h"
#include "../Pipeline/BlockRegistry.h"
#include "../Common/Expected.h"

namespace Plugin {

class PluginLoader {
public:
    Expected<void, Error> loadPlugin(const QString& path) {
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

    Expected<void, Error> loadPluginDir(const QString& dirPath,
                                         const QStringList& filters = {"*.so", "*.dll", "*.dylib"})
    {
        QDir dir(dirPath);
        if (!dir.exists()) {
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Plugin directory does not exist: " + dirPath.toStdString(),
                "PluginLoader::loadPluginDir"
            });
        }

        int loaded = 0;
        for (const auto& file : dir.entryList(filters, QDir::Files)) {
            auto result = loadPlugin(dir.filePath(file));
            if (result) ++loaded;
        }
        return {};
    }

    QVector<PluginMetadata> listPlugins() const {
        QVector<PluginMetadata> result;
        for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
            result.push_back(it.value().metadata);
        }
        return result;
    }

    int pluginCount() const { return m_plugins.size(); }

    ~PluginLoader() {
        for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
            it.value().destroyFn(it.value().plugin);
            it.value().library->unload();
            delete it.value().library;
        }
    }

private:
    struct PluginInfo {
        IBlockPlugin* plugin;
        void(*destroyFn)(IBlockPlugin*);
        QLibrary* library;
        PluginMetadata metadata;
    };

    QMap<QString, PluginInfo> m_plugins;
};

} // namespace Plugin

#endif // PLUGIN_PLUGINLOADER_H
