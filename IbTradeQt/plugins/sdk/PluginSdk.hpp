#ifndef IBTRADE_PLUGIN_SDK_HPP
#define IBTRADE_PLUGIN_SDK_HPP

#include "../api/ibtrade_plugin_api.h"

#include <cstring>
#include <memory>
#include <sstream>
#include <string>

namespace ibtrade::sdk {

inline ibtrade_owned_string_v1 make_owned_string(std::string value)
{
    char* raw = new char[value.size() + 1];
    std::memcpy(raw, value.c_str(), value.size() + 1);

    ibtrade_owned_string_v1 out{};
    out.data = raw;
    out.size = value.size();
    out.release = [](const char* data, size_t, void*) {
        delete[] data;
    };
    out.user_data = nullptr;
    return out;
}

inline void release_owned_string(ibtrade_owned_string_v1& value)
{
    if (value.release && value.data) {
        value.release(value.data, value.size, value.user_data);
    }
    value = {};
}

inline std::string to_string_copy(const ibtrade_owned_string_v1& value)
{
    if (!value.data || value.size == 0) {
        return {};
    }
    return std::string(value.data, value.size);
}

class ScopedString {
public:
    explicit ScopedString(ibtrade_owned_string_v1 value)
        : m_value(value)
    {
    }

    ~ScopedString()
    {
        release_owned_string(m_value);
    }

    std::string str() const
    {
        return to_string_copy(m_value);
    }

private:
    ibtrade_owned_string_v1 m_value{};
};

inline void emit_event(const ibtrade_host_services_v1* host,
                       const char* event_name,
                       const std::string& payload_json)
{
    if (host && host->emit_event_json) {
        host->emit_event_json(host->user_data, event_name, payload_json.c_str());
    }
}

inline void log_message(const ibtrade_host_services_v1* host,
                        ibtrade_log_level_v1 level,
                        const std::string& message)
{
    if (host && host->log_message) {
        host->log_message(host->user_data, level, message.c_str());
    }
}

inline std::string call_host_json(ibtrade_owned_string_v1 (*fn)(void*, const char*),
                                  const ibtrade_host_services_v1* host,
                                  const std::string& arg)
{
    if (!host || !fn) {
        return {};
    }
    ScopedString result(fn(host->user_data, arg.c_str()));
    return result.str();
}

inline std::string call_host_json(ibtrade_owned_string_v1 (*fn)(void*),
                                  const ibtrade_host_services_v1* host)
{
    if (!host || !fn) {
        return {};
    }
    ScopedString result(fn(host->user_data));
    return result.str();
}

inline double extract_json_number(const std::string& json,
                                  const std::string& key,
                                  double fallback)
{
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return fallback;
    }
    const std::size_t colon = json.find(':', keyPos + needle.size());
    if (colon == std::string::npos) {
        return fallback;
    }
    const std::size_t start = json.find_first_of("-0123456789", colon + 1);
    if (start == std::string::npos) {
        return fallback;
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
        return std::stod(json.substr(start, end - start));
    } catch (...) {
        return fallback;
    }
}

inline std::string extract_json_string(const std::string& json,
                                       const std::string& key,
                                       const std::string& fallback = {})
{
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return fallback;
    }
    const std::size_t colon = json.find(':', keyPos + needle.size());
    const std::size_t firstQuote = json.find('"', colon + 1);
    if (colon == std::string::npos || firstQuote == std::string::npos) {
        return fallback;
    }
    std::ostringstream out;
    bool escaped = false;
    for (std::size_t i = firstQuote + 1; i < json.size(); ++i) {
        const char ch = json[i];
        if (escaped) {
            out << ch;
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return out.str();
        }
        out << ch;
    }
    return fallback;
}

inline std::string escape_json(const std::string& value)
{
    std::ostringstream out;
    for (const char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << ch; break;
        }
    }
    return out.str();
}

} // namespace ibtrade::sdk

#endif // IBTRADE_PLUGIN_SDK_HPP
