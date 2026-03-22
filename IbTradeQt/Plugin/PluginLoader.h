#ifndef PLUGIN_PLUGINLOADER_H
#define PLUGIN_PLUGINLOADER_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include "BlockPlugin.h"
#include "../Common/Expected.h"

class QLibrary;

namespace Plugin {

class PluginLoader {
public:
    Expected<void, Error> loadPlugin(const QString& path);

    Expected<void, Error> loadPluginDir(const QString& dirPath,
                                        const QStringList& filters = {QStringLiteral("*.so"), QStringLiteral("*.dll"), QStringLiteral("*.dylib")});

    QVector<PluginMetadata> listPlugins() const;

    int pluginCount() const;

    ~PluginLoader();

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
