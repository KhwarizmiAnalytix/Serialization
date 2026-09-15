#pragma once

#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <variant>

#include "core/serializer.h"

namespace serialization
{
namespace core_detail
{
template <class T>
concept tuple_value = requires { typename std::tuple_size<T>::type; };
template <class T>
concept collection =
    std::ranges::range<T> && !tuple_value<T> && requires { typename T::value_type; };
template <class T>
concept map_value = requires {
    typename T::key_type;
    typename T::mapped_type;
};
}  // namespace core_detail

template <class T>
struct type_codec<T, std::enable_if_t<core_detail::tuple_value<T>>>
{
    using builtin = void;
    template <class A, class C>
    static void save(A& a, const T& value, C& ctx)
    {
        constexpr auto count = std::tuple_size_v<T>;
        ctx.check_count(count);
        archive_traits<A>::write_sequence(
            a,
            count,
            [&](A& sequence)
            {
                [&]<std::size_t... I>(std::index_sequence<I...>)
                {
                    (ctx.save_element(sequence, I, std::get<I>(value)), ...);
                }(std::make_index_sequence<count>{});
            });
    }
    template <class A, class C>
    static void load(A& a, T& value, C& ctx)
    {
        constexpr auto count = std::tuple_size_v<T>;
        archive_traits<A>::read_sequence(
            a,
            [&](A& sequence, std::size_t actual)
            {
                ctx.check_count(actual);
                if (actual != count)
                    throw serialization_error(
                        error_code::size_mismatch, "Tuple/array size mismatch");
                [&]<std::size_t... I>(std::index_sequence<I...>)
                {
                    (ctx.load_element(sequence, I, std::get<I>(value)), ...);
                }(std::make_index_sequence<count>{});
            });
    }
};

template <class T>
struct type_codec<T, std::enable_if_t<core_detail::collection<T>>>
{
    using builtin = void;
    template <class A, class C>
    static void save(A& a, const T& value, C& ctx)
    {
        const auto count = static_cast<std::size_t>(std::ranges::distance(value));
        ctx.check_count(count);
        archive_traits<A>::write_sequence(
            a,
            count,
            [&](A& sequence)
            {
                std::size_t i = 0;
                for (const auto& item : value)
                {
                    if constexpr (std::is_same_v<typename T::value_type, bool>)
                        ctx.save_element(sequence, i++, static_cast<bool>(item));
                    else
                        ctx.save_element(sequence, i++, item);
                }
            });
    }
    template <class A, class C>
    static void load(A& a, T& value, C& ctx)
    {
        archive_traits<A>::read_sequence(
            a,
            [&](A& sequence, std::size_t count)
            {
                ctx.check_count(count);
                value.clear();
                if constexpr (requires { value.reserve(count); })
                    value.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    if constexpr (core_detail::map_value<T>)
                    {
                        std::pair<typename T::key_type, typename T::mapped_type> item;
                        ctx.load_element(sequence, i, item);
                        value.emplace(std::move(item.first), std::move(item.second));
                    }
                    else
                    {
                        auto item = access::serializer::make_ptr<typename T::value_type>();
                        ctx.load_element(sequence, i, *item);
                        if constexpr (requires { value.emplace_back(std::move(*item)); })
                            value.emplace_back(std::move(*item));
                        else if constexpr (requires {
                                               value.insert(value.end(), std::move(*item));
                                           })
                            value.insert(value.end(), std::move(*item));
                        else
                            static_assert(
                                core_detail::unsupported<T>,
                                "Container requires an insertion codec");
                    }
                }
            });
    }
};

template <class T>
struct type_codec<std::optional<T>>
{
    using builtin = void;
    template <class A, class C>
    static void save(A& a, const std::optional<T>& value, C& ctx)
    {
        archive_traits<A>::write_tagged(
            a,
            {tag_kind::nullable, value.has_value(), 0, {}},
            [&](A& child) { ctx.save(child, *value); });
    }
    template <class A, class C>
    static void load(A& a, std::optional<T>& value, C& ctx)
    {
        archive_traits<A>::read_tagged(
            a,
            tag_kind::nullable,
            [&](A& child, const tagged_header& header)
            {
                if (!header.present)
                {
                    value.reset();
                    return;
                }
                auto loaded = access::serializer::make_ptr<T>();
                ctx.load(child, *loaded);
                value.emplace(std::move(*loaded));
            });
    }
};

template <>
struct type_codec<std::monostate>
{
    using builtin = void;
    template <class A, class C>
    static void save(A& a, const std::monostate&, C&)
    {
        archive_traits<A>::write_tagged(a, {tag_kind::nullable, false, 0, {}}, [](A&) {});
    }
    template <class A, class C>
    static void load(A& a, std::monostate&, C&)
    {
        archive_traits<A>::read_tagged(
            a,
            tag_kind::nullable,
            [](A&, const tagged_header& h)
            {
                if (h.present)
                    throw serialization_error(
                        error_code::invalid_value, "Expected empty monostate");
            });
    }
};

template <class... T>
struct type_codec<std::variant<T...>>
{
    using builtin = void;
    using variant = std::variant<T...>;
    template <class A, class C>
    static void save(A& a, const variant& value, C& ctx)
    {
        if (value.valueless_by_exception())
            throw serialization_error(error_code::invalid_value, "Valueless variant");
        archive_traits<A>::write_tagged(
            a,
            {tag_kind::variant, true, value.index(), {}},
            [&](A& child) { std::visit([&](const auto& item) { ctx.save(child, item); }, value); });
    }
    template <class A, class C>
    static void load(A& a, variant& value, C& ctx)
    {
        archive_traits<A>::read_tagged(
            a,
            tag_kind::variant,
            [&](A& child, const tagged_header& h)
            {
                if (!h.present || h.index >= sizeof...(T))
                    throw serialization_error(
                        error_code::invalid_value, "Invalid variant discriminator");
                load_alternative<0>(child, h.index, value, ctx);
            });
    }

private:
    template <std::size_t I, class A, class C>
    static void load_alternative(A& a, std::size_t index, variant& value, C& ctx)
    {
        if constexpr (I < sizeof...(T))
        {
            if (index == I)
            {
                auto item = access::serializer::make_ptr<std::variant_alternative_t<I, variant>>();
                ctx.load(a, *item);
                value.template emplace<I>(std::move(*item));
            }
            else
                load_alternative<I + 1>(a, index, value, ctx);
        }
    }
};

namespace core_detail
{
template <class P>
struct pointer_codec
{
    using builtin    = void;
    using value_type = std::remove_const_t<typename P::element_type>;
    static constexpr tag_kind kind =
        std::is_polymorphic_v<value_type> ? tag_kind::polymorphic : tag_kind::nullable;

    template <class A, class C>
    static void save(A& a, const P& value, C& ctx)
    {
        tagged_header header{kind, static_cast<bool>(value), 0, {}};
        if (!value)
        {
            archive_traits<A>::write_tagged(a, header, [](A&) {});
            return;
        }
        const void* address = value.get();
        if constexpr (std::is_polymorphic_v<value_type>)
            address = dynamic_cast<const void*>(value.get());
        ctx.with_pointer(
            address,
            [&]
            {
                if constexpr (std::is_polymorphic_v<value_type>)
                {
                    const auto* registry = ctx.template registry<value_type>();
                    const auto* entry =
                        registry ? registry->find(std::type_index(typeid(*value))) : nullptr;
                    if (entry)
                    {
                        header.type_id = entry->id;
                        archive_traits<A>::write_tagged(
                            a, header, [&](A& child) { entry->save(child, *value, ctx); });
                        return;
                    }
                    if (typeid(*value) != typeid(value_type))
                        throw serialization_error(
                            error_code::unknown_type, "Dynamic type is not registered");
                }
                if constexpr (!std::is_abstract_v<value_type>)
                    archive_traits<A>::write_tagged(
                        a, header, [&](A& child) { ctx.save(child, *value); });
                else
                    throw serialization_error(
                        error_code::unknown_type, "Abstract type requires registration");
            });
    }

    template <class A, class C>
    static void load(A& a, P& value, C& ctx)
    {
        archive_traits<A>::read_tagged(
            a,
            kind,
            [&](A& child, const tagged_header& h)
            {
                if (!h.present)
                {
                    value.reset();
                    return;
                }
                if constexpr (std::is_polymorphic_v<value_type>)
                {
                    if (!h.type_id.empty())
                    {
                        const auto* registry = ctx.template registry<value_type>();
                        const auto* entry    = registry ? registry->find(h.type_id) : nullptr;
                        if (!entry)
                            throw serialization_error(
                                error_code::unknown_type,
                                "Unknown polymorphic type ID: " + h.type_id);
                        auto loaded = entry->load(child, ctx);
                        value.reset(loaded.release());
                        return;
                    }
                }
                if constexpr (!std::is_abstract_v<value_type>)
                {
                    auto loaded = access::serializer::make_ptr<value_type>();
                    ctx.load(child, *loaded);
                    value.reset(loaded.release());
                }
                else
                    throw serialization_error(
                        error_code::unknown_type, "Abstract type requires registration");
            });
    }
};
}  // namespace core_detail

template <class T>
struct type_codec<std::shared_ptr<T>> : core_detail::pointer_codec<std::shared_ptr<T>>
{
};
template <class T>
struct type_codec<std::unique_ptr<T>> : core_detail::pointer_codec<std::unique_ptr<T>>
{
};
}  // namespace serialization
