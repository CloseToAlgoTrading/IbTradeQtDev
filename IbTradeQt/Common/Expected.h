#ifndef EXPECTED_H
#define EXPECTED_H

#include "../ThirdParty/expected.hpp"
#include <string>

template<typename T, typename E = struct Error>
using Expected = tl::expected<T, E>;

template<typename E>
auto make_unexpected(E&& error) {
    return tl::unexpected<std::decay_t<E>>(std::forward<E>(error));
}

enum class ErrorCode {
    Success = 0,
    InvalidArgument,
    NotFound,
    BrokerConnectionFailed,
    DatabaseError,
    OrderRejected,
    InsufficientFunds,
    ConfigurationError,
    Timeout,
    UnknownError
};

struct Error {
    ErrorCode code;
    std::string message;
    std::string context;

    std::string toString() const {
        return "Error " + std::to_string(static_cast<int>(code)) +
               ": " + message + " (context: " + context + ")";
    }
};

#endif // EXPECTED_H
