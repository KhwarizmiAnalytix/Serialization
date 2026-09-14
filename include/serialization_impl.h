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


static_assert(__cplusplus >= 202002L, "This header requires C++20 or later");

#include <array>
#include <concepts>
#include <cstddef>
#include <exception>
#include <format>
#include <iterator>
#include <memory>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>

#include "common/archiver_wrapper.h"
#include "common/helper.h"
#include "common/reflection.h"
#include "common/serialization_concepts.h"
#include "common/serialization_type_traits.h"
#include "logging/util/exception.h"
#include "util/pointer.h"
#include "util/registry.h"

//-----------------------------------------------------------------------------
// Error handling — Logging exception.h (LOGGING_CHECK / LOGGING_THROW)
//-----------------------------------------------------------------------------
#define SERIALIZATION_THROW(...) LOGGING_THROW(__VA_ARGS__)
#define SERIALIZATION_CHECK(condition, ...) LOGGING_CHECK(condition, ##__VA_ARGS__)
#define SERIALIZATION_CHECK_DEBUG(condition, ...) LOGGING_CHECK_DEBUG(condition, ##__VA_ARGS__)

//-----------------------------------------------------------------------------
// Enhanced Error Handling with C++20
//-----------------------------------------------------------------------------
namespace serialization::detail
{

/**
 * @brief Cached type name to avoid repeated demangling
 */
template <typename T>
[[nodiscard]] inline const std::string& cached_type_name() noexcept
{
    static const std::string name = logging::demangle(typeid(T).name());
    return name;
}

/**
 * @brief Get type name for polymorphic objects with caching
 */
template <typename T>
[[nodiscard]] inline std::string polymorphic_type_name(const T* obj) noexcept
{
    if constexpr (std::is_polymorphic_v<T>)
    {
        static thread_local std::unordered_map<const std::type_info*, std::string> cache;
        const auto& type_info = typeid(*obj);

        auto it = cache.find(&type_info);
        if (it != cache.end()) [[likely]]
        {
            return it->second;
        }

        auto name         = logging::demangle(type_info.name());
        cache[&type_info] = name;
        return name;
    }
    else
    {
        return cached_type_name<T>();
    }
}

/**
 * @brief Serialization context for tracking depth and detecting cycles
 */
struct serialization_context
{
    std::size_t                  depth     = 0;
    static constexpr std::size_t max_depth = 1000;

    struct depth_guard
    {
        serialization_context& ctx;

        explicit depth_guard(serialization_context& c) : ctx(c)
        {
            ++ctx.depth;
            if (ctx.depth > ctx.max_depth) [[unlikely]]
            {
                SERIALIZATION_THROW(
                    "Serialization depth {} exceeds maximum {}", ctx.depth, ctx.max_depth);
            }
        }

        ~depth_guard() { --ctx.depth; }

        depth_guard(const depth_guard&)            = delete;
        depth_guard& operator=(const depth_guard&) = delete;
        depth_guard(depth_guard&&)                 = delete;
        depth_guard& operator=(depth_guard&&)      = delete;
    };

    [[nodiscard]] depth_guard enter() { return depth_guard(*this); }
};

}  // namespace serialization::detail

//-----------------------------------------------------------------------------
namespace serialization
{

//-----------------------------------------------------------------------------
// Helper type trait to detect reflection_empty type
//-----------------------------------------------------------------------------
template <typename T>
struct is_reflection_empty : std::false_type
{
};

template <typename Class>
struct is_reflection_empty<reflection_empty<Class>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_reflection_empty_v = is_reflection_empty<T>::value;

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
inline constexpr std::string_view VALUE_NAME = "Value";
inline constexpr std::string_view INDEX_NAME = "Index";
inline constexpr std::string_view EMPTY_NAME = "null object!";

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
template <typename Archiver, typename T>
    requires(BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void save(Archiver& archive, const T& obj);

template <typename Archiver, typename T>
    requires(BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void load(Archiver& archive, T& obj);

// Overloads for rvalue references (e.g., XML nodes returned by value)
// Constrained to only match actual rvalues (not forwarding references)
template <typename Archiver, typename T>
    requires(!std::is_lvalue_reference_v<Archiver>) &&
            (BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void save(Archiver&& archive, const T& obj);

template <typename Archiver, typename T>
    requires(!std::is_lvalue_reference_v<Archiver>) &&
            (BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void load(Archiver&& archive, T& obj);

//-----------------------------------------------------------------------------
// Registry registration helper with const-correctness
//-----------------------------------------------------------------------------
namespace detail
{
template <typename Archiver, typename T>
void save_polymorphic(Archiver& archive, const T& obj)
{
    serialization::save(archive, obj);
}

template <typename Archiver, typename T>
void load_polymorphic(Archiver& archive, T& obj)
{
    serialization::load(archive, obj);
}
}  // namespace detail

template <typename Archiver, typename T>
void register_serializer_impl(Archiver& archive, void* obj, bool load_obj)
{
    if (load_obj)
    {
        auto* obj_ptr       = static_cast<ptr_const<T>*>(obj);
        auto  loaded_object = serialization::access::serializer::make_ptr<T>();
        detail::load_polymorphic(archive, *loaded_object);
        obj_ptr->reset(loaded_object.release());
    }
    else
    {
        const auto* obj_ptr = static_cast<const T*>(obj);
        detail::save_polymorphic(archive, *obj_ptr);
    }
}

//-----------------------------------------------------------------------------
namespace impl
{

//-----------------------------------------------------------------------------
// Container serialization - Sequential containers (C++20 concepts)
//-----------------------------------------------------------------------------

template <typename Archiver, Container C>
    requires(!AssociativeContainer<C>)
void load_container(Archiver& archive, C& container)
{
    const size_t size = archiver_wrapper<Archiver>::size(archive);

    container.clear();

    if constexpr (Reservable<C>)
    {
        container.reserve(size);
    }

    for (size_t i = 0; i < size; ++i)
    {
        if constexpr (EmplaceBackable<C>)
        {
            auto& element = container.emplace_back();
            serialization::load(archiver_wrapper<Archiver>::get(archive, i), element);
        }
        else
        {
            typename C::value_type item;
            serialization::load(archiver_wrapper<Archiver>::get(archive, i), item);
            container.insert(container.end(), std::move(item));
        }
    }
}

template <typename Archiver, Container C>
    requires(!AssociativeContainer<C>)
void save_container(Archiver& archive, const C& container)
{
    const size_t size = container.size();
    archiver_wrapper<Archiver>::resize(archive, size);

    if constexpr (RandomAccessContainer<C>)
    {
        for (size_t i = 0; i < size; ++i)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, i), container[i]);
        }
    }
    else if constexpr (std::ranges::sized_range<C>)
    {
        size_t i = 0;
        for (const auto& item : container)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, i++), item);
        }
    }
    else
    {
        size_t i = 0;
        for (const auto& item : container)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, i), item);
            ++i;
        }
    }
}

//-----------------------------------------------------------------------------
// Associative container serialization
//-----------------------------------------------------------------------------
template <typename Archiver, AssociativeContainer C>
void load_associative_container(Archiver& archive, C& container)
{
    const size_t size = archiver_wrapper<Archiver>::size(archive);

    container.clear();

    if constexpr (Reservable<C>)
    {
        container.reserve(size);
    }

    if constexpr (MapLike<C>)
    {
        SERIALIZATION_CHECK(
            size % 2 == 0, "Invalid map serialization: odd number of elements ({})", size);

        for (size_t i = 0; i < size / 2; ++i)
        {
            typename C::key_type    key;
            typename C::mapped_type value;

            serialization::load(archiver_wrapper<Archiver>::get(archive, 2 * i), key);
            serialization::load(archiver_wrapper<Archiver>::get(archive, 2 * i + 1), value);

            container.emplace(std::move(key), std::move(value));
        }
    }
    else  // SetLike
    {
        for (size_t i = 0; i < size; ++i)
        {
            typename C::value_type value;
            serialization::load(archiver_wrapper<Archiver>::get(archive, i), value);
            container.emplace(std::move(value));
        }
    }
}

template <typename Archiver, AssociativeContainer C>
void save_associative_container(Archiver& archive, const C& container)
{
    const size_t size = container.size();

    if constexpr (MapLike<C>)
    {
        archiver_wrapper<Archiver>::resize(archive, 2 * size);

        size_t i = 0;
        for (const auto& [key, value] : container)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, i++), key);
            serialization::save(archiver_wrapper<Archiver>::get(archive, i++), value);
        }
    }
    else  // SetLike
    {
        archiver_wrapper<Archiver>::resize(archive, size);

        size_t i = 0;
        for (const auto& item : container)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, i++), item);
        }
    }
}

//-----------------------------------------------------------------------------
// Generic serializer implementation
//-----------------------------------------------------------------------------
template <typename Archiver, typename T>
struct serializer_impl
{
    //-------------------------------------------------------------------------
    // Save object with reflection
    //-------------------------------------------------------------------------
    static void save_object(Archiver& archive, const T* obj)
    {
        if (obj == nullptr)
        {
            archiver_wrapper<Archiver>::push_class_name(archive, std::string(EMPTY_NAME));
            return;
        }

        constexpr auto nbProperties =
            std::tuple_size_v<decltype(serialization::access::serializer::tuple<T>())>;

        const std::string& class_name = detail::polymorphic_type_name(obj);
        archiver_wrapper<Archiver>::push_class_name(archive, class_name);

        if constexpr (nbProperties > 0)
        {
            for_sequence(
                std::make_index_sequence<nbProperties>{},
                [&]<auto I>(std::integral_constant<std::size_t, I>)
                {
                    constexpr auto property =
                        std::get<I>(serialization::access::serializer::tuple<T>());
                    const auto& name = property.name();

                    if constexpr (!is_reflection_empty_v<std::decay_t<decltype(property)>>)
                    {
                        const auto& member_ref = obj->*(property.member());
                        serialization::save(archiver_wrapper<Archiver>::get(archive, name),
                                            member_ref);
                    }
                });
        }
    }

    //-------------------------------------------------------------------------
    // Load object with reflection
    //-------------------------------------------------------------------------
    static void load_object(Archiver& archive, T& obj)
    {
        constexpr auto nbProperties =
            std::tuple_size_v<decltype(serialization::access::serializer::tuple<T>())>;

        if constexpr (nbProperties > 0)
        {
            const auto class_name = archiver_wrapper<Archiver>::pop_class_name(archive);

            SERIALIZATION_CHECK(!class_name.empty(), "Invalid or missing class name");

            if (class_name != EMPTY_NAME)
            {
                for_sequence(
                    std::make_index_sequence<nbProperties>{},
                    [&]<auto I>(std::integral_constant<std::size_t, I>)
                    {
                        constexpr auto property =
                            std::get<I>(serialization::access::serializer::tuple<T>());
                        const auto& name = property.name();

                        if constexpr (!is_reflection_empty_v<std::decay_t<decltype(property)>>)
                        {
                            using member_type =
                                typename std::decay_t<decltype(property)>::member_type;
                            auto& member_ref = obj.*(property.member());
                            serialization::load<Archiver, member_type>(
                                archiver_wrapper<Archiver>::get(archive, name), member_ref);
                        }
                    });

                serialization::access::serializer::initialize(obj);
            }
        }
    }

    //-------------------------------------------------------------------------
    // Main save dispatcher with concepts
    //-------------------------------------------------------------------------
    static void save(Archiver& archive, const T& obj)
        requires BaseSerializable<T> || Container<T> || Reflectable<T> || std::is_pointer_v<T>
    {
        if constexpr (BaseSerializable<T>)
        {
            archiver_wrapper<Archiver>::push(archive, obj);
        }
        else if constexpr (Container<T>)
        {
            if constexpr (AssociativeContainer<T>)
            {
                save_associative_container(archive, obj);
            }
            else
            {
                save_container(archive, obj);
            }
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            using U = std::remove_pointer_t<T>;
            if constexpr (Reflectable<U>)
            {
                serializer_impl<Archiver, U>::save_object(archive, obj);
            }
            else
            {
                static_assert(
                    always_false<T>::value, "Pointer type without reflection not supported");
            }
        }
        else if constexpr (Reflectable<T>)
        {
            save_object(archive, &obj);
        }
        else
        {
            static_assert(always_false<T>::value, "Type not supported for serialization");
        }
    }

    //-------------------------------------------------------------------------
    // Main load dispatcher with concepts
    //-------------------------------------------------------------------------
    static void load(Archiver& archive, T& obj)
        requires BaseSerializable<T> || Container<T> || Reflectable<T>
    {
        if constexpr (BaseSerializable<T>)
        {
            archiver_wrapper<Archiver>::pop(archive, obj);
        }
        else if constexpr (Reflectable<T>)
        {
            load_object(archive, obj);
        }
        else if constexpr (Container<T>)
        {
            if constexpr (AssociativeContainer<T>)
            {
                load_associative_container(archive, obj);
            }
            else
            {
                load_container(archive, obj);
            }
        }
        else
        {
            static_assert(always_false<T>::value, "Type not supported for deserialization");
        }
    }
};

//-----------------------------------------------------------------------------
// std::array specialization
//-----------------------------------------------------------------------------
template <typename Archiver, typename Item, std::size_t Size>
struct serializer_impl<Archiver, std::array<Item, Size>>
{
    static void load(Archiver& archive, std::array<Item, Size>& array)
    {
        const auto archive_size = archiver_wrapper<Archiver>::size(archive);

        SERIALIZATION_CHECK(
            archive_size == Size,
            "Array size mismatch: expected {} but got {}",
            Size,
            archive_size);

        for (size_t i = 0; i < Size; ++i)
        {
            serialization::load(archiver_wrapper<Archiver>::get(archive, i), array[i]);
        }
    }

    static void save(Archiver& archive, const std::array<Item, Size>& array)
    {
        archiver_wrapper<Archiver>::resize(archive, Size);

        for (size_t i = 0; i < Size; ++i)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, i), array[i]);
        }
    }
};

//-----------------------------------------------------------------------------
// std::variant specialization
//-----------------------------------------------------------------------------
template <typename Archiver, typename... Types>
struct serializer_impl<Archiver, std::variant<Types...>>
{
    static void save(Archiver& archive, const std::variant<Types...>& variant)
    {
        static_assert(sizeof...(Types) <= 255, "Variant can contain at most 255 types");

        const auto variant_index = variant.index();

        SERIALIZATION_CHECK(
            variant_index != std::variant_npos, "Cannot serialize a valueless variant");

        const auto index = static_cast<unsigned char>(variant_index);

        std::visit(
            [&archive, index](const auto& object)
            {
                archiver_wrapper<Archiver>::push_index(archive, INDEX_NAME, index);
                serialization::save(archiver_wrapper<Archiver>::get(archive, VALUE_NAME), object);
            },
            variant);
    }

    static void load(Archiver& archive, std::variant<Types...>& variant)
    {
        constexpr size_t num_types = sizeof...(Types);
        static_assert(num_types <= 255, "Variant can contain at most 255 types");

        const auto index = archiver_wrapper<Archiver>::pop_index(archive, INDEX_NAME);

        SERIALIZATION_CHECK(
            index < num_types, "Variant index {} out of range (max {})", index, num_types - 1);

        using variant_type = std::variant<Types...>;

        // Helper to load a specific variant alternative
        auto load_alternative = [&archive, &variant]<typename AltType>()
        {
            if constexpr (std::is_default_constructible_v<AltType>)
            {
                if (!std::holds_alternative<AltType>(variant))
                {
                    variant = AltType{};
                }
                serialization::load(archiver_wrapper<Archiver>::get(archive, VALUE_NAME),
                                    std::get<AltType>(variant));
            }
            else
            {
                alignas(AltType) unsigned char storage[sizeof(AltType)];
                auto*                          ptr =
                    access::serializer::placement_new<AltType>(reinterpret_cast<void*>(storage));

                try
                {
                    serialization::load(archiver_wrapper<Archiver>::get(archive, VALUE_NAME), *ptr);
                    variant = std::move(*ptr);
                }
                catch (...)
                {
                    access::serializer::destruct(*ptr);
                    throw;
                }
                access::serializer::destruct(*ptr);
            }
        };

        // Dispatch to the correct loader based on index
        using loader_type = void (*)(decltype(load_alternative)&, variant_type&);
        static constexpr loader_type loaders[] = {
            [](decltype(load_alternative)& loader, [[maybe_unused]] variant_type& v)
            { loader.template operator()<Types>(); }...};

        loaders[index](load_alternative, variant);
    }
};

//-----------------------------------------------------------------------------
// std::pair specialization
//-----------------------------------------------------------------------------
template <typename Archiver, typename First, typename Second>
struct serializer_impl<Archiver, std::pair<First, Second>>
{
    static void load(Archiver& archive, std::pair<First, Second>& pair)
    {
        serialization::load(archiver_wrapper<Archiver>::get(archive, 0), pair.first);
        serialization::load(archiver_wrapper<Archiver>::get(archive, 1), pair.second);
    }

    static void save(Archiver& archive, const std::pair<First, Second>& pair)
    {
        serialization::save(archiver_wrapper<Archiver>::get(archive, 0), pair.first);
        serialization::save(archiver_wrapper<Archiver>::get(archive, 1), pair.second);
    }
};

//-----------------------------------------------------------------------------
// Smart pointer specializations using concepts
//-----------------------------------------------------------------------------
template <typename Archiver, UniquePointer T>
struct serializer_impl<Archiver, T>
{
    using element_type = typename T::element_type;

    static void save(Archiver& archive, const T& object)
    {
        SERIALIZATION_CHECK(object != nullptr, "Cannot serialize null unique_ptr");

        serialization::save(archive, *object);
    }

    static void load(Archiver& archive, T& object)
    {
        using mutable_element_type = std::remove_const_t<element_type>;
        auto loaded_object = serialization::access::serializer::make_ptr<mutable_element_type>();
        serialization::load(archive, *loaded_object);
        object.reset(loaded_object.release());
    }
};

template <typename Archiver, SharedPointer T>
    requires(!UniquePointer<T>)
struct serializer_impl<Archiver, T>
{
    using element_type = typename T::element_type;

    static void save(Archiver& archive, const T& object)
    {
        if (!object)
        {
            archiver_wrapper<Archiver>::push_class_name(archive, std::string(EMPTY_NAME));
            return;
        }

        if constexpr (Reflectable<element_type>)
        {
            // For reflectable types, write class name at shared_ptr level
            // save_object will also write the class name for the object itself
            const std::string& class_name = detail::polymorphic_type_name(object.get());
            archiver_wrapper<Archiver>::push_class_name(archive, class_name);

            serialization::save(archive, *object);
        }
        else
        {
            // For non-reflectable types, push class name before registry call
            const std::string& class_name = detail::polymorphic_type_name(object.get());
            archiver_wrapper<Archiver>::push_class_name(archive, class_name);

            using archiver_type = std::remove_cv_t<Archiver>;
            if (archiver_wrapper<archiver_type>::registry()->Has(class_name))
            {
                archiver_wrapper<archiver_type>::registry()->run(
                    class_name, archive, const_cast<element_type*>(object.get()), false);
            }
        }
    }

    static void load(Archiver& archive, T& object)
    {
        using archiver_type          = std::remove_cv_t<Archiver>;
        const std::string class_name = archiver_wrapper<archiver_type>::pop_class_name(archive);

        if (class_name == EMPTY_NAME)
        {
            object = nullptr;
            return;
        }

        if (archiver_wrapper<archiver_type>::registry()->Has(class_name))
        {
            archiver_wrapper<archiver_type>::registry()->run(class_name, archive, &object, true);
            return;
        }

        if constexpr (Reflectable<element_type>)
        {
            // Push the class name back since load_object will pop it again
            archiver_wrapper<archiver_type>::push_class_name(archive, class_name);

            using mutable_element_type = std::remove_const_t<element_type>;
            auto loaded_object =
                serialization::access::serializer::make_ptr<mutable_element_type>();
            serialization::load(archive, *loaded_object);
            object.reset(loaded_object.release());
        }
        else
        {
            SERIALIZATION_THROW(
                "Cannot deserialize type '{}': not registered and no reflection available",
                class_name);
        }
    }
};

//-----------------------------------------------------------------------------
// std::tuple specialization using concepts
//-----------------------------------------------------------------------------
template <typename Archiver, TupleLike T>
struct serializer_impl<Archiver, T>
{
    static void load(Archiver& archive, T& tuple)
    {
        constexpr auto tuple_size   = std::tuple_size_v<T>;
        const auto     archive_size = archiver_wrapper<Archiver>::size(archive);

        SERIALIZATION_CHECK(
            archive_size == tuple_size,
            "Tuple size mismatch: expected {} but got {}",
            tuple_size,
            archive_size);

        load_tuple_impl(archive, tuple, std::make_index_sequence<tuple_size>{});
    }

    static void save(Archiver& archive, const T& tuple)
    {
        constexpr auto tuple_size = std::tuple_size_v<T>;
        archiver_wrapper<Archiver>::resize(archive, tuple_size);
        save_tuple_impl(archive, tuple, std::make_index_sequence<tuple_size>{});
    }

private:
    template <std::size_t... Is>
    static void load_tuple_impl(Archiver& archive, T& tuple, std::index_sequence<Is...>)
    {
        (serialization::load(archiver_wrapper<Archiver>::get(archive, Is), std::get<Is>(tuple)),
         ...);
    }

    template <std::size_t... Is>
    static void save_tuple_impl(Archiver& archive, const T& tuple, std::index_sequence<Is...>)
    {
        (serialization::save(archiver_wrapper<Archiver>::get(archive, Is), std::get<Is>(tuple)),
         ...);
    }
};

//-----------------------------------------------------------------------------
// std::optional specialization
//-----------------------------------------------------------------------------
template <typename Archiver, OptionalLike T>
struct serializer_impl<Archiver, T>
{
    using value_type = typename T::value_type;

    static void save(Archiver& archive, const T& optional)
    {
        const bool has_value = optional.has_value();

        // First, save whether the optional has a value
        archiver_wrapper<Archiver>::resize(archive, 2);
        serialization::save(archiver_wrapper<Archiver>::get(archive, 0), has_value);

        // If it has a value, save it
        if (has_value)
        {
            serialization::save(archiver_wrapper<Archiver>::get(archive, 1), *optional);
        }
    }

    static void load(Archiver& archive, T& optional)
    {
        const auto archive_size = archiver_wrapper<Archiver>::size(archive);

        SERIALIZATION_CHECK(
            archive_size >= 1,
            "Invalid optional serialization: expected at least 1 element but got {}",
            archive_size);

        // Load the has_value flag
        bool has_value = false;
        serialization::load(archiver_wrapper<Archiver>::get(archive, 0), has_value);

        if (has_value)
        {
            SERIALIZATION_CHECK(
                archive_size >= 2,
                "Invalid optional serialization: has_value=true but only {} elements",
                archive_size);

            value_type loaded_value;
            serialization::load(archiver_wrapper<Archiver>::get(archive, 1), loaded_value);
            optional = std::move(loaded_value);
        }
        else
        {
            optional = std::nullopt;
        }
    }
};

}  // namespace impl

//-----------------------------------------------------------------------------
// Public API with concepts
//-----------------------------------------------------------------------------

// Lvalue reference overloads (for JSON, Binary, etc.)
template <typename Archiver, typename T>
    requires(BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void save(Archiver& archive, const T& obj)
{
    impl::serializer_impl<Archiver, T>::save(archive, obj);
}

template <typename Archiver, typename T>
    requires(BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void load(Archiver& archive, T& obj)
{
    impl::serializer_impl<Archiver, T>::load(archive, obj);
}

// Rvalue reference overloads (for XML nodes returned by value)
// Constrained to only match actual rvalues (not forwarding references)
template <typename Archiver, typename T>
    requires(!std::is_lvalue_reference_v<Archiver>) &&
            (BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void save(Archiver&& archive, const T& obj)
{
    impl::serializer_impl<Archiver, T>::save(archive, obj);
}

template <typename Archiver, typename T>
    requires(!std::is_lvalue_reference_v<Archiver>) &&
            (BaseSerializable<T> || Container<T> || Reflectable<T> || SmartPointer<T> ||
             TupleLike<T> || VariantLike<T> || OptionalLike<T>)
void load(Archiver&& archive, T& obj)
{
    impl::serializer_impl<Archiver, T>::load(archive, obj);
}

}  // namespace serialization
