#include "PluginManifest.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>

namespace Plugin {

namespace {

Error makeManifestError(const QString& message, const QString& context)
{
    return Error{
        ErrorCode::ConfigurationError,
        message.toStdString(),
        context.toStdString()
    };
}

Expected<Pipeline::Scope, Error> parseScope(const QString& rawScope)
{
    if (rawScope.compare(QStringLiteral("Strategy"), Qt::CaseInsensitive) == 0) {
        return Pipeline::Scope::Strategy;
    }
    if (rawScope.compare(QStringLiteral("Portfolio"), Qt::CaseInsensitive) == 0) {
        return Pipeline::Scope::Portfolio;
    }
    return make_unexpected(makeManifestError(
        QStringLiteral("Unsupported plugin scope: %1").arg(rawScope),
        QStringLiteral("PluginManifest::parseScope")));
}

Expected<ExtensionManifest, Error> parseExtension(const QJsonObject& obj,
                                                 const QString& pluginId)
{
    ExtensionManifest ext;
    ext.extensionPointId = obj.value(QStringLiteral("extension_point_id")).toString().trimmed();
    ext.extensionId = obj.value(QStringLiteral("extension_id")).toString().trimmed();
    ext.name = obj.value(QStringLiteral("name")).toString().trimmed();
    ext.description = obj.value(QStringLiteral("description")).toString().trimmed();
    ext.defaultConfig = obj.value(QStringLiteral("default_config")).toObject();
    ext.stateSchema = obj.value(QStringLiteral("state_schema")).toObject();
    ext.supportsAsyncSemantic =
        obj.value(QStringLiteral("supports_async_semantic")).toBool(false);

    const QString scopeRaw =
        obj.value(QStringLiteral("scope")).toString(QStringLiteral("Strategy"));
    auto scope = parseScope(scopeRaw);
    if (!scope) {
        return make_unexpected(scope.error());
    }
    ext.scope = *scope;

    if (ext.extensionPointId.isEmpty() || ext.extensionId.isEmpty() || ext.name.isEmpty()) {
        return make_unexpected(makeManifestError(
            QStringLiteral("Plugin '%1' has an extension with missing required fields")
                .arg(pluginId),
            QStringLiteral("PluginManifest::parseExtension")));
    }

    return ext;
}

} // namespace

Expected<PluginMetadata, Error> loadPluginManifest(const QString& manifestPath)
{
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return make_unexpected(makeManifestError(
            QStringLiteral("Failed to open plugin manifest: %1").arg(manifestPath),
            QStringLiteral("PluginManifest::loadPluginManifest")));
    }

    const QByteArray raw = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) {
        return make_unexpected(makeManifestError(
            QStringLiteral("Plugin manifest is not a JSON object: %1").arg(manifestPath),
            QStringLiteral("PluginManifest::loadPluginManifest")));
    }

    const QJsonObject obj = doc.object();
    PluginMetadata meta;
    meta.pluginId = obj.value(QStringLiteral("plugin_id")).toString().trimmed();
    meta.name = obj.value(QStringLiteral("name")).toString().trimmed();
    meta.version = obj.value(QStringLiteral("version")).toString().trimmed();
    meta.description = obj.value(QStringLiteral("description")).toString().trimmed();
    meta.vendor = obj.value(QStringLiteral("vendor")).toString().trimmed();
    meta.manifestVersion = obj.value(QStringLiteral("manifest_version")).toInt();
    meta.hostApiVersion = obj.value(QStringLiteral("host_api_version")).toInt();
    meta.manifestPath = manifestPath;
    meta.packageRoot = QFileInfo(manifestPath).absolutePath();

    const QString libraryRel = obj.value(QStringLiteral("library")).toString().trimmed();
    if (!libraryRel.isEmpty()) {
        meta.libraryPath = QFileInfo(meta.packageRoot + QLatin1Char('/') + libraryRel).absoluteFilePath();
    }

    if (meta.pluginId.isEmpty() || meta.name.isEmpty() || meta.version.isEmpty()
        || meta.manifestVersion <= 0 || meta.hostApiVersion <= 0
        || meta.libraryPath.isEmpty()) {
        return make_unexpected(makeManifestError(
            QStringLiteral("Plugin manifest missing required fields: %1").arg(manifestPath),
            QStringLiteral("PluginManifest::loadPluginManifest")));
    }

    const QJsonArray extArray = obj.value(QStringLiteral("extensions")).toArray();
    if (extArray.isEmpty()) {
        return make_unexpected(makeManifestError(
            QStringLiteral("Plugin manifest has no extensions: %1").arg(manifestPath),
            QStringLiteral("PluginManifest::loadPluginManifest")));
    }

    for (const QJsonValue& value : extArray) {
        if (!value.isObject()) {
            return make_unexpected(makeManifestError(
                QStringLiteral("Plugin manifest contains a non-object extension entry"),
                QStringLiteral("PluginManifest::loadPluginManifest")));
        }
        auto parsed = parseExtension(value.toObject(), meta.pluginId);
        if (!parsed) {
            return make_unexpected(parsed.error());
        }
        meta.extensions.push_back(*parsed);
    }

    return meta;
}

} // namespace Plugin
