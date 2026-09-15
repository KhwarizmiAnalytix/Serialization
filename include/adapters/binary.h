#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "archive/numeric.h"
#include "archive/traits.h"

namespace serialization::adapters
{
// Structured binary protocol v1, identified by the SRL1 header.
class binary_writer
{
public:
    explicit binary_writer(std::vector<std::byte>& bytes) : bytes_(&bytes)
    {
        for (unsigned char c : {'S', 'R', 'L', '1'})
            put(c);
    }
    void put(std::uint8_t value) { bytes_->push_back(static_cast<std::byte>(value)); }
    void word(std::uint64_t value)
    {
        for (unsigned i = 0; i < 8; ++i)
            put(static_cast<std::uint8_t>(value >> (8 * i)));
    }
    void text(std::string_view value)
    {
        word(value.size());
        for (unsigned char c : value)
            put(c);
    }

private:
    std::vector<std::byte>* bytes_;
};

class binary_reader
{
public:
    explicit binary_reader(
        std::span<const std::byte> bytes, std::size_t max_string_bytes = 16 * 1024 * 1024)
        : bytes_(bytes), max_string_bytes_(max_string_bytes)
    {
        for (unsigned char c : {'S', 'R', 'L', '1'})
            if (get() != c)
                throw serialization_error(
                    error_code::unsupported_version, "Expected structured binary SRL1 header");
    }
    std::uint8_t get()
    {
        if (position_ == bytes_.size())
            throw serialization_error(
                error_code::truncated_input, "Unexpected end of binary input");
        return std::to_integer<std::uint8_t>(bytes_[position_++]);
    }
    void expect(std::uint8_t kind)
    {
        if (get() != kind)
            throw serialization_error(error_code::invalid_value, "Unexpected binary value kind");
    }
    std::uint64_t word()
    {
        std::uint64_t value = 0;
        for (unsigned i = 0; i < 8; ++i)
            value |= std::uint64_t(get()) << (i * 8);
        return value;
    }
    std::size_t count() { return adapter_detail::checked_integer<std::size_t>(word()); }
    std::string text()
    {
        const auto size = count();
        if (size > bytes_.size() - position_)
            throw serialization_error(error_code::truncated_input, "Truncated binary string");
        if (size > max_string_bytes_)
            throw serialization_error(
                error_code::size_mismatch, "Binary string exceeds byte limit");
        std::string value(size, '\0');
        for (auto& c : value)
            c = static_cast<char>(get());
        return value;
    }
    bool empty() const noexcept { return position_ == bytes_.size(); }

private:
    std::span<const std::byte> bytes_;
    std::size_t                max_string_bytes_;
    std::size_t                position_ = 0;
};
}  // namespace serialization::adapters

namespace serialization
{
template <>
struct archive_traits<adapters::binary_writer>
{
    using A = adapters::binary_writer;
    template <class T>
        requires(std::is_integral_v<T> && sizeof(T) <= 8)
    static void write_scalar(A& a, T value)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            a.put(1);
            a.put(value ? 1 : 0);
        }
        else if constexpr (std::is_signed_v<T>)
        {
            a.put(2);
            a.word(std::bit_cast<std::uint64_t>(std::int64_t(value)));
        }
        else
        {
            a.put(3);
            a.word(std::uint64_t(value));
        }
    }
    template <class T>
        requires(std::is_floating_point_v<T> && sizeof(T) <= sizeof(double))
    static void write_scalar(A& a, T value)
    {
        a.put(4);
        a.word(std::bit_cast<std::uint64_t>(adapter_detail::checked_float<double>(value)));
    }
    static void write_scalar(A& a, const std::string& value)
    {
        a.put(5);
        a.text(value);
    }
    template <class F>
    static void write_object(A& a, const object_header& h, F&& body)
    {
        a.put(16);
        a.text(h.type_id);
        a.word(h.version);
        a.word(h.field_count);
        body(a);
    }
    template <class F>
    static void write_field(A& a, field_key, F&& body)
    {
        body(a);
    }
    template <class F>
    static void write_sequence(A& a, std::size_t count, F&& body)
    {
        a.put(17);
        a.word(count);
        body(a);
    }
    template <class F>
    static void write_element(A& a, std::size_t, F&& body)
    {
        body(a);
    }
    template <class F>
    static void write_tagged(A& a, const tagged_header& h, F&& body)
    {
        a.put(18);
        a.put(static_cast<std::uint8_t>(h.kind));
        a.put(h.present ? 1 : 0);
        a.word(h.index);
        a.text(h.type_id);
        if (h.present)
            body(a);
    }
};

template <>
struct archive_traits<adapters::binary_reader>
{
    using A = adapters::binary_reader;
    template <class T>
        requires(std::is_integral_v<T> && sizeof(T) <= 8)
    static void read_scalar(A& a, T& value)
    {
        const auto kind = a.get();
        if constexpr (std::is_same_v<T, bool>)
        {
            if (kind != 1)
                throw serialization_error(error_code::invalid_value, "Expected binary boolean");
            const auto v = a.get();
            if (v > 1)
                throw serialization_error(error_code::invalid_value, "Invalid binary boolean");
            value = v != 0;
        }
        else
        {
            if (kind == 2)
                value = adapter_detail::checked_integer<T>(std::bit_cast<std::int64_t>(a.word()));
            else if (kind == 3)
                value = adapter_detail::checked_integer<T>(a.word());
            else
                throw serialization_error(error_code::invalid_value, "Expected binary integer");
        }
    }
    template <class T>
        requires(std::is_floating_point_v<T> && sizeof(T) <= sizeof(double))
    static void read_scalar(A& a, T& value)
    {
        a.expect(4);
        value = adapter_detail::checked_float<T>(std::bit_cast<double>(a.word()));
    }
    static void read_scalar(A& a, std::string& value)
    {
        a.expect(5);
        value = a.text();
    }
    template <class F>
    static void read_object(A& a, const object_header& expected, F&& body)
    {
        a.expect(16);
        object_header h;
        h.type_id     = a.text();
        h.version     = adapter_detail::checked_integer<std::uint32_t>(a.word());
        h.field_count = a.count();
        if (h.field_count != expected.field_count)
            throw serialization_error(
                error_code::size_mismatch, "Binary record field count mismatch");
        body(a, h);
    }
    template <class F>
    static field_presence read_field(A& a, field_key, F&& body)
    {
        body(a);
        return field_presence::present;
    }
    template <class F>
    static void read_sequence(A& a, F&& body)
    {
        a.expect(17);
        const auto count = a.count();
        body(a, count);
    }
    template <class F>
    static void read_element(A& a, std::size_t, F&& body)
    {
        body(a);
    }
    template <class F>
    static void read_tagged(A& a, tag_kind expected, F&& body)
    {
        a.expect(18);
        const auto kind    = a.get();
        const auto present = a.get();
        if (kind != static_cast<std::uint8_t>(expected) || present > 1)
            throw serialization_error(error_code::invalid_value, "Invalid binary tag");
        tagged_header h{expected, present != 0, a.count(), a.text()};
        body(a, h);
    }
};
}  // namespace serialization
