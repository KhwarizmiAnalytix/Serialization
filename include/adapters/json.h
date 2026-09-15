#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <nlohmann/json.hpp>

#include "archive/numeric.h"
#include "archive/traits.h"

namespace serialization::adapters
{
using json = nlohmann::ordered_json;
struct json_writer { json& node; };
struct json_reader { const json& node; };
}

namespace serialization::adapter_detail
{
inline void require_json(bool condition, const char* message)
{
    if (!condition) throw serialization_error(error_code::invalid_value, message);
}
inline const adapters::json& json_member(const adapters::json& node, const char* key)
{
    require_json(node.is_object(), "Expected JSON object");
    auto it = node.find(key);
    if (it == node.end()) throw serialization_error(error_code::missing_field, std::string("Missing JSON member: ") + key);
    return *it;
}
template<class T> void json_scalar(const adapters::json& node, T& value)
{
    if constexpr (std::is_same_v<T, bool>)
    {
        require_json(node.is_boolean(), "Expected JSON boolean");
        value = node.get<bool>();
    }
    else if constexpr (std::is_integral_v<T>)
    {
        require_json(node.is_number_integer(), "Expected JSON integer");
        if (node.is_number_unsigned()) value = checked_integer<T>(node.get<std::uint64_t>());
        else value = checked_integer<T>(node.get<std::int64_t>());
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        require_json(node.is_number(), "Expected JSON number");
        value = checked_float<T>(node.get<double>());
    }
    else
    {
        static_assert(std::is_same_v<T, std::string>);
        require_json(node.is_string(), "Expected JSON string");
        value = node.get<std::string>();
    }
}
}

namespace serialization
{
template<> struct archive_traits<adapters::json_writer>
{
    using A = adapters::json_writer;
    using json = adapters::json;
    template<class T> requires (std::is_arithmetic_v<T> && sizeof(T) <= 8)
    static void write_scalar(A& a, T value)
    {
        if constexpr (std::is_floating_point_v<T>) a.node = adapter_detail::checked_float<double>(value);
        else a.node = value;
    }
    static void write_scalar(A& a, const std::string& value) { a.node = value; }
    template<class F> static void write_object(A& a, const object_header& h, F&& body)
    {
        a.node = json{{"$type", h.type_id}, {"$version", h.version}, {"$fields", json::object()}};
        A fields{a.node["$fields"]}; body(fields);
    }
    template<class F> static void write_field(A& a, field_key key, F&& body)
    {
        A child{a.node[std::string(key.name)]}; body(child);
    }
    template<class F> static void write_sequence(A& a, std::size_t, F&& body)
    {
        a.node = json::array(); body(a);
    }
    template<class F> static void write_element(A& a, std::size_t, F&& body)
    {
        a.node.push_back(nullptr); A child{a.node.back()}; body(child);
    }
    template<class F> static void write_tagged(A& a, const tagged_header& h, F&& body)
    {
        a.node = json{{"$kind", static_cast<unsigned>(h.kind)}, {"$present", h.present},
                      {"$index", h.index}, {"$type", h.type_id}};
        if (h.present) { A child{a.node["$value"]}; body(child); }
    }
};

template<> struct archive_traits<adapters::json_reader>
{
    using A = adapters::json_reader;
    template<class T> requires ((std::is_arithmetic_v<T> && sizeof(T) <= 8) || std::is_same_v<T, std::string>)
    static void read_scalar(A& a, T& value) { adapter_detail::json_scalar(a.node, value); }
    template<class F> static void read_object(A& a, const object_header&, F&& body)
    {
        object_header h;
        adapter_detail::json_scalar(adapter_detail::json_member(a.node, "$type"), h.type_id);
        adapter_detail::json_scalar(adapter_detail::json_member(a.node, "$version"), h.version);
        const auto& fields = adapter_detail::json_member(a.node, "$fields");
        adapter_detail::require_json(fields.is_object(), "Expected JSON fields object");
        h.field_count = fields.size(); A scope{fields}; body(scope, h);
    }
    template<class F> static field_presence read_field(A& a, field_key key, F&& body)
    {
        auto it = a.node.find(std::string(key.name));
        if (it == a.node.end()) return field_presence::missing;
        A child{*it}; body(child); return field_presence::present;
    }
    template<class F> static void read_sequence(A& a, F&& body)
    {
        adapter_detail::require_json(a.node.is_array(), "Expected JSON sequence");
        body(a, a.node.size());
    }
    template<class F> static void read_element(A& a, std::size_t index, F&& body)
    {
        adapter_detail::require_json(index < a.node.size(), "JSON sequence index out of range");
        A child{a.node[index]}; body(child);
    }
    template<class F> static void read_tagged(A& a, tag_kind expected, F&& body)
    {
        unsigned kind;
        tagged_header h;
        adapter_detail::json_scalar(adapter_detail::json_member(a.node, "$kind"), kind);
        adapter_detail::require_json(kind == static_cast<unsigned>(expected), "Unexpected JSON tag kind");
        h.kind = expected;
        adapter_detail::json_scalar(adapter_detail::json_member(a.node, "$present"), h.present);
        adapter_detail::json_scalar(adapter_detail::json_member(a.node, "$index"), h.index);
        adapter_detail::json_scalar(adapter_detail::json_member(a.node, "$type"), h.type_id);
        if (h.present) { A child{adapter_detail::json_member(a.node, "$value")}; body(child, h); }
        else body(a, h);
    }
};
}
