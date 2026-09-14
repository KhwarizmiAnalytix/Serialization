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

#include <string>
#include <type_traits>

namespace serialization
{

/**
 * Client extension point for domain scalars (datetime, tenor, key, …).
 *
 * The library has no knowledge of those types. Specialize this trait in
 * namespace serialization so save/load treat T as a native value: JSON/XML
 * write the wire representation, not a Class/properties object.
 *
 * Required members when value is true:
 *   using wire_type = <a library primitive: arithmetic, enum, or std::string>;
 *   static wire_type to_wire(const T&);
 *   static void from_wire(T&, const wire_type&);
 *
 * Prefer SERIALIZATION_NATIVE_CAST / SERIALIZATION_NATIVE_STRING at global
 * scope instead of writing the specialization by hand.
 */
template <typename T>
struct native_serializable : std::false_type
{
};

template <typename T>
inline constexpr bool is_native_serializable_v = native_serializable<std::remove_cv_t<T>>::value;

template <typename T>
concept NativeSerializable = is_native_serializable_v<T>;

}  // namespace serialization

/**
 * Treat Type as a native scalar by converting to/from WireType.
 * WireType must be a library primitive (e.g. double, int), not another native type.
 *
 *   SERIALIZATION_NATIVE_CAST(quant::datetime, double);
 */
#define SERIALIZATION_NATIVE_CAST(Type, WireType)                 \
    namespace serialization                                       \
    {                                                             \
    template <>                                                   \
    struct native_serializable<Type> : std::true_type             \
    {                                                             \
        using wire_type = WireType;                               \
        static wire_type to_wire(const Type& value)               \
        {                                                         \
            return static_cast<WireType>(value);                  \
        }                                                         \
        static void from_wire(Type& value, const wire_type& wire) \
        {                                                         \
            value = Type(wire);                                   \
        }                                                         \
    };                                                            \
    }

/**
 * Treat Type as a native string via to_string() and Type(std::string).
 *
 *   SERIALIZATION_NATIVE_STRING(quant::tenor);
 */
#define SERIALIZATION_NATIVE_STRING(Type)                           \
    namespace serialization                                         \
    {                                                               \
    template <>                                                     \
    struct native_serializable<Type> : std::true_type               \
    {                                                               \
        using wire_type = std::string;                              \
        static std::string to_wire(const Type& value)               \
        {                                                           \
            return value.to_string();                               \
        }                                                           \
        static void from_wire(Type& value, const std::string& wire) \
        {                                                           \
            value = Type(wire);                                     \
        }                                                           \
    };                                                              \
    }
