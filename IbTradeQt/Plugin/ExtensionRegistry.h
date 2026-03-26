#ifndef PLUGIN_EXTENSIONREGISTRY_H
#define PLUGIN_EXTENSIONREGISTRY_H

#include <QMap>
#include <QString>
#include <QVector>

#include "../Common/Expected.h"
#include "PluginManifest.h"

namespace Plugin {

struct RegisteredExtension {
    PluginMetadata plugin;
    ExtensionManifest extension;
};

class ExtensionRegistry {
public:
    Expected<void, Error> registerExtension(const PluginMetadata& plugin,
                                            const ExtensionManifest& extension);

    bool contains(const QString& extensionId) const;

    Expected<RegisteredExtension, Error> extension(const QString& extensionId) const;

    QVector<RegisteredExtension> extensionsForPoint(const QString& extensionPointId) const;

    bool unregisterExtension(const QString& extensionId);

    void clear();

private:
    QMap<QString, RegisteredExtension> m_extensions;
};

} // namespace Plugin

#endif // PLUGIN_EXTENSIONREGISTRY_H
