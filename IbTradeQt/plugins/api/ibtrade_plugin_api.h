#ifndef IBTRADE_PLUGIN_API_H
#define IBTRADE_PLUGIN_API_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  define IBTRADE_PLUGIN_EXPORT __declspec(dllexport)
#else
#  define IBTRADE_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IBTRADE_PLUGIN_ABI_VERSION_V1 1u
#define IBTRADE_PLUGIN_MANIFEST_VERSION_V1 1u

typedef struct ibtrade_owned_string_v1 {
    const char* data;
    size_t size;
    void (*release)(const char* data, size_t size, void* user_data);
    void* user_data;
} ibtrade_owned_string_v1;

typedef enum ibtrade_log_level_v1 {
    IBTRADE_LOG_DEBUG_V1 = 0,
    IBTRADE_LOG_INFO_V1 = 1,
    IBTRADE_LOG_WARNING_V1 = 2,
    IBTRADE_LOG_ERROR_V1 = 3
} ibtrade_log_level_v1;

typedef struct ibtrade_host_services_v1 {
    uint32_t abi_version;
    void* user_data;

    ibtrade_owned_string_v1 (*current_time_iso8601)(void* user_data);
    ibtrade_owned_string_v1 (*read_market_data_json)(void* user_data, const char* symbol);
    ibtrade_owned_string_v1 (*read_historical_bars_json)(void* user_data, const char* request_json);
    ibtrade_owned_string_v1 (*read_holdings_json)(void* user_data);

    int (*set_subscription_request_json)(
        void* user_data,
        const char* owner_id,
        const char* request_json,
        ibtrade_owned_string_v1* error_out);

    int (*place_order_json)(
        void* user_data,
        const char* intent_json,
        ibtrade_owned_string_v1* error_out);

    int (*cancel_all_pending)(
        void* user_data,
        ibtrade_owned_string_v1* error_out);

    void (*log_message)(
        void* user_data,
        ibtrade_log_level_v1 level,
        const char* message);

    void (*emit_event_json)(
        void* user_data,
        const char* event_name,
        const char* payload_json);
} ibtrade_host_services_v1;

typedef struct ibtrade_plugin_api_v1 {
    uint32_t abi_version;
    uint32_t struct_size;

    void* (*create_extension)(
        const char* extension_id,
        const ibtrade_host_services_v1* host_services,
        ibtrade_owned_string_v1* error_out);

    void (*destroy_extension)(void* instance);

    int (*initialize)(
        void* instance,
        ibtrade_owned_string_v1* error_out);

    void (*shutdown)(void* instance);

    int (*set_config_json)(
        void* instance,
        const char* config_json,
        ibtrade_owned_string_v1* error_out);

    ibtrade_owned_string_v1 (*get_state_json)(void* instance);

    ibtrade_owned_string_v1 (*invoke_json)(
        void* instance,
        const char* operation,
        const char* input_json,
        ibtrade_owned_string_v1* error_out);
} ibtrade_plugin_api_v1;

typedef const ibtrade_plugin_api_v1* (*ibtrade_get_plugin_api_v1_fn)(void);

#ifdef __cplusplus
}
#endif

#endif // IBTRADE_PLUGIN_API_H
