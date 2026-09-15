#pragma once

#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "metadata/access.h"
#include "metadata/field.h"

namespace serialization
{
namespace generated
{
template <class T>
struct metadata;
}

struct macro_metadata
{
    template <class T>
    static constexpr auto properties() -> decltype(access::serializer::tuple<T>())
    {
        return access::serializer::tuple<T>();
    }
};

struct ast_metadata
{
    template <class T>
    static constexpr auto properties() -> decltype(generated::metadata<T>::properties())
    {
        return generated::metadata<T>::properties();
    }
};

template <class Provider, class T>
concept HasMetadata = requires { Provider::template properties<std::remove_cv_t<T>>(); };

// Specialize to opt into a durable record identifier/version. Ordinary records
// need neither RTTI names nor a process-global registration.
template <class T>
struct record_info
{
    static constexpr std::string_view type_id = {};
    static constexpr std::uint32_t    version = 0;
};
}  // namespace serialization
