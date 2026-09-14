
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "common/serialization_macros.h"
#include "serialization_impl.h"
#include "util/multi_process_stream.h"
#include "util/pointer.h"

//=============================================================================
// Test Classes
//=============================================================================

namespace serialization
{
class test_serialization
{
public:
    explicit test_serialization(double d) : d_(d) {}

    double d() const { return d_; }

private:
    void initialize() {}
    test_serialization() = default;
    SERIALIZATION_MACRO(test_serialization, d_);

    double d_{0};

    friend struct serialization::access::serializer;
};
}  // namespace serialization

//=============================================================================
// Binary Serialization Tests
//=============================================================================

class BinarySerializationTest : public ::testing::Test
{
protected:
    serialization::multi_process_stream buffer;

    void SetUp() override
    {
        // Clear buffer before each test
        buffer.Reset();
    }
};

//=============================================================================
// Basic Type Tests
//=============================================================================

TEST_F(BinarySerializationTest, IntSerialization)
{
    int a_in  = 42;
    int a_out = 0;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(BinarySerializationTest, DoubleSerialization)
{
    double a_in  = 3.14159;
    double a_out = 0.0;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(BinarySerializationTest, BoolSerialization)
{
    bool a_in  = true;
    bool a_out = false;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(BinarySerializationTest, StringSerialization)
{
    std::string a_in = "Hello, World!";
    std::string a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(BinarySerializationTest, EmptyStringSerialization)
{
    std::string a_in = "";
    std::string a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(BinarySerializationTest, StringWithSpecialCharacters)
{
    std::string a_in = "Tab:\tNewline:\nQuote:\"Backslash:\\";
    std::string a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

//=============================================================================
// Container Tests - Edge Cases
//=============================================================================

TEST_F(BinarySerializationTest, EmptyVectorSerialization)
{
    std::vector<int> rhs;
    std::vector<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
    EXPECT_TRUE(lhs.empty());
}

TEST_F(BinarySerializationTest, SingleElementVectorSerialization)
{
    std::vector<int> rhs{42};
    std::vector<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
    EXPECT_EQ(lhs.size(), 1);
    EXPECT_EQ(lhs[0], 42);
}

TEST_F(BinarySerializationTest, VectorOfDoublesSerialization)
{
    std::vector<double> rhs = {1.1, 2.2, 4.4, 6.6, 8.8};
    std::vector<double> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs.size(), lhs.size());
    EXPECT_TRUE(std::equal(rhs.begin(), rhs.end(), lhs.begin()));
}

TEST_F(BinarySerializationTest, EmptySetSerialization)
{
    std::set<int> rhs;
    std::set<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
    EXPECT_TRUE(lhs.empty());
}

TEST_F(BinarySerializationTest, SetOfIntsSerialization)
{
    std::set<int> rhs{1, 2, 3, 4, 5};
    std::set<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs.size(), lhs.size());
    EXPECT_TRUE(std::equal(rhs.begin(), rhs.end(), lhs.begin()));
}

TEST_F(BinarySerializationTest, EmptyMapSerialization)
{
    std::map<int, std::string> rhs;
    std::map<int, std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
    EXPECT_TRUE(lhs.empty());
}

TEST_F(BinarySerializationTest, MapWithIntKeysSerialization)
{
    std::map<int64_t, int> rhs{{1, 10}, {2, 20}, {3, 30}};
    std::map<int64_t, int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs.size(), lhs.size());
    EXPECT_TRUE(std::equal(rhs.begin(), rhs.end(), lhs.begin()));
}

//=============================================================================
// Pair Tests
//=============================================================================

TEST_F(BinarySerializationTest, PairFloatFloatSerialization)
{
    std::pair<float, float> a_in = std::make_pair(1.5f, 2.5f);
    std::pair<float, float> a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(BinarySerializationTest, PairIntStringSerialization)
{
    std::pair<int, std::string> a_in = {42, "answer"};
    std::pair<int, std::string> a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

//=============================================================================
// Array Tests
//=============================================================================

TEST_F(BinarySerializationTest, ArrayOfShortsSerialization)
{
    std::array<short, 5> rhs{1, 2, 3, 4, 5};
    std::array<short, 5> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs.size(), lhs.size());
    EXPECT_TRUE(std::equal(rhs.begin(), rhs.end(), lhs.begin()));
}

TEST_F(BinarySerializationTest, ArrayOfUnsignedIntSerialization)
{
    std::array<unsigned int, 3> rhs{10, 20, 30};
    std::array<unsigned int, 3> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

//=============================================================================
// Unordered Map Tests
//=============================================================================

TEST_F(BinarySerializationTest, UnorderedMapSerialization)
{
    std::unordered_map<size_t, float> rhs{{1, 1.5f}, {2, 2.5f}};
    std::unordered_map<size_t, float> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs, rhs);
}

//=============================================================================
// Smart Pointer Tests
//=============================================================================

TEST_F(BinarySerializationTest, UniquePtrSerialization)
{
    auto rhs = std::make_unique<serialization::test_serialization>(5.6);
    std::unique_ptr<serialization::test_serialization> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs->d(), lhs->d());
}

TEST_F(BinarySerializationTest, SharedPtrSerialization)
{
    serialization::ptr_mutable<serialization::test_serialization> rhs =
        std::make_shared<serialization::test_serialization>(6.7);
    serialization::ptr_const<serialization::test_serialization> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs->d(), lhs->d());
}

//=============================================================================
// Optional Tests
//=============================================================================

TEST_F(BinarySerializationTest, OptionalWithValueSerialization)
{
    std::optional<int> rhs = 42;
    std::optional<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_TRUE(lhs.has_value());
    EXPECT_EQ(*lhs, 42);
}

TEST_F(BinarySerializationTest, OptionalWithoutValueSerialization)
{
    std::optional<int> rhs = std::nullopt;
    std::optional<int> lhs = 999;  // Start with value
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_FALSE(lhs.has_value());
}

TEST_F(BinarySerializationTest, OptionalStringSerialization)
{
    std::optional<std::string> rhs = "Hello";
    std::optional<std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_TRUE(lhs.has_value());
    EXPECT_EQ(*lhs, "Hello");
}

TEST_F(BinarySerializationTest, OptionalVectorSerialization)
{
    std::optional<std::vector<int>> rhs = std::vector<int>{1, 2, 3};
    std::optional<std::vector<int>> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_TRUE(lhs.has_value());
    std::vector<int> expected{1, 2, 3};
    EXPECT_EQ(*lhs, expected);
}

//=============================================================================
// Variant Tests
//=============================================================================

TEST_F(BinarySerializationTest, VariantWithIntSerialization)
{
    std::variant<int, double, std::string> rhs = 42;
    std::variant<int, double, std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_TRUE(std::holds_alternative<int>(lhs));
    EXPECT_EQ(std::get<int>(lhs), 42);
}

TEST_F(BinarySerializationTest, VariantWithStringSerialization)
{
    std::variant<int, double, std::string> rhs = std::string("hello");
    std::variant<int, double, std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_TRUE(std::holds_alternative<std::string>(lhs));
    EXPECT_EQ(std::get<std::string>(lhs), "hello");
}

TEST_F(BinarySerializationTest, VariantWithMonostateSerialization)
{
    std::variant<std::monostate, int, std::string> rhs = std::monostate{};
    std::variant<std::monostate, int, std::string> lhs = 42;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(lhs));
}

//=============================================================================
// Tuple Tests
//=============================================================================

TEST_F(BinarySerializationTest, TupleSerialization)
{
    std::tuple<int, double, std::string> rhs{42, 3.14, "test"};
    std::tuple<int, double, std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(std::get<0>(lhs), 42);
    EXPECT_EQ(std::get<1>(lhs), 3.14);
    EXPECT_EQ(std::get<2>(lhs), "test");
}

//=============================================================================
// Nested Structure Tests
//=============================================================================

TEST_F(BinarySerializationTest, VectorOfVectorsSerialization)
{
    std::vector<std::vector<int>> rhs{{1, 2}, {3, 4, 5}, {6}};
    std::vector<std::vector<int>> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

TEST_F(BinarySerializationTest, MapOfVectorsSerialization)
{
    std::map<std::string, std::vector<int>> rhs{{"first", {1, 2, 3}}, {"second", {4, 5}}};
    std::map<std::string, std::vector<int>> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

TEST_F(BinarySerializationTest, VectorOfPairsSerialization)
{
    std::vector<std::pair<int, std::string>> rhs{{1, "one"}, {2, "two"}, {3, "three"}};
    std::vector<std::pair<int, std::string>> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

TEST_F(BinarySerializationTest, VectorOfOptionalsSerialization)
{
    std::vector<std::optional<int>> rhs{1, std::nullopt, 3, std::nullopt, 5};
    std::vector<std::optional<int>> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs.size(), lhs.size());
    EXPECT_TRUE(lhs[0].has_value() && *lhs[0] == 1);
    EXPECT_FALSE(lhs[1].has_value());
    EXPECT_TRUE(lhs[2].has_value() && *lhs[2] == 3);
    EXPECT_FALSE(lhs[3].has_value());
    EXPECT_TRUE(lhs[4].has_value() && *lhs[4] == 5);
}
