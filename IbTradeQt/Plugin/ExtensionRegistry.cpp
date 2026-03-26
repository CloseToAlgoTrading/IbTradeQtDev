#include "ExtensionRegistry.h"

namespace Plugin {

Expected<void, Error> ExtensionRegistry::registerExtension(const PluginMetadata& plugin,
                                                           const ExtensionManifest& extension)
{
    if (m_extensions.contains(extension.extensionId)) {
        return make_unexpected(Error{
            ErrorCode::ConfigurationError,
            QStringLiteral("Duplicate plugin extension id: %1").arg(extension.extensionId).toStdString(),
            "ExtensionRegistry::registerExtension"
        });
    }

    m_extensions.insert(extension.extensionId, RegisteredExtension{plugin, extension});
    return {};
}

bool ExtensionRegistry::contains(const QString& extensionId) const
{
    return m_extensions.contains(extensionId);
}

Expected<RegisteredExtension, Error> ExtensionRegistry::extension(const QString& extensionId) const
{
    auto it = m_extensions.find(extensionId);
    if (it == m_extensions.end()) {
        return make_unexpected(Error{
            ErrorCode::NotFound,
            QStringLiteral("Plugin extension not found: %1").arg(extensionId).toStdString(),
            "ExtensionRegistry::extension"
        });
    }
    return it.value();
}

QVector<RegisteredExtension> ExtensionRegistry::extensionsForPoint(const QString& extensionPointId) const
{
    QVector<RegisteredExtension> out;
    for (auto it = m_extensions.begin(); it != m_extensions.end(); ++it) {
        if (it.value().extension.extensionPointId == extensionPointId) {
            out.push_back(it.value());
        }
    }
    return out;
}

bool ExtensionRegistry::unregisterExtension(const QString& extensionId)
{
    return m_extensions.remove(extensionId) > 0;
}

void ExtensionRegistry::clear()
{
    m_extensions.clear();
}

} // namespace Plugin
