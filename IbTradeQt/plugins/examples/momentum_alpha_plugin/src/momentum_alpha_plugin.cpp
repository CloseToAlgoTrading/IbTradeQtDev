#include "../../../sdk/PluginSdk.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace ibtrade::sdk;

namespace {

struct PluginInstance {
    const ibtrade_host_services_v1* host = nullptr;
    std::string extension_id;
    std::string config_json = "{}";
    std::string last_state_json = "{}";
    int period = 20;
    double threshold = 0.02;
    int top_n = 3;
    double position_size = 100.0;
    std::string resolution = "Day1";
    std::string data_source_id = "yahoo";
    int lookback_years = 1;
};

std::vector<std::string> extract_symbols_from_model_data(const std::string& json)
{
    std::vector<std::string> symbols;
    std::size_t pos = 0;
    while (true) {
        const std::size_t key_pos = json.find("\"symbol\"", pos);
        if (key_pos == std::string::npos) {
            break;
        }
        const std::size_t first_quote = json.find('"', json.find(':', key_pos) + 1);
        const std::size_t second_quote = json.find('"', first_quote + 1);
        if (first_quote == std::string::npos || second_quote == std::string::npos) {
            break;
        }
        symbols.push_back(json.substr(first_quote + 1, second_quote - first_quote - 1));
        pos = second_quote + 1;
    }
    return symbols;
}

std::vector<double> extract_bar_closes(const std::string& json)
{
    std::vector<double> closes;
    std::size_t pos = 0;
    while (true) {
        const std::size_t key_pos = json.find("\"close\"", pos);
        if (key_pos == std::string::npos) {
            break;
        }
        const std::size_t colon = json.find(':', key_pos + 7);
        if (colon == std::string::npos) {
            break;
        }
        const std::size_t start = json.find_first_of("-0123456789", colon + 1);
        if (start == std::string::npos) {
            break;
        }
        std::size_t end = start;
        while (end < json.size()
               && (std::isdigit(static_cast<unsigned char>(json[end]))
                   || json[end] == '.'
                   || json[end] == '-'
                   || json[end] == '+'
                   || json[end] == 'e'
                   || json[end] == 'E')) {
            ++end;
        }
        try {
            closes.push_back(std::stod(json.substr(start, end - start)));
        } catch (...) {
        }
        pos = end;
    }
    return closes;
}

std::string replace_year_component(const std::string& iso8601, int delta_years)
{
    if (iso8601.size() < 4) {
        return iso8601;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(iso8601[i]))) {
            return iso8601;
        }
    }

    try {
        int year = std::stoi(iso8601.substr(0, 4));
        year = std::max(1970, year - delta_years);
        std::ostringstream out;
        out << year << iso8601.substr(4);
        return out.str();
    } catch (...) {
        return iso8601;
    }
}

std::string make_historical_request_json(const std::string& symbol,
                                         const PluginInstance& instance,
                                         const std::string& from,
                                         const std::string& to)
{
    std::ostringstream out;
    out << "{"
        << "\"symbol\":\"" << escape_json(symbol) << "\","
        << "\"resolution\":\"" << escape_json(instance.resolution) << "\","
        << "\"dataSourceId\":\"" << escape_json(instance.data_source_id) << "\","
        << "\"from\":\"" << escape_json(from) << "\","
        << "\"to\":\"" << escape_json(to) << "\""
        << "}";
    return out.str();
}

std::string make_subscription_request_json(const std::vector<std::string>& symbols)
{
    std::ostringstream out;
    out << "{"
        << "\"symbols\":[";
    for (std::size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "\"" << escape_json(symbols[i]) << "\"";
    }
    out << "],"
        << "\"kindMask\":1"
        << "}";
    return out.str();
}

std::string make_model_data_json(const std::vector<std::pair<std::string, double>>& ranked,
                                 double threshold,
                                 double position_size)
{
    const double threshold_floor = std::max(threshold, 1e-9);
    std::ostringstream out;
    out << "{\"modelData\":[";
    for (std::size_t i = 0; i < ranked.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        const double probability = std::min(std::abs(ranked[i].second) / threshold_floor, 1.0);
        out << "{"
            << "\"symbol\":\"" << escape_json(ranked[i].first) << "\","
            << "\"direction\":1,"
            << "\"probability\":" << probability << ","
            << "\"amount\":" << position_size << ","
            << "\"currentPrice\":0.0"
            << "}";
    }
    out << "]}";
    return out.str();
}

void set_state_json(PluginInstance* instance,
                    const std::vector<std::pair<std::string, double>>& ranked,
                    const std::vector<std::string>& active_symbols)
{
    std::ostringstream out;
    out << "{"
        << "\"rankedCount\":" << ranked.size() << ","
        << "\"activeSymbols\":[";
    for (std::size_t i = 0; i < active_symbols.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "\"" << escape_json(active_symbols[i]) << "\"";
    }
    out << "],"
        << "\"topMomentum\":";
    if (!ranked.empty()) {
        out << ranked.front().second;
    } else {
        out << 0.0;
    }
    out << "}";
    instance->last_state_json = out.str();
}

void apply_config(PluginInstance* instance, const std::string& json)
{
    instance->config_json = json;
    instance->period = static_cast<int>(extract_json_number(json, "period", instance->period));
    instance->threshold = extract_json_number(json, "threshold", instance->threshold);
    instance->top_n = static_cast<int>(extract_json_number(json, "topN", instance->top_n));
    instance->position_size = extract_json_number(json, "positionSize", instance->position_size);
    instance->resolution = extract_json_string(json, "resolution", instance->resolution);
    instance->data_source_id = extract_json_string(json, "dataSourceId", instance->data_source_id);
    instance->lookback_years =
        static_cast<int>(extract_json_number(json, "lookbackYears", instance->lookback_years));
}

} // namespace

extern "C" {

static void* create_extension(const char* extension_id,
                              const ibtrade_host_services_v1* host_services,
                              ibtrade_owned_string_v1* error_out)
{
    auto* instance = new PluginInstance();
    instance->host = host_services;
    instance->extension_id = extension_id ? extension_id : "";
    instance->last_state_json = "{\"lifecycle\":\"created\"}";
    if (error_out) {
        *error_out = {};
    }
    return instance;
}

static void destroy_extension(void* instance_ptr)
{
    delete static_cast<PluginInstance*>(instance_ptr);
}

static int initialize_instance(void* instance_ptr, ibtrade_owned_string_v1* error_out)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    instance->last_state_json = "{\"lifecycle\":\"initialized\"}";
    if (error_out) {
        *error_out = {};
    }
    return 1;
}

static void shutdown_instance(void* instance_ptr)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    instance->last_state_json = "{\"lifecycle\":\"shutdown\"}";
}

static int set_config_json(void* instance_ptr,
                           const char* config_json,
                           ibtrade_owned_string_v1* error_out)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    apply_config(instance, config_json ? config_json : "{}");
    if (error_out) {
        *error_out = {};
    }
    return 1;
}

static ibtrade_owned_string_v1 get_state_json(void* instance_ptr)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    return make_owned_string(instance->last_state_json);
}

static ibtrade_owned_string_v1 invoke_json(void* instance_ptr,
                                           const char* operation,
                                           const char* input_json,
                                           ibtrade_owned_string_v1* error_out)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    const std::string op = operation ? operation : "";
    const std::string input = input_json ? input_json : "{}";
    if (error_out) {
        *error_out = {};
    }

    if (op == "process_semantic") {
        const std::vector<std::string> symbols = extract_symbols_from_model_data(input);
        if (symbols.empty() || !instance->host || !instance->host->read_historical_bars_json) {
            set_state_json(instance, {}, {});
            return make_owned_string("{\"modelData\":[]}");
        }

        const std::string now = call_host_json(instance->host->current_time_iso8601, instance->host);
        const std::string from = replace_year_component(now, std::max(instance->lookback_years, 1));
        const int period = std::max(instance->period, 1);

        std::vector<std::pair<std::string, double>> ranked;
        for (const std::string& symbol : symbols) {
            const std::string request =
                make_historical_request_json(symbol, *instance, from, now);
            const std::string bars_json =
                call_host_json(instance->host->read_historical_bars_json, instance->host, request);
            const std::vector<double> closes = extract_bar_closes(bars_json);
            if (static_cast<int>(closes.size()) <= period) {
                continue;
            }
            const double base_close = closes[closes.size() - 1 - period];
            const double last_close = closes.back();
            if (base_close <= 0.0) {
                continue;
            }
            ranked.emplace_back(symbol, (last_close - base_close) / base_close);
        }

        std::sort(ranked.begin(), ranked.end(),
                  [](const auto& left, const auto& right) {
                      return left.second > right.second;
                  });

        const std::size_t count =
            std::min<std::size_t>(ranked.size(), static_cast<std::size_t>(std::max(instance->top_n, 0)));
        std::vector<std::pair<std::string, double>> selected;
        std::vector<std::string> active_symbols;
        selected.reserve(count);
        active_symbols.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            selected.push_back(ranked[i]);
            active_symbols.push_back(ranked[i].first);
        }

        if (instance->host->set_subscription_request_json) {
            ibtrade_owned_string_v1 subscription_error{};
            const std::string request = make_subscription_request_json(active_symbols);
            instance->host->set_subscription_request_json(
                instance->host->user_data,
                instance->extension_id.c_str(),
                request.c_str(),
                &subscription_error);
            release_owned_string(subscription_error);
        }

        set_state_json(instance, ranked, active_symbols);
        return make_owned_string(
            make_model_data_json(selected, instance->threshold, instance->position_size));
    }

    if (op == "on_tick" || op == "on_bar_close" || op == "on_tick_by_tick") {
        return make_owned_string("{}");
    }

    if (error_out) {
        *error_out = make_owned_string("Unsupported momentum plugin operation");
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
