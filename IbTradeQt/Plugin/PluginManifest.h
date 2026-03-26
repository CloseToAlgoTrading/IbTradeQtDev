#ifndef PLUGIN_PLUGINMANIFEST_H
#define PLUGIN_PLUGINMANIFEST_H

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "../Common/Expected.h"
#include "../Pipeline/Scope.h"

namespace Plugin {

struct ExtensionManifest {
    QString extensionPointId;
    QString extensionId;
    QString name;
    QString description;
    Pipeline::Scope scope = Pipeline::Scope::Strategy;
    QJsonObject defaultConfig;
    QJsonObject stateSchema;
    bool supportsAsyncSemantic = false;
};

struct PluginMetadata {
    QString pluginId;
    QString name;
    QString version;
    QString description;
    QString vendor;
    QString manifestPath;
    QString packageRoot;
    QString libraryPath;
    int manifestVersion = 0;
    int hostApiVersion = 0;
    QVector<ExtensionManifest> extensions;
};

Expected<PluginMetadata, Error> loadPluginManifest(const QString& manifestPath);

} // namespace Plugin

#endif // PLUGIN_PLUGINMANIFEST_H
