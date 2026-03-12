#ifndef PLUGIN_BLOCKPLUGIN_H
#define PLUGIN_BLOCKPLUGIN_H

#include <QString>
#include <QVector>
#include "../Pipeline/BlockRegistry.h"

namespace Plugin {

struct PluginMetadata {
    QString name;
    QString version;
    QString description;
    QString author;
};

class IBlockPlugin {
public:
    virtual ~IBlockPlugin() = default;

    virtual PluginMetadata metadata() const = 0;
    virtual QVector<Pipeline::BlockDescriptor> blockDescriptors() const = 0;
};

#define EXPORT_BLOCK_PLUGIN(PluginClass) \
    extern "C" { \
        Q_DECL_EXPORT Plugin::IBlockPlugin* createBlockPlugin() { \
            return new PluginClass(); \
        } \
        Q_DECL_EXPORT void destroyBlockPlugin(Plugin::IBlockPlugin* plugin) { \
            delete plugin; \
        } \
    }

} // namespace Plugin

#endif // PLUGIN_BLOCKPLUGIN_H
