#include "../../sdk/PluginSdk.hpp"

using namespace ibtrade::sdk;

extern "C" {

static void* create_extension(const char*, const ibtrade_host_services_v1*, ibtrade_owned_string_v1*)
{
    return nullptr;
}

static void destroy_extension(void*) {}

static int initialize_instance(void*, ibtrade_owned_string_v1* error_out)
{
    if (error_out) {
        *error_out = make_owned_string("Template plugin: implement initialize_instance");
    }
    return 0;
}

static void shutdown_instance(void*) {}

static int set_config_json(void*, const char*, ibtrade_owned_string_v1* error_out)
{
    if (error_out) {
        *error_out = {};
    }
    return 1;
}

static ibtrade_owned_string_v1 get_state_json(void*)
{
    return make_owned_string("{}");
}

static ibtrade_owned_string_v1 invoke_json(void*, const char*, const char*, ibtrade_owned_string_v1* error_out)
{
    if (error_out) {
        *error_out = make_owned_string("Template plugin: implement invoke_json");
    }
    return make_owned_string("{}");
}

static const ibtrade_plugin_api_v1 kPluginApi = {
    IBTRADE_PLUGIN_ABI_VERSION_V1,
    sizeof(ibtrade_plugin_api_v1),
    &create_extension,
    &destroy_extension,
    &initialize_instance,
    &shutdown_instance,
    &set_config_json,
    &get_state_json,
    &invoke_json
};

IBTRADE_PLUGIN_EXPORT const ibtrade_plugin_api_v1* ibtrade_get_plugin_api_v1(void)
{
    return &kPluginApi;
}

} // extern "C"
