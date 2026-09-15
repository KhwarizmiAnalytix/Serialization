#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace serialization
{
enum class error_code
{
    missing_field,
    invalid_value,
    size_mismatch,
    unsupported_version,
    unknown_type,
    duplicate_type,
    truncated_input,
    depth_limit,
    cycle,
    invalid_registry
};

class serialization_error : public std::runtime_error
{
public:
    serialization_error(error_code code, std::string message, std::string path = {})
        : std::runtime_error(path.empty() ? message : path + ": " + message),
          code_(code), message_(std::move(message)), path_(std::move(path)) {}

    error_code code() const noexcept { return code_; }
    const std::string& path() const noexcept { return path_; }
    const std::string& message() const noexcept { return message_; }

private:
    error_code code_;
    std::string message_;
    std::string path_;
};
}
