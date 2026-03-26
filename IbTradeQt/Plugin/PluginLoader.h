#ifndef PLUGIN_PLUGINLOADER_H
#define PLUGIN_PLUGINLOADER_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "../Common/Expected.h"
#include "ExtensionRegistry.h"
#include "PluginRuntime.h"

namespace Plugin {

struct PluginLoadFailure {
    QString sourcePath;
    QString pluginId;
    QString message;
};

class PluginLoader {
public:
    Expected<void, Error> loadPlugin(const QString& path);

    Expected<void, Error> loadPluginDir(const QString& dirPath);

    QVector<PluginMetadata> listPlugins() const;

    QVector<PluginLoadFailure> loadFailures() const;

    int pluginCount() const;

    int extensionCount() const;

    const ExtensionRegistry& extensionRegistry() const;

    void clear();

    ~PluginLoader();

private:
    Expected<void, Error> registerExtensions(const PluginRuntimePtr& runtime);
    void recordFailure(const QString& sourcePath, const QString& pluginId, const QString& message);

    ExtensionRegistry m_extensionRegistry;
    QMap<QString, PluginRuntimePtr> m_plugins;
    QVector<PluginLoadFailure> m_failures;
};

} // namespace Plugin

#endif // PLUGIN_PLUGINLOADER_H
