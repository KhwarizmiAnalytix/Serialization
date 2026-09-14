/* Copyright 2018 The Serialization Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include <array>        // for array
#include <cstdint>      // for uint32_t, uint64_t, int64_t
#include <iosfwd>       // for size_t
#include <iterator>     // for pair
#include <new>          // for operator new
#include <type_traits>  // for declval, false_type, true_type, enable...
#include <utility>      // for forward, pair
#include <variant>      // for monostate

#include "archive/native_serializable.h"
#include "reflection/helper.h"
#include "util/macros.h"
#include "util/pointer.h"

namespace serialization
{

//-----------------------------------------------------------------------------
template <typename T>
struct is_base_serializable
{
    static constexpr bool value =
        ((std::is_arithmetic<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value) ||
         std::is_same<T, const char*>::value || std::is_same<T, std::string>::value ||
         std::is_enum<T>::value || std::is_same<T, std::monostate>::value ||
         is_native_serializable_v<T>);
};

//-----------------------------------------------------------------------------
template <typename Type, typename = void>
struct is_basic_container : std::false_type
{
};

template <typename Type>
struct is_basic_container<
    Type,
    void_t<
        decltype(std::declval<Type&>().size()),
        decltype(std::declval<Type&>().begin()),
        decltype(std::declval<Type&>().end()),
        decltype(std::declval<Type&>().resize(std::size_t()))>> : std::true_type
{
};

//-----------------------------------------------------------------------------
template <typename T, typename = void>
struct is_container : std::false_type
{
};

// Specialization for types that have container-like properties
template <typename T>
struct is_container<
    T,
    std::void_t<
        // Check for begin() and end() methods
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        // Check for value_type
        typename T::value_type,
        // Check for size_type
        typename T::size_type,
        // Check for iterator
        typename T::iterator,
        // Check for const_iterator
        typename T::const_iterator>> : std::true_type
{
};

// Helper variable template
template <typename T>
inline constexpr bool is_container_v = is_container<T>::value;

//-----------------------------------------------------------------------------
template <typename T, typename = void>
struct has_key_type : std::false_type
{
};

template <typename T>
struct has_key_type<T, std::void_t<typename T::key_type>> : std::true_type
{
};

// Helper to check if a type has a mapped_type
template <typename T, typename = void>
struct has_mapped_type : std::false_type
{
};

template <typename T>
struct has_mapped_type<T, std::void_t<typename T::mapped_type>> : std::true_type
{
};

// Primary template
template <typename T, typename = void>
struct is_associative_container : std::false_type
{
};

// Specialization for associative containers
template <typename T>
struct is_associative_container<
    T,
    std::enable_if_t<has_key_type<T>::value && is_container<T>::value>> : std::true_type
{
};

// Helper to check if a container is map-like
template <typename T>
struct is_map_like
    : std::bool_constant<is_associative_container<T>::value && has_mapped_type<T>::value>
{
};

// Helper variable templates
template <typename T>
inline constexpr bool is_associative_container_v = is_associative_container<T>::value;

template <typename T>
inline constexpr bool is_map_like_v = is_map_like<T>::value;

//-----------------------------------------------------------------------------
// Remove const of container value_type
template <typename Container, typename = void>
struct container_nonconst_value_type
{
    using type = std::remove_const_t<typename Container::value_type>;
};

//-----------------------------------------------------------------------------
// Same as above, except in case of std::map and std::unordered_map, and
// similar, we also need to remove the const of the key type.
template <
    template <typename...> class Container,
    typename KeyType,
    typename MappedType,
    typename... ExtraTypes>
struct container_nonconst_value_type<
    Container<KeyType, MappedType, ExtraTypes...>,
    void_t<
        // Require existence of key_type.
        typename Container<KeyType, MappedType, ExtraTypes...>::key_type,

        // Require existence of mapped_type.
        typename Container<KeyType, MappedType, ExtraTypes...>::mapped_type,

        // Require that the value type is a pair of const KeyType and
        // MappedType.
        std::enable_if_t<std::is_same<
            std::pair<const KeyType, MappedType>,
            typename Container<KeyType, MappedType, ExtraTypes...>::value_type>::value>>>
{
    using type = std::pair<KeyType, MappedType>;
};

template <typename Container>
using container_nonconst_value_type_t = typename container_nonconst_value_type<Container>::type;

//-----------------------------------------------------------------------------
// Checks if has 'data()' member function.
template <typename Type, typename = void>
struct has_data_member_function : std::false_type
{
};

template <typename Type>
struct has_data_member_function<Type, void_t<decltype(std::declval<Type&>().data())>>
    : std::true_type
{
};

//-----------------------------------------------------------------------------
// Checks if has 'properties()' member function.
template <typename Type, typename = void>
struct has_reflection : std::false_type
{
};

template <typename Type>
struct has_reflection<Type, void_t<decltype(access::serializer::tuple<Type>())>> : std::true_type
{
};

//----------------------------------------------------------------------------
template <typename T>
struct always_false : std::false_type
{
};

//----------------------------------------------------------------------------
template <typename T, typename = void>
struct has_reserve : std::false_type
{
};

template <typename T>
struct has_reserve<
    T,
    std::void_t<decltype(std::declval<T>().reserve(std::declval<typename T::size_type>()))>>
    : std::true_type
{
};

// Helper variable template for easier usage
template <typename T>
inline constexpr bool has_reserve_v = has_reserve<T>::value;

//----------------------------------------------------------------------------
template <typename T, typename = void>
struct has_random_access : std::false_type
{
};

template <typename T>
struct has_random_access<
    T,
    std::void_t<
        typename std::iterator_traits<typename T::iterator>::iterator_category,
        std::enable_if_t<std::is_base_of_v<
            std::random_access_iterator_tag,
            typename std::iterator_traits<typename T::iterator>::iterator_category>>>>
    : std::true_type
{
};

//----------------------------------------------------------------------------
template <typename T, typename = void>
struct has_emplace_back : std::false_type
{
};

template <typename T>
struct has_emplace_back<T, std::void_t<decltype(std::declval<T>().emplace_back())>> : std::true_type
{
};

// Helper variable template for easier usage
template <typename T>
inline constexpr bool has_emplace_back_v = has_emplace_back<T>::value;

// Helper variable template for easier usage
template <typename T>
inline constexpr bool has_random_access_v = has_random_access<T>::value;
}  // namespace serialization
