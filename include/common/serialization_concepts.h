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

/**
 * @file serialization_concepts.h
 * @brief C++20 Concepts for Type-Safe Serialization
 * @requires C++20 or later
 */

static_assert(__cplusplus >= 202002L, "This header requires C++20 or later");

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

#include "common/helper.h"
#include "common/native_serializable.h"

namespace serialization
{

//-----------------------------------------------------------------------------
// Core Serialization Concepts
//-----------------------------------------------------------------------------

/**
 * @brief Concept for types that can be directly serialized (arithmetic, enums,
 * strings, and client native scalars specialized via native_serializable)
 */
template <typename T>
concept BaseSerializable =
    (std::is_arithmetic_v<T> && !std::is_pointer_v<T> && !std::is_array_v<T>) ||
    std::same_as<T, const char*> || std::same_as<T, std::string> || std::is_enum_v<T> ||
    std::same_as<T, std::monostate> || NativeSerializable<T>;

/**
 * @brief Concept for container types with standard container interface
 */
template <typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::size_type;
    typename T::iterator;
    typename T::const_iterator;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
    { std::as_const(t).begin() } -> std::same_as<typename T::const_iterator>;
    { std::as_const(t).end() } -> std::same_as<typename T::const_iterator>;
    { t.size() } -> std::convertible_to<typename T::size_type>;
};

/**
 * @brief Concept for containers that support reserve operation
 */
template <typename T>
concept Reservable = Container<T> && requires(T t, typename T::size_type n) {
    { t.reserve(n) } -> std::same_as<void>;
};

/**
 * @brief Concept for containers with random access
 */
template <typename T>
concept RandomAccessContainer = Container<T> && std::random_access_iterator<typename T::iterator>;

/**
 * @brief Concept for containers with emplace_back support
 */
template <typename T>
concept EmplaceBackable = Container<T> && requires(T t) {
    { t.emplace_back() } -> std::same_as<typename T::reference>;
};

/**
 * @brief Concept for associative containers (sets, maps)
 */
template <typename T>
concept AssociativeContainer = Container<T> && requires { typename T::key_type; };

/**
 * @brief Concept for map-like containers (has mapped_type)
 */
template <typename T>
concept MapLike = AssociativeContainer<T> && requires { typename T::mapped_type; };

/**
 * @brief Concept for set-like containers (no mapped_type)
 */
template <typename T>
concept SetLike = AssociativeContainer<T> && !MapLike<T>;

/**
 * @brief Concept for types that have reflection metadata
 */
template <typename T>
concept Reflectable = requires {
    { access::serializer::tuple<T>() };
};

/**
 * @brief Concept for types that can be default constructed
 */
template <typename T>
concept DefaultConstructible = std::default_initializable<T>;

/**
 * @brief Concept for smart pointer types
 */
template <typename T>
concept SmartPointer = requires(T t) {
    { t.get() } -> std::convertible_to<typename T::element_type*>;
    { t.reset() } -> std::same_as<void>;
    { static_cast<bool>(t) } -> std::same_as<bool>;
    typename T::element_type;
};

/**
 * @brief Concept for unique pointer types
 */
template <typename T>
concept UniquePointer = SmartPointer<T> && requires(T t) {
    { t.release() } -> std::convertible_to<typename T::element_type*>;
} && !std::copy_constructible<T>;

/**
 * @brief Concept for shared pointer types
 */
template <typename T>
concept SharedPointer = SmartPointer<T> && std::copy_constructible<T>;

/**
 * @brief Concept for archiver types
 */
template <typename A>
concept Archiver = requires { typename A::value_type; } || requires(A a, std::size_t idx) {
    { a.size() } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Concept for types that support serialization
 */
template <typename T, typename A>
concept Serializable = requires(A& archive, const T& obj) {
    { save(archive, obj) } -> std::same_as<void>;
};

/**
 * @brief Concept for types that support deserialization
 */
template <typename T, typename A>
concept Deserializable = requires(A& archive, T& obj) {
    { load(archive, obj) } -> std::same_as<void>;
};

/**
 * @brief Concept for fully serializable types (both save and load)
 */
template <typename T, typename A>
concept FullySerializable = Serializable<T, A> && Deserializable<T, A>;

/**
 * @brief Concept for types with noexcept move operations
 */
template <typename T>
concept NothrowMovable = std::move_constructible<T> && std::is_nothrow_move_constructible_v<T> &&
                         std::is_nothrow_move_assignable_v<T>;

/**
 * @brief Concept for range-based containers
 */
template <typename T>
concept Range = std::ranges::range<T>;

/**
 * @brief Concept for sized ranges
 */
template <typename T>
concept SizedRange = Range<T> && std::ranges::sized_range<T>;

/**
 * @brief Concept for contiguous containers (like vector, array)
 */
template <typename T>
concept ContiguousContainer =
    Container<T> && std::contiguous_iterator<typename T::iterator> && requires(T t) {
        { t.data() } -> std::same_as<typename T::value_type*>;
    };

/**
 * @brief Concept for types that can be serialized as trivially copyable
 */
template <typename T>
concept TriviallyCopyableSerializable = std::is_trivially_copyable_v<T> && BaseSerializable<T>;

/**
 * @brief Concept for types with a hash function
 */
template <typename T>
concept Hashable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Concept for tuple-like types
 */
template <typename T>
concept TupleLike = requires {
    typename std::tuple_size<T>::type;
    requires std::
        derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
};

/**
 * @brief Concept for variant-like types
 */
template <typename T>
concept VariantLike = requires(T t) {
    { t.index() } -> std::convertible_to<std::size_t>;
    {
        std::visit([](auto&&) {}, t)
    };
};

/**
 * @brief Concept for optional-like types
 */
template <typename T>
concept OptionalLike = requires(T t) {
    { t.has_value() } -> std::same_as<bool>;
    { t.value() } -> std::convertible_to<typename T::value_type>;
    typename T::value_type;
};

//-----------------------------------------------------------------------------
// Helper Concepts for Implementation Details
//-----------------------------------------------------------------------------

/**
 * @brief Concept for types that need deep copy during serialization
 */
template <typename T>
concept RequiresDeepCopy =
    !std::is_trivially_copyable_v<T> && (Container<T> || Reflectable<T> || SmartPointer<T>);

/**
 * @brief Concept for types that can use memcpy for serialization
 */
template <typename T>
concept MemcpySerializable =
    std::is_trivially_copyable_v<T> && std::has_unique_object_representations_v<T>;

/**
 * @brief Concept for types with custom serialization methods
 */
template <typename T, typename A>
concept HasCustomSerialization = requires(T t, A& archive) {
    { t.serialize(archive) } -> std::same_as<void>;
};

/**
 * @brief Concept for types with custom deserialization methods
 */
template <typename T, typename A>
concept HasCustomDeserialization = requires(T t, A& archive) {
    { t.deserialize(archive) } -> std::same_as<void>;
};

/**
 * @brief Concept for types that require versioning
 */
template <typename T>
concept Versionable = requires {
    { T::serialization_version } -> std::convertible_to<std::uint32_t>;
};

/**
 * @brief Concept for const-qualified types
 */
template <typename T>
concept ConstQualified = std::is_const_v<std::remove_reference_t<T>>;

/**
 * @brief Concept for mutable (non-const) types
 */
template <typename T>
concept MutableType = !ConstQualified<T>;

}  // namespace serialization
