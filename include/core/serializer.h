#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "archive/traits.h"
#include "codecs/native.h"
#include "codecs/type_codec.h"
#include "core/error.h"
#include "metadata/provider.h"

namespace serialization
{
struct operation_options
{
    std::size_t max_depth        = 256;
    std::size_t max_elements     = 1'000'000;
    std::size_t max_string_bytes = 16 * 1024 * 1024;
};

namespace core_detail
{
template <class...>
inline constexpr bool unsupported = false;
template <class F>
struct on_exit
{
    F f;
    ~on_exit() { f(); }
};
template <class F>
on_exit(F) -> on_exit<F>;
template <class T>
constexpr bool builtin_codec = requires { typename type_codec<T>::builtin; };
template <class T>
constexpr bool primitive =
    std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, std::string>;
}  // namespace core_detail

template <class A, class Metadata, class Base>
class polymorphic_registry;
template <class A, class Metadata = macro_metadata>
class context;

// A catalog is independent of archive and metadata provider. Binding produces
// callbacks only for the archive's supported direction.
template <class Base, class... Derived>
struct type_catalog
{
    std::array<std::string, sizeof...(Derived)> ids;
};

template <class A, class Metadata>
class context
{
public:
    using archive_type      = A;
    using metadata_provider = Metadata;

    explicit context(operation_options options = {}) : options_(options) {}
    context(const context&)            = delete;
    context& operator=(const context&) = delete;

    template <class Base>
    void bind(const polymorphic_registry<A, Metadata, Base>& registry)
    {
        if (!registry.frozen())
            throw serialization_error(
                error_code::invalid_registry, "Freeze the registry before binding");
        registries_[std::type_index(typeid(Base))] = &registry;
    }

    template <class Base>
    const polymorphic_registry<A, Metadata, Base>* registry() const
    {
        const auto it = registries_.find(std::type_index(typeid(Base)));
        return it == registries_.end()
                   ? nullptr
                   : static_cast<const polymorphic_registry<A, Metadata, Base>*>(it->second);
    }

    void check_count(std::size_t count) const
    {
        if (count > options_.max_elements)
            throw serialization_error(error_code::size_mismatch, "Sequence exceeds element limit");
    }

    template <class F>
    decltype(auto) at(std::string component, F&& f)
    {
        const auto old_size = path_.size();
        path_ += component;
        core_detail::on_exit restore{[&] { path_.resize(old_size); }};
        try
        {
            return std::forward<F>(f)();
        }
        catch (const serialization_error& e)
        {
            if (e.path().empty())
                throw serialization_error(e.code(), e.message(), path_);
            throw;
        }
    }

    template <class F>
    void with_pointer(const void* address, F&& f)
    {
        if (!active_.insert(address).second)
            throw serialization_error(error_code::cycle, "Cyclic object reference");
        core_detail::on_exit restore{[&] { active_.erase(address); }};
        std::forward<F>(f)();
    }

    template <class T>
    void save(A& archive, const T& value)
        requires OutputArchive<A>
    {
        visit([&] { save_value(archive, value); });
    }

    template <class T>
    void load(A& archive, T& value)
        requires InputArchive<A>
    {
        static_assert(!std::is_const_v<T>, "Cannot deserialize into a const value");
        visit([&] { load_value(archive, value); });
    }

    template <class T>
    void save_element(A& archive, std::size_t i, const T& value)
    {
        at("[" + std::to_string(i) + "]",
           [&]
           {
               archive_traits<A>::write_element(archive, i, [&](A& child) { save(child, value); });
           });
    }

    template <class T>
    void load_element(A& archive, std::size_t i, T& value)
    {
        at("[" + std::to_string(i) + "]",
           [&]
           { archive_traits<A>::read_element(archive, i, [&](A& child) { load(child, value); }); });
    }

private:
    template <class F>
    void visit(F&& f)
    {
        at({},
           [&]
           {
               if (depth_ >= options_.max_depth)
                   throw serialization_error(
                       error_code::depth_limit, "Maximum serialization depth exceeded");
               ++depth_;
               core_detail::on_exit restore{[&] { --depth_; }};
               std::forward<F>(f)();
           });
    }

    template <class T>
    static constexpr std::size_t field_count()
    {
        std::size_t count = 0;
        std::apply(
            [&](const auto&... fields)
            { ((count += std::remove_cvref_t<decltype(fields)>::has_member ? 1 : 0), ...); },
            Metadata::template properties<T>());
        return count;
    }

    template <class T>
    void save_record(A& a, const T& value)
    {
        object_header header{
            std::string(record_info<T>::type_id), record_info<T>::version, field_count<T>()};
        archive_traits<A>::write_object(
            a,
            header,
            [&](A& object)
            {
                std::size_t position = 0;
                std::apply(
                    [&](const auto&... fields)
                    {
                        auto field = [&](const auto& f)
                        {
                            if constexpr (std::remove_cvref_t<decltype(f)>::has_member)
                            {
                                const field_key key{f.name(), position++};
                                at("." + std::string(key.name),
                                   [&]
                                   {
                                       archive_traits<A>::write_field(
                                           object,
                                           key,
                                           [&](A& child) { save(child, value.*f.member()); });
                                   });
                            }
                        };
                        (field(fields), ...);
                    },
                    Metadata::template properties<T>());
            });
    }

    template <class T>
    void load_record(A& a, T& value)
    {
        object_header expected{
            std::string(record_info<T>::type_id), record_info<T>::version, field_count<T>()};
        archive_traits<A>::read_object(
            a,
            expected,
            [&](A& object, const object_header& actual)
            {
                if (actual.version != expected.version)
                    throw serialization_error(
                        error_code::unsupported_version, "Record version mismatch");
                if (!expected.type_id.empty() && actual.type_id != expected.type_id)
                    throw serialization_error(error_code::unknown_type, "Record type mismatch");
                std::size_t position = 0;
                std::apply(
                    [&](const auto&... fields)
                    {
                        auto field = [&](const auto& f)
                        {
                            if constexpr (std::remove_cvref_t<decltype(f)>::has_member)
                            {
                                const field_key key{f.name(), position++};
                                at("." + std::string(key.name),
                                   [&]
                                   {
                                       const auto presence = archive_traits<A>::read_field(
                                           object,
                                           key,
                                           [&](A& child) { load(child, value.*f.member()); });
                                       if (presence == field_presence::missing)
                                           throw serialization_error(
                                               error_code::missing_field,
                                               "Required field is missing");
                                   });
                            }
                        };
                        (field(fields), ...);
                    },
                    Metadata::template properties<T>());
                access::serializer::initialize(value);
            });
    }

    template <class T>
    void save_value(A& a, const T& value)
    {
        using V              = std::remove_cv_t<T>;
        constexpr bool codec = requires { type_codec<V>::save(a, value, *this); };
        static_assert(
            !(codec && !core_detail::builtin_codec<V> &&
              (is_native_serializable_v<V> || HasMetadata<Metadata, V>)),
            "type_codec conflicts with native or selected-member metadata");
        if constexpr (is_native_serializable_v<V>)
        {
            using wire = typename native_serializable<V>::wire_type;
            static_assert(
                core_detail::primitive<wire> && !is_native_serializable_v<wire>,
                "Native wire_type must be a library primitive");
            save(a, detail::native_to_wire(value));
        }
        else if constexpr (HasMetadata<Metadata, V>)
            save_record(a, value);
        else if constexpr (std::is_enum_v<V>)
            save(a, static_cast<std::underlying_type_t<V>>(value));
        else if constexpr (std::is_arithmetic_v<V> || std::is_same_v<V, std::string>)
        {
            if constexpr (std::is_same_v<V, std::string>)
                if (value.size() > options_.max_string_bytes)
                    throw serialization_error(
                        error_code::size_mismatch, "String exceeds byte limit");
            static_assert(ScalarOutput<A, V>, "Archive does not support this scalar output type");
            archive_traits<A>::write_scalar(a, value);
        }
        else if constexpr (std::is_convertible_v<const T&, std::string_view>)
            save(a, std::string(std::string_view(value)));
        else if constexpr (codec)
            type_codec<V>::save(a, value, *this);
        else
            static_assert(
                core_detail::unsupported<T>, "No serialization metadata or type_codec for type");
    }

    template <class T>
    void load_value(A& a, T& value)
    {
        using V              = std::remove_cv_t<T>;
        constexpr bool codec = requires { type_codec<V>::load(a, value, *this); };
        static_assert(
            !(codec && !core_detail::builtin_codec<V> &&
              (is_native_serializable_v<V> || HasMetadata<Metadata, V>)),
            "type_codec conflicts with native or selected-member metadata");
        if constexpr (is_native_serializable_v<V>)
        {
            using wire = typename native_serializable<V>::wire_type;
            static_assert(core_detail::primitive<wire> && !is_native_serializable_v<wire>);
            detail::load_native_value(value, [&](auto& temporary) { load(a, temporary); });
        }
        else if constexpr (HasMetadata<Metadata, V>)
            load_record(a, value);
        else if constexpr (std::is_enum_v<V>)
        {
            std::underlying_type_t<V> temporary{};
            load(a, temporary);
            value = static_cast<V>(temporary);
        }
        else if constexpr (std::is_arithmetic_v<V> || std::is_same_v<V, std::string>)
        {
            static_assert(ScalarInput<A, V>, "Archive does not support this scalar input type");
            archive_traits<A>::read_scalar(a, value);
            if constexpr (std::is_same_v<V, std::string>)
                if (value.size() > options_.max_string_bytes)
                    throw serialization_error(
                        error_code::size_mismatch, "String exceeds byte limit");
        }
        else if constexpr (codec)
            type_codec<V>::load(a, value, *this);
        else
            static_assert(
                core_detail::unsupported<T>, "No deserialization metadata or type_codec for type");
    }

    operation_options                                options_;
    std::size_t                                      depth_ = 0;
    std::string                                      path_  = "$";
    std::unordered_set<const void*>                  active_;
    std::unordered_map<std::type_index, const void*> registries_;
};

template <class MetadataProvider = macro_metadata>
class basic_serializer
{
public:
    explicit basic_serializer(operation_options options = {}) : options_(options) {}
    template <OutputArchive A, class T>
    void save(A& a, const T& value) const
    {
        context<A, MetadataProvider> ctx(options_);
        ctx.save(a, value);
    }
    template <InputArchive A, class T>
    void load(A& a, T& value) const
    {
        context<A, MetadataProvider> ctx(options_);
        ctx.load(a, value);
    }
    template <OutputArchive A, class T>
    void save(A& a, const T& value, context<A, MetadataProvider>& ctx) const
    {
        ctx.save(a, value);
    }
    template <InputArchive A, class T>
    void load(A& a, T& value, context<A, MetadataProvider>& ctx) const
    {
        ctx.load(a, value);
    }

private:
    operation_options options_;
};
using serializer = basic_serializer<>;

template <class A, class Metadata, class Base>
class polymorphic_registry
{
public:
    struct entry
    {
        std::string                                                     id;
        std::function<void(A&, const Base&, context<A, Metadata>&)>     save;
        std::function<std::unique_ptr<Base>(A&, context<A, Metadata>&)> load;
    };
    polymorphic_registry() = default;
    template <class... Derived>
    explicit polymorphic_registry(const type_catalog<Base, Derived...>& catalog)
    {
        std::size_t i = 0;
        (add<Derived>(catalog.ids[i++]), ...);
        freeze();
    }

    template <class Derived>
    void add(std::string id)
    {
        static_assert(
            std::derived_from<Derived, Base> && std::has_virtual_destructor_v<Base>,
            "Polymorphic registration requires public inheritance and a virtual base destructor");
        if (frozen_)
            throw serialization_error(error_code::invalid_registry, "Registry is frozen");
        const auto type = std::type_index(typeid(Derived));
        if (id.empty() || by_id_.contains(id) || by_type_.contains(type))
            throw serialization_error(
                error_code::duplicate_type, "Empty or duplicate polymorphic type ID/type");
        entry e;
        e.id = id;
        if constexpr (OutputArchive<A>)
            e.save = [](A& a, const Base& value, context<A, Metadata>& ctx)
            { ctx.save(a, dynamic_cast<const Derived&>(value)); };
        if constexpr (InputArchive<A>)
            e.load = [](A& a, context<A, Metadata>& ctx) -> std::unique_ptr<Base>
            {
                auto value = access::serializer::make_ptr<Derived>();
                ctx.load(a, *value);
                return value;
            };
        by_id_.emplace(id, std::move(e));
        by_type_.emplace(type, std::move(id));
    }
    void         freeze() noexcept { frozen_ = true; }
    bool         frozen() const noexcept { return frozen_; }
    const entry* find(std::string_view id) const
    {
        auto i = by_id_.find(std::string(id));
        return i == by_id_.end() ? nullptr : &i->second;
    }
    const entry* find(std::type_index type) const
    {
        auto i = by_type_.find(type);
        return i == by_type_.end() ? nullptr : find(i->second);
    }

private:
    bool                                             frozen_ = false;
    std::unordered_map<std::string, entry>           by_id_;
    std::unordered_map<std::type_index, std::string> by_type_;
};
}  // namespace serialization
