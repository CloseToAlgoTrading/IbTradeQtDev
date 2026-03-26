#ifndef PLUGIN_PLUGINRUNTIME_H
#define PLUGIN_PLUGINRUNTIME_H

#include <atomic>
#include <memory>

#include "PluginManifest.h"
#include "../plugins/api/ibtrade_plugin_api.h"

class QLibrary;

namespace Plugin {

struct PluginRuntime {
    PluginMetadata metadata;
    std::unique_ptr<QLibrary> library;
    const ibtrade_plugin_api_v1* api = nullptr;
    std::atomic<int> activeInstances{0};

    ~PluginRuntime();
};

using PluginRuntimePtr = std::shared_ptr<PluginRuntime>;

} // namespace Plugin

#endif // PLUGIN_PLUGINRUNTIME_H
