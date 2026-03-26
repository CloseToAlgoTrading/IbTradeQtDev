#include "../../../sdk/PluginSdk.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

using namespace ibtrade::sdk;

namespace {

enum class ExtensionKind {
    Selection,
    Alpha,
    Rebalance,
    Risk,
    Execution
};

struct PluginInstance {
    ExtensionKind kind;
    const ibtrade_host_services_v1* host = nullptr;
    std::string extension_id;
    std::string config_json = "{}";
    std::string last_state_json = "{}";
    double fixed_probability = 0.75;
    double fixed_amount = 25.0;
    double target_size = 10.0;
    double max_abs_target = 50.0;
    int selection_limit = 2;
};

std::vector<std::string> extract_string_values_for_key(const std::string& json,
                                                       const std::string& key)
{
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return {};
    }
    const std::size_t openBracket = json.find('[', keyPos + needle.size());
    const std::size_t closeBracket = json.find(']', openBracket + 1);
    if (openBracket == std::string::npos || closeBracket == std::string::npos) {
        return {};
    }

    std::vector<std::string> values;
    bool inString = false;
    bool escaped = false;
    std::ostringstream current;
    for (std::size_t i = openBracket + 1; i < closeBracket; ++i) {
        const char ch = json[i];
        if (!inString) {
            if (ch == '"') {
                inString = true;
                current.str({});
                current.clear();
            }
            continue;
        }
        if (escaped) {
            current << ch;
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            values.push_back(current.str());
            inString = false;
            continue;
        }
        current << ch;
    }
    return values;
}

std::vector<std::string> extract_symbols_from_model_data(const std::string& json)
{
    std::vector<std::string> symbols;
    std::size_t pos = 0;
    while (true) {
        const std::size_t keyPos = json.find("\"symbol\"", pos);
        if (keyPos == std::string::npos) {
            break;
        }
        const std::size_t firstQuote = json.find('"', json.find(':', keyPos) + 1);
        const std::size_t secondQuote = json.find('"', firstQuote + 1);
        if (firstQuote == std::string::npos || secondQuote == std::string::npos) {
            break;
        }
        symbols.push_back(json.substr(firstQuote + 1, secondQuote - firstQuote - 1));
        pos = secondQuote + 1;
    }
    return symbols;
}

std::vector<std::pair<std::string, double>> extract_execution_intents(const std::string& json)
{
    std::vector<std::pair<std::string, double>> intents;
    const std::string needle = "\"intents\"";
    const std::size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return intents;
    }

    const std::size_t openBracket = json.find('[', keyPos + needle.size());
    const std::size_t closeBracket = json.find(']', openBracket + 1);
    if (openBracket == std::string::npos || closeBracket == std::string::npos) {
        return intents;
    }

    int braceDepth = 0;
    std::size_t objectStart = std::string::npos;
    for (std::size_t i = openBracket + 1; i < closeBracket; ++i) {
        const char ch = json[i];
        if (ch == '{') {
            if (braceDepth == 0) {
                objectStart = i;
            }
            ++braceDepth;
            continue;
        }
        if (ch != '}') {
            continue;
        }

        --braceDepth;
        if (braceDepth == 0 && objectStart != std::string::npos) {
            const std::string objectJson = json.substr(objectStart, i - objectStart + 1);
            const std::string symbol = extract_json_string(objectJson, "symbol", "");
            const double quantity = extract_json_number(objectJson, "quantity", 0.0);
            if (!symbol.empty()) {
                intents.emplace_back(symbol, quantity);
            }
            objectStart = std::string::npos;
        }
    }

    return intents;
}

std::string make_signal_json(const std::string& symbol, double probability)
{
    std::ostringstream out;
    out << "{"
        << "\"symbol\":\"" << escape_json(symbol) << "\","
        << "\"confidence\":" << probability << ","
        << "\"direction\":0,"
        << "\"correlationId\":\"\","
        << "\"timestamp\":\"\","
        << "\"alphaBlockId\":\"external-example\","
        << "\"suggestedQuantity\":0.0"
        << "}";
    return out.str();
}

std::string make_model_data_json(const std::vector<std::string>& symbols,
                                 double probability,
                                 double amount)
{
    std::ostringstream out;
    out << "{\"modelData\":[";
    for (std::size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "{"
            << "\"symbol\":\"" << escape_json(symbols[i]) << "\","
            << "\"direction\":1,"
            << "\"probability\":" << probability << ","
            << "\"amount\":" << amount << ","
            << "\"currentPrice\":0.0"
            << "}";
    }
    out << "]}";
    return out.str();
}

std::string make_targets_json(const std::vector<std::string>& symbols,
                              double targetSize,
                              const std::string& holdings_json)
{
    std::ostringstream out;
    out << "{\"targets\":[";
    for (std::size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        const double currentQuantity = extract_json_number(holdings_json, symbols[i], 0.0);
        out << "{"
            << "\"symbol\":\"" << escape_json(symbols[i]) << "\","
            << "\"targetQuantity\":" << targetSize << ","
            << "\"currentQuantity\":" << currentQuantity << ","
            << "\"reason\":\"external rebalance\","
            << "\"correlationId\":\"\","
            << "\"timestamp\":\"\""
            << "}";
    }
    out << "]}";
    return out.str();
}

std::string make_risk_decision_json(double targetQuantity,
                                    double maxAbsTarget)
{
    if (std::fabs(targetQuantity) <= maxAbsTarget) {
        return "{\"action\":\"Approve\",\"reason\":\"within limit\"}";
    }
    const double capped = targetQuantity > 0.0 ? maxAbsTarget : -maxAbsTarget;
    std::ostringstream out;
    out << "{"
        << "\"action\":\"Modify\","
        << "\"reason\":\"capped by external risk\","
        << "\"modifiedQuantity\":" << capped
        << "}";
    return out.str();
}

void apply_config(PluginInstance* instance, const std::string& json)
{
    instance->config_json = json;
    instance->fixed_probability = extract_json_number(json, "fixed_probability", instance->fixed_probability);
    instance->fixed_amount = extract_json_number(json, "fixed_amount", instance->fixed_amount);
    instance->target_size = extract_json_number(json, "target_size", instance->target_size);
    instance->max_abs_target = extract_json_number(json, "max_abs_target", instance->max_abs_target);
    instance->selection_limit = static_cast<int>(extract_json_number(json, "limit", instance->selection_limit));
}

void set_state(PluginInstance* instance, const std::string& state_json)
{
    instance->last_state_json = state_json;
}

} // namespace

extern "C" {

static void* create_extension(const char* extension_id,
                              const ibtrade_host_services_v1* host_services,
                              ibtrade_owned_string_v1* error_out)
{
    const std::string id = extension_id ? extension_id : "";
    auto* instance = new PluginInstance();
    instance->host = host_services;
    instance->extension_id = id;

    if (id.find("static-selection") != std::string::npos) {
        instance->kind = ExtensionKind::Selection;
    } else if (id.find("semantic-alpha") != std::string::npos) {
        instance->kind = ExtensionKind::Alpha;
    } else if (id.find("fixed-rebalance") != std::string::npos) {
        instance->kind = ExtensionKind::Rebalance;
    } else if (id.find("cap-risk") != std::string::npos) {
        instance->kind = ExtensionKind::Risk;
    } else if (id.find("host-execution") != std::string::npos) {
        instance->kind = ExtensionKind::Execution;
    } else {
        delete instance;
        if (error_out) {
            *error_out = make_owned_string("Unknown extension id");
        }
        return nullptr;
    }

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
    set_state(instance, "{\"lifecycle\":\"initialized\"}");
    if (error_out) {
        *error_out = {};
    }
    return 1;
}

static void shutdown_instance(void* instance_ptr)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    set_state(instance, "{\"lifecycle\":\"shutdown\"}");
}

static int set_config_json(void* instance_ptr,
                           const char* config_json,
                           ibtrade_owned_string_v1* error_out)
{
    auto* instance = static_cast<PluginInstance*>(instance_ptr);
    apply_config(instance, config_json ? config_json : "{}");
    set_state(instance, "{\"config\":\"updated\"}");
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

    switch (instance->kind) {
    case ExtensionKind::Selection: {
        if (op == "select") {
            const auto universe = extract_string_values_for_key(input, "universe");
            std::ostringstream out;
            out << "{\"selected\":[";
            const std::size_t limit = std::min<std::size_t>(
                universe.size(),
                instance->selection_limit > 0 ? static_cast<std::size_t>(instance->selection_limit) : universe.size());
            for (std::size_t i = 0; i < limit; ++i) {
                if (i > 0) {
                    out << ',';
                }
                out << "\"" << escape_json(universe[i]) << "\"";
            }
            out << "]}";
            set_state(instance, "{\"selection\":\"complete\"}");
            return make_owned_string(out.str());
        }
        break;
    }
    case ExtensionKind::Alpha: {
        if (op == "on_tick") {
            const std::string symbol = extract_json_string(input, "symbol", "UNKNOWN");
            emit_event(instance->host, "alpha.signal_generated",
                       make_signal_json(symbol, instance->fixed_probability));
            set_state(instance, "{\"alpha\":\"tick\"}");
            return make_owned_string("{}");
        }
        if (op == "process_semantic") {
            const auto symbols = extract_symbols_from_model_data(input);
            set_state(instance, "{\"alpha\":\"semantic\"}");
            return make_owned_string(
                make_model_data_json(symbols, instance->fixed_probability, instance->fixed_amount));
        }
        if (op == "on_bar_close" || op == "on_tick_by_tick") {
            return make_owned_string("{}");
        }
        break;
    }
    case ExtensionKind::Rebalance: {
        if (op == "rebalance") {
            const auto symbols = extract_symbols_from_model_data(input);
            set_state(instance, "{\"rebalance\":\"complete\"}");
            const std::string holdingsJson =
                (instance->host && instance->host->read_holdings_json)
                ? call_host_json(instance->host->read_holdings_json, instance->host)
                : "{}";
            return make_owned_string(
                make_targets_json(symbols, instance->target_size, holdingsJson));
        }
        break;
    }
    case ExtensionKind::Risk: {
        if (op == "evaluate") {
            const double targetQuantity = extract_json_number(input, "targetQuantity", 0.0);
            set_state(instance, "{\"risk\":\"evaluated\"}");
            return make_owned_string(make_risk_decision_json(targetQuantity, instance->max_abs_target));
        }
        if (op == "on_tick") {
            return make_owned_string("{}");
        }
        break;
    }
    case ExtensionKind::Execution: {
        if (op == "execute") {
            const auto intents = extract_execution_intents(input);
            for (const auto& intent : intents) {
                std::ostringstream intentJson;
                intentJson << "{"
                           << "\"symbol\":\"" << escape_json(intent.first) << "\","
                           << "\"quantity\":" << intent.second << ","
                           << "\"orderType\":0,"
                           << "\"riskApproval\":\"external\","
                           << "\"correlationId\":\"\","
                           << "\"timestamp\":\"\""
                           << "}";
                if (instance->host && instance->host->place_order_json) {
                    ibtrade_owned_string_v1 serviceError{};
                    instance->host->place_order_json(
                        instance->host->user_data,
                        intentJson.str().c_str(),
                        &serviceError);
                    release_owned_string(serviceError);
                }
                std::ostringstream eventJson;
                eventJson << "{"
                          << "\"symbol\":\"" << escape_json(intent.first) << "\","
                          << "\"orderId\":\"host\""
                          << "}";
                emit_event(instance->host, "execution.order_placed", eventJson.str());
            }
            set_state(instance, "{\"execution\":\"sent\"}");
            return make_owned_string("{}");
        }
        break;
    }
    }

    if (error_out) {
        *error_out = make_owned_string("Unsupported operation for extension");
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
