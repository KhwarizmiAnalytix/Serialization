#include "TestSupport.h"
#include <array>
#include <deque>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace
{
using Backends = ::testing::Types<test_support::Binary, test_support::Json, test_support::Xml>;
template<class B> class SerializationTest : public ::testing::Test {};
TYPED_TEST_SUITE(SerializationTest, Backends);
using namespace test_support;
using serialization::error_code;

template<class B, class T> void check(const T& value) { EXPECT_EQ((round_trip<B>(value)), value); }

TYPED_TEST(SerializationTest, Scalars)
{
    check<TypeParam>(42); check<TypeParam>(-21); check<TypeParam>(true); check<TypeParam>(false);
    check<TypeParam>(3.125); check<TypeParam>(1.25f); check<TypeParam>(static_cast<short>(12));
    check<TypeParam>(static_cast<unsigned char>(255)); check<TypeParam>('A');
    check<TypeParam>(std::numeric_limits<std::int64_t>::min());
    check<TypeParam>(std::numeric_limits<std::uint64_t>::max());
    check<TypeParam>(std::string{}); check<TypeParam>(std::string{"text <>&\" and UTF-8: café"});
    enum class Mode : unsigned { first, second }; check<TypeParam>(Mode::second);
}
TYPED_TEST(SerializationTest, Sequences)
{
    check<TypeParam>(std::vector<int>{}); check<TypeParam>(std::vector<int>{1,2,3});
    check<TypeParam>(std::vector<bool>{true,false,true});
    check<TypeParam>(std::deque<double>{1.25,2.5}); check<TypeParam>(std::list<std::string>{"a","b"});
    check<TypeParam>(std::vector<std::vector<int>>{{1,2},{},{3}});
}
TYPED_TEST(SerializationTest, AssociativeContainers)
{
    check<TypeParam>(std::set<int>{1,2,3}); check<TypeParam>(std::multiset<int>{1,1,2});
    check<TypeParam>(std::unordered_set<int>{7,4}); check<TypeParam>(std::map<int,std::string>{{1,"one"},{2,"two"}});
    check<TypeParam>(std::unordered_map<int,int>{{1,2},{3,4}});
    check<TypeParam>(std::multimap<int,int>{{1,2},{1,3}});
}
TYPED_TEST(SerializationTest, TuplePairAndArray)
{
    check<TypeParam>(std::pair<int,std::string>{3,"x"});
    check<TypeParam>(std::tuple<int,double,std::string>{42,1.25,"tuple"});
    check<TypeParam>(std::array<int,3>{1,2,3}); check<TypeParam>(std::array<int,0>{});
    check<TypeParam>(std::tuple<>{});
}
TYPED_TEST(SerializationTest, NullableAndVariants)
{
    check<TypeParam>(std::optional<int>{}); check<TypeParam>(std::optional<int>{42});
    check<TypeParam>(std::optional<std::vector<int>>{{1,2}});
    check<TypeParam>(std::variant<std::monostate,int,std::string>{std::string{"variant"}});
    check<TypeParam>(std::variant<std::monostate,int>{std::monostate{}});
}
TYPED_TEST(SerializationTest, SelectedPrivateMembersAndInitialization)
{
    auto loaded = round_trip<TypeParam>(Record{21,"record"});
    EXPECT_EQ(loaded.number(),21); EXPECT_EQ(loaded.cache(),42); EXPECT_EQ(loaded.initializations(),1);
    auto empty = round_trip<TypeParam>(Empty{}); EXPECT_EQ(empty.initializations,1);
}
TYPED_TEST(SerializationTest, NativeScalars)
{
    check<TypeParam>(Day{17.5}); check<TypeParam>(Tenor{"6M"}); check<TypeParam>(NativeRecord{});
}
TYPED_TEST(SerializationTest, PointersAndPrivateConstruction)
{
    auto unique = round_trip<TypeParam>(PrivateRecord::make(37)); ASSERT_TRUE(unique); EXPECT_EQ(unique->value(),37);
    auto shared = round_trip<TypeParam>(std::make_shared<Record>(7,"shared")); ASSERT_TRUE(shared); EXPECT_EQ(shared->number(),7);
    auto constant = round_trip<TypeParam>(std::make_shared<const Record>(9,"const")); ASSERT_TRUE(constant); EXPECT_EQ(constant->number(),9);
    EXPECT_FALSE(round_trip<TypeParam>(std::unique_ptr<int>{})); EXPECT_FALSE(round_trip<TypeParam>(std::shared_ptr<int>{}));
}
TYPED_TEST(SerializationTest, PolymorphicRegistryAndPrivateBaseMembers)
{
    using B = TypeParam;
    serialization::type_catalog<Base,Derived> catalog{{"derived-v1"}};
    serialization::polymorphic_registry<typename B::Writer,serialization::macro_metadata,Base> output_registry(catalog);
    serialization::polymorphic_registry<typename B::Reader,serialization::macro_metadata,Base> input_registry(catalog);
    serialization::context<typename B::Writer> output; output.bind(output_registry);
    serialization::context<typename B::Reader> input; input.bind(input_registry);
    typename B::Storage storage; auto writer = B::writer(storage);
    std::shared_ptr<Base> value = std::make_shared<Derived>();
    output.save(writer,value);
    auto reader = B::reader(storage); std::shared_ptr<Base> loaded; input.load(reader,loaded);
    auto derived = std::dynamic_pointer_cast<Derived>(loaded); ASSERT_TRUE(derived);
    EXPECT_EQ(derived->value(),7); EXPECT_EQ(derived->extra(),11);
}
TYPED_TEST(SerializationTest, RegistryRejectsDuplicateAndMutation)
{
    serialization::polymorphic_registry<typename TypeParam::Writer,serialization::macro_metadata,Base> registry;
    registry.template add<Derived>("derived");
    expect_error(error_code::duplicate_type,[&] { registry.template add<Derived>("other"); });
    registry.freeze();
    expect_error(error_code::invalid_registry,[&] { registry.template add<Derived>("other"); });
}
TYPED_TEST(SerializationTest, UnknownDynamicType)
{
    typename TypeParam::Storage storage; auto writer = TypeParam::writer(storage);
    std::shared_ptr<Base> value = std::make_shared<Derived>(); serialization::serializer codec;
    expect_error(error_code::unknown_type,[&] { codec.save(writer,value); });
}
TYPED_TEST(SerializationTest, CyclesAndContextRecovery)
{
    typename TypeParam::Storage storage; auto writer = TypeParam::writer(storage);
    auto node = std::make_shared<Node>(); node->next = node;
    serialization::context<typename TypeParam::Writer> context;
    expect_error(error_code::cycle,[&] { context.save(writer,node); });
    node->next.reset();
    typename TypeParam::Storage clean; auto clean_writer = TypeParam::writer(clean);
    EXPECT_NO_THROW(context.save(clean_writer,node));
}
TYPED_TEST(SerializationTest, DepthAndSizeLimits)
{
    typename TypeParam::Storage storage; auto writer = TypeParam::writer(storage);
    serialization::serializer depth_codec(serialization::operation_options{.max_depth=1});
    expect_error(error_code::depth_limit,[&] { depth_codec.save(writer,Record{1,"x"}); });
    serialization::serializer size_codec(serialization::operation_options{.max_elements=1});
    expect_error(error_code::size_mismatch,[&] { size_codec.save(writer,std::vector<int>{1,2}); });
    serialization::serializer string_codec(serialization::operation_options{.max_string_bytes=1});
    expect_error(error_code::size_mismatch,[&] { string_codec.save(writer,std::string{"long"}); });
}
TYPED_TEST(SerializationTest, NarrowingAndArrayMismatch)
{
    typename TypeParam::Storage storage; auto writer = TypeParam::writer(storage); serialization::serializer codec;
    codec.save(writer,std::uint64_t{1000}); auto reader = TypeParam::reader(storage); std::uint8_t value=0;
    expect_error(error_code::invalid_value,[&] { codec.load(reader,value); });
    typename TypeParam::Storage sequence; auto sequence_writer=TypeParam::writer(sequence);
    codec.save(sequence_writer,std::vector<int>{1,2}); auto sequence_reader=TypeParam::reader(sequence);
    std::array<int,3> array{};
    expect_error(error_code::size_mismatch,[&] { codec.load(sequence_reader,array); });
}
}
