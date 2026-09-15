#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace serialization
{
template<class Archive> struct archive_traits;

enum class field_presence { missing, present };
enum class tag_kind : std::uint8_t { nullable, variant, polymorphic };
struct field_key { std::string_view name; std::size_t position; };
struct object_header
{
    std::string type_id;
    std::uint32_t version = 0;
    std::size_t field_count = 0;
};
struct tagged_header
{
    tag_kind kind = tag_kind::nullable;
    bool present = false;
    std::size_t index = 0;
    std::string type_id;
};

namespace protocol_detail
{
struct body
{
    template<class... T> void operator()(T&&...) const;
};
}

// Direction and structure are checked here. Scalar support is checked at each
// concrete use; no archive is forced to support a value type it cannot encode.
template<class A>
concept OutputArchive = requires(A& a, object_header o, tagged_header t, field_key f)
{
    archive_traits<A>::write_object(a, o, protocol_detail::body{});
    archive_traits<A>::write_field(a, f, protocol_detail::body{});
    archive_traits<A>::write_sequence(a, std::size_t{}, protocol_detail::body{});
    archive_traits<A>::write_element(a, std::size_t{}, protocol_detail::body{});
    archive_traits<A>::write_tagged(a, t, protocol_detail::body{});
};

template<class A>
concept InputArchive = requires(A& a, object_header o, field_key f)
{
    archive_traits<A>::read_object(a, o, protocol_detail::body{});
    { archive_traits<A>::read_field(a, f, protocol_detail::body{}) } -> std::same_as<field_presence>;
    archive_traits<A>::read_sequence(a, protocol_detail::body{});
    archive_traits<A>::read_element(a, std::size_t{}, protocol_detail::body{});
    archive_traits<A>::read_tagged(a, tag_kind::nullable, protocol_detail::body{});
};

template<class A, class T>
concept ScalarOutput = requires(A& a, const T& value)
{
    archive_traits<A>::write_scalar(a, value);
};
template<class A, class T>
concept ScalarInput = requires(A& a, T& value)
{
    archive_traits<A>::read_scalar(a, value);
};
}
