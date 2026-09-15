#pragma once

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <pugixml.hpp>

#include "archive/numeric.h"
#include "archive/traits.h"

namespace serialization::adapters
{
struct xml_writer { pugi::xml_node node; };
struct xml_reader { pugi::xml_node node; };
}
namespace serialization::adapter_detail
{
inline void require_xml(bool condition, const char* message)
{
    if (!condition) throw serialization_error(error_code::invalid_value, message);
}
inline void xml_kind(pugi::xml_node node, const char* kind)
{
    require_xml(std::string_view(node.attribute("kind").value()) == kind, "Unexpected XML value kind");
}
inline void start_xml(pugi::xml_node node, const char* kind)
{
    node.remove_children(); node.remove_attributes(); node.append_attribute("kind").set_value(kind);
}
inline std::string_view xml_attribute(pugi::xml_node node, const char* name)
{
    const auto attr = node.attribute(name);
    if (!attr) throw serialization_error(error_code::missing_field, std::string("Missing XML attribute: ") + name);
    return attr.value();
}
template<class T> T xml_number(std::string_view text)
{
    T value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    require_xml(result.ec == std::errc{} && result.ptr == text.data() + text.size(), "Invalid XML number");
    return value;
}
template<class T> void xml_write_number(pugi::xml_node node, T value)
{
    char text[128];
    const auto result = std::to_chars(text, text + sizeof(text), value);
    require_xml(result.ec == std::errc{}, "Cannot encode XML number");
    node.text().set(std::string(text, result.ptr).c_str());
}
}
namespace serialization
{
template<> struct archive_traits<adapters::xml_writer>
{
    using A = adapters::xml_writer;
    template<class T> requires (std::is_arithmetic_v<T> && sizeof(T) <= 8)
    static void write_scalar(A& a, T value)
    {
        if constexpr (std::is_same_v<T, bool>)
        { adapter_detail::start_xml(a.node, "bool"); a.node.text().set(value ? "true" : "false"); }
        else if constexpr (std::is_floating_point_v<T>)
        { adapter_detail::start_xml(a.node, "float"); adapter_detail::xml_write_number(a.node, adapter_detail::checked_float<double>(value)); }
        else
        { adapter_detail::start_xml(a.node, std::is_signed_v<T> ? "integer" : "unsigned"); adapter_detail::xml_write_number(a.node, value); }
    }
    static void write_scalar(A& a, const std::string& value)
    {
        adapter_detail::require_xml(value.find('\0') == std::string::npos, "XML cannot represent embedded NUL characters");
        adapter_detail::start_xml(a.node, "string"); a.node.text().set(value.c_str());
    }
    template<class F> static void write_object(A& a, const object_header& h, F&& body)
    {
        adapter_detail::start_xml(a.node, "object");
        a.node.append_attribute("type").set_value(h.type_id.c_str());
        a.node.append_attribute("version").set_value(h.version); body(a);
    }
    template<class F> static void write_field(A& a, field_key key, F&& body)
    {
        A child{a.node.append_child("field")}; body(child);
        child.node.append_attribute("name").set_value(std::string(key.name).c_str());
    }
    template<class F> static void write_sequence(A& a, std::size_t count, F&& body)
    {
        adapter_detail::start_xml(a.node, "sequence");
        a.node.append_attribute("count").set_value(static_cast<unsigned long long>(count)); body(a);
    }
    template<class F> static void write_element(A& a, std::size_t, F&& body)
    { A child{a.node.append_child("item")}; body(child); }
    template<class F> static void write_tagged(A& a, const tagged_header& h, F&& body)
    {
        adapter_detail::start_xml(a.node, "tagged");
        a.node.append_attribute("tag").set_value(static_cast<unsigned>(h.kind));
        a.node.append_attribute("present").set_value(h.present ? "true" : "false");
        a.node.append_attribute("index").set_value(static_cast<unsigned long long>(h.index));
        a.node.append_attribute("type").set_value(h.type_id.c_str());
        if (h.present) { A child{a.node.append_child("value")}; body(child); }
    }
};
template<> struct archive_traits<adapters::xml_reader>
{
    using A = adapters::xml_reader;
    template<class T> requires (std::is_arithmetic_v<T> && sizeof(T) <= 8)
    static void read_scalar(A& a, T& value)
    {
        const auto kind = adapter_detail::xml_attribute(a.node, "kind");
        const std::string_view text = a.node.text().get();
        if constexpr (std::is_same_v<T, bool>)
        {
            adapter_detail::require_xml(kind == "bool" && (text == "true" || text == "false"), "Invalid XML boolean");
            value = text == "true";
        }
        else if constexpr (std::is_integral_v<T>)
        {
            adapter_detail::require_xml(kind == "integer" || kind == "unsigned", "Expected XML integer");
            if (kind == "integer") value = adapter_detail::checked_integer<T>(adapter_detail::xml_number<std::int64_t>(text));
            else value = adapter_detail::checked_integer<T>(adapter_detail::xml_number<std::uint64_t>(text));
        }
        else
        {
            adapter_detail::xml_kind(a.node, "float");
            value = adapter_detail::checked_float<T>(adapter_detail::xml_number<double>(text));
        }
    }
    static void read_scalar(A& a, std::string& value)
    { adapter_detail::xml_kind(a.node, "string"); value = a.node.text().get(); }
    template<class F> static void read_object(A& a, const object_header&, F&& body)
    {
        adapter_detail::xml_kind(a.node, "object");
        object_header h;
        h.type_id = adapter_detail::xml_attribute(a.node, "type");
        h.version = adapter_detail::xml_number<std::uint32_t>(adapter_detail::xml_attribute(a.node, "version"));
        for (auto field : a.node.children("field")) { (void)field; ++h.field_count; }
        body(a, h);
    }
    template<class F> static field_presence read_field(A& a, field_key key, F&& body)
    {
        pugi::xml_node found;
        for (auto field : a.node.children("field"))
            if (std::string_view(field.attribute("name").value()) == key.name)
            {
                adapter_detail::require_xml(!found, "Duplicate XML field"); found = field;
            }
        if (!found) return field_presence::missing;
        A child{found}; body(child); return field_presence::present;
    }
    template<class F> static void read_sequence(A& a, F&& body)
    {
        adapter_detail::xml_kind(a.node, "sequence");
        const auto count = adapter_detail::xml_number<std::size_t>(adapter_detail::xml_attribute(a.node, "count"));
        std::size_t actual = 0;
        for (auto item : a.node.children("item")) { (void)item; ++actual; }
        if (actual != count) throw serialization_error(error_code::size_mismatch, "XML sequence size mismatch");
        body(a, count);
    }
    template<class F> static void read_element(A& a, std::size_t index, F&& body)
    {
        auto item = a.node.child("item");
        while (index-- && item) item = item.next_sibling("item");
        adapter_detail::require_xml(static_cast<bool>(item), "XML sequence index out of range");
        A child{item}; body(child);
    }
    template<class F> static void read_tagged(A& a, tag_kind expected, F&& body)
    {
        adapter_detail::xml_kind(a.node, "tagged");
        const auto kind = adapter_detail::xml_number<unsigned>(adapter_detail::xml_attribute(a.node, "tag"));
        const auto present = adapter_detail::xml_attribute(a.node, "present");
        adapter_detail::require_xml(kind == static_cast<unsigned>(expected) && (present == "true" || present == "false"), "Invalid XML tag");
        tagged_header h{expected, present == "true",
            adapter_detail::xml_number<std::size_t>(adapter_detail::xml_attribute(a.node, "index")),
            std::string(adapter_detail::xml_attribute(a.node, "type"))};
        if (h.present)
        {
            auto node = a.node.child("value");
            adapter_detail::require_xml(static_cast<bool>(node), "Missing XML tag payload");
            A child{node}; body(child, h);
        }
        else body(a, h);
    }
};
}
