#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "core/error.h"

namespace serialization::adapter_detail
{
template <class To, class From>
To checked_integer(From value)
{
    static_assert(std::is_integral_v<To> && std::is_integral_v<From>);
    bool valid;
    if constexpr (std::is_signed_v<From>)
    {
        const auto signed_value = static_cast<std::intmax_t>(value);
        if (signed_value < 0)
        {
            if constexpr (std::is_signed_v<To>)
                valid = signed_value >= static_cast<std::intmax_t>(std::numeric_limits<To>::min());
            else
                valid = false;
        }
        else
            valid = static_cast<std::uintmax_t>(value) <=
                    static_cast<std::uintmax_t>(std::numeric_limits<To>::max());
    }
    else
        valid = static_cast<std::uintmax_t>(value) <=
                static_cast<std::uintmax_t>(std::numeric_limits<To>::max());
    if (!valid)
        throw serialization_error(
            error_code::invalid_value, "Integer is outside destination range");
    return static_cast<To>(value);
}

template <class To, class From>
To checked_float(From value)
{
    const auto wide = static_cast<long double>(value);
    if (!std::isfinite(wide) || wide < std::numeric_limits<To>::lowest() ||
        wide > std::numeric_limits<To>::max())
        throw serialization_error(
            error_code::invalid_value, "Floating-point value is outside destination range");
    return static_cast<To>(value);
}
}  // namespace serialization::adapter_detail
