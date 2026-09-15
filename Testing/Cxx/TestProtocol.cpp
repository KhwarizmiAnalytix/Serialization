#include <limits>

#include "TestSupport.h"
#include "version.h"

namespace external
{
struct Writer
{
    std::vector<int> values;
};
struct Number
{
    int value;
};
class Model
{
    int  selected_ = 17;
    int  omitted_  = 99;
    void initialize() {}
    SERIALIZATION_MACRO(Model, selected_);
};
}  // namespace external
namespace serialization
{
template <>
struct archive_traits<external::Writer>
{
    static void write_scalar(external::Writer& a, int value) { a.values.push_back(value); }
    template <class F>
    static void write_object(external::Writer& a, object_header, F&& body)
    {
        body(a);
    }
    template <class F>
    static void write_field(external::Writer& a, field_key, F&& body)
    {
        body(a);
    }
    template <class F>
    static void write_sequence(external::Writer& a, std::size_t, F&& body)
    {
        body(a);
    }
    template <class F>
    static void write_element(external::Writer& a, std::size_t, F&& body)
    {
        body(a);
    }
    template <class F>
    static void write_tagged(external::Writer& a, tagged_header h, F&& body)
    {
        if (h.present)
            body(a);
    }
};
template <>
struct type_codec<external::Number>
{
    template <class A, class C>
    static void save(A& a, const external::Number& n, C& context)
    {
        context.save(a, n.value);
    }
};
}  // namespace serialization
// This supplies generated-format metadata by hand; it does not claim to test an AST generator.
namespace serialization::generated
{
template <>
struct metadata<test_support::Record>
{
    static constexpr auto properties()
    {
        return std::make_tuple(
            reflection(&test_support::Record::number_, "number_"),
            reflection(&test_support::Record::text_, "text_"));
    }
};
}  // namespace serialization::generated
static_assert(serialization::OutputArchive<external::Writer>);
static_assert(!serialization::InputArchive<external::Writer>);

TEST(CoreProtocol, ExternalAdapterAndCodec)
{
    external::Writer          writer;
    serialization::serializer codec;
    codec.save(writer, external::Model{});
    codec.save(writer, external::Number{23});
    EXPECT_EQ(writer.values, (std::vector<int>{17, 23}));
}
TEST(CoreProtocol, VersionReportsProjectVersion)
{
    EXPECT_STREQ(serialization::version(), "2.0.0");
}
TEST(CoreProtocol, ErrorRetainsCodeMessageAndPath)
{
    serialization::serialization_error error(
        serialization::error_code::invalid_value, "bad scalar", "$.field[0]");
    EXPECT_EQ(error.path(), "$.field[0]");
    EXPECT_EQ(error.message(), "bad scalar");
    EXPECT_EQ(std::string(error.what()), "$.field[0]: bad scalar");
}
TEST(CoreProtocol, NestedCodecRetainsDepthLimit)
{
    external::Writer          writer;
    serialization::serializer codec(serialization::operation_options{.max_depth = 1});
    test_support::expect_error(
        serialization::error_code::depth_limit, [&] { codec.save(writer, external::Number{1}); });
}

TEST(BinaryProtocol, InputAndOutputDirectionsAreSeparate)
{
    static_assert(serialization::OutputArchive<test_support::Binary::Writer>);
    static_assert(!serialization::InputArchive<test_support::Binary::Writer>);
    static_assert(serialization::InputArchive<test_support::Binary::Reader>);
    static_assert(!serialization::OutputArchive<test_support::Binary::Reader>);
}
TEST(BinaryProtocol, FixedPortableIntegerEncoding)
{
    std::vector<std::byte>                 bytes;
    serialization::adapters::binary_writer writer(bytes);
    serialization::serializer              codec;
    codec.save(writer, std::uint64_t{0x0102030405060708});
    std::vector<std::byte> expected;
    for (unsigned char c : {'S', 'R', 'L', '1'})
        expected.push_back(static_cast<std::byte>(c));
    expected.push_back(std::byte{3});
    for (unsigned char c : {8, 7, 6, 5, 4, 3, 2, 1})
        expected.push_back(static_cast<std::byte>(c));
    EXPECT_EQ(bytes, expected);
    serialization::adapters::binary_reader reader(expected);
    std::uint64_t                          loaded = 0;
    codec.load(reader, loaded);
    EXPECT_EQ(loaded, 0x0102030405060708ULL);
    EXPECT_TRUE(reader.empty());
}
TEST(BinaryProtocol, TruncationIsAnError)
{
    std::vector<std::byte>                 bytes;
    serialization::adapters::binary_writer writer(bytes);
    serialization::serializer              codec;
    codec.save(writer, std::int64_t{42});
    bytes.pop_back();
    serialization::adapters::binary_reader reader(bytes);
    std::int64_t                           value = 0;
    test_support::expect_error(
        serialization::error_code::truncated_input, [&] { codec.load(reader, value); });
}
TEST(BinaryProtocol, MacroAndGeneratedProvidersInteroperate)
{
    std::vector<std::byte>                                       bytes;
    serialization::adapters::binary_writer                       writer(bytes);
    serialization::serializer                                    macros;
    serialization::basic_serializer<serialization::ast_metadata> generated;
    macros.save(writer, test_support::Record{19, "provider"});
    serialization::adapters::binary_reader reader(bytes);
    test_support::Record                   loaded;
    generated.load(reader, loaded);
    EXPECT_EQ(loaded.number(), 19);
    EXPECT_EQ(loaded.initializations(), 1);
    std::vector<std::byte>                 generated_bytes;
    serialization::adapters::binary_writer other(generated_bytes);
    generated.save(other, loaded);
    EXPECT_EQ(bytes, generated_bytes);
    serialization::adapters::binary_reader other_reader(generated_bytes);
    test_support::Record                   second;
    macros.load(other_reader, second);
    EXPECT_EQ(second, loaded);
}
TEST(JsonProtocol, ReadsDoNotMutateAndMissingFieldsHavePaths)
{
    test_support::Json::Storage document;
    auto                        writer = test_support::Json::writer(document);
    serialization::serializer   codec;
    codec.save(writer, test_support::Record{7, "seven"});
    const auto           original = document.dump();
    auto                 reader   = test_support::Json::reader(document);
    test_support::Record loaded;
    codec.load(reader, loaded);
    EXPECT_EQ(document.dump(), original);
    document["$fields"].erase("number_");
    const auto           missing = document.dump();
    auto                 invalid = test_support::Json::reader(document);
    test_support::Record failed;
    try
    {
        codec.load(invalid, failed);
        FAIL();
    }
    catch (const serialization::serialization_error& error)
    {
        EXPECT_EQ(error.code(), serialization::error_code::missing_field);
        EXPECT_EQ(error.path(), "$.number_");
    }
    EXPECT_EQ(failed.initializations(), 0);
    EXPECT_EQ(document.dump(), missing);
}
TEST(JsonProtocol, InvalidNativeValueDoesNotOverwriteDestination)
{
    test_support::Json::Storage document = "not a number";
    auto                        reader   = test_support::Json::reader(document);
    test_support::Day           value{17};
    serialization::serializer   codec;
    test_support::expect_error(
        serialization::error_code::invalid_value, [&] { codec.load(reader, value); });
    EXPECT_EQ(value, test_support::Day{17});
}
TEST(JsonProtocol, WrongVersionRejected)
{
    test_support::Json::Storage document;
    auto                        writer = test_support::Json::writer(document);
    serialization::serializer   codec;
    codec.save(writer, test_support::Record{});
    document["$version"]        = 2;
    auto                 reader = test_support::Json::reader(document);
    test_support::Record value;
    test_support::expect_error(
        serialization::error_code::unsupported_version, [&] { codec.load(reader, value); });
}
TEST(XmlProtocol, ReadsDoNotMutateAndMissingFieldsHavePaths)
{
    test_support::Xml::Storage document;
    auto                       writer = test_support::Xml::writer(document);
    serialization::serializer  codec;
    codec.save(writer, test_support::Record{7, "seven"});
    std::ostringstream before;
    document.save(before);
    auto                 reader = test_support::Xml::reader(document);
    test_support::Record loaded;
    codec.load(reader, loaded);
    std::ostringstream after;
    document.save(after);
    EXPECT_EQ(before.str(), after.str());
    document.child("value").remove_child(document.child("value").child("field"));
    std::ostringstream missing_before;
    document.save(missing_before);
    auto                 invalid = test_support::Xml::reader(document);
    test_support::Record failed;
    try
    {
        codec.load(invalid, failed);
        FAIL();
    }
    catch (const serialization::serialization_error& error)
    {
        EXPECT_EQ(error.code(), serialization::error_code::missing_field);
        EXPECT_EQ(error.path(), "$.number_");
    }
    EXPECT_EQ(failed.initializations(), 0);
    std::ostringstream missing_after;
    document.save(missing_after);
    EXPECT_EQ(missing_before.str(), missing_after.str());
}
TEST(XmlProtocol, RejectsInvalidNumbersAndUnsupportedNul)
{
    test_support::Xml::Storage document;
    auto                       writer = test_support::Xml::writer(document);
    serialization::serializer  codec;
    codec.save(writer, 42);
    writer.node.text().set("42garbage");
    auto reader = test_support::Xml::reader(document);
    int  value  = 0;
    test_support::expect_error(
        serialization::error_code::invalid_value, [&] { codec.load(reader, value); });
    test_support::expect_error(
        serialization::error_code::invalid_value,
        [&] { codec.save(writer, std::string{"a\0b", 3}); });
}
