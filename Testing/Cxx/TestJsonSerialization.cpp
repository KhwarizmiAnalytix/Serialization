#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "common/serialization_macros.h"
#include "serialization.h"
#include "serialization_impl.h"
#include "util/pointer.h"

namespace test
{
class test_serialization
{
public:
    explicit test_serialization(double d) : d_(d) {}

    const auto d() const { return d_; }

    virtual ~test_serialization() = default;

protected:
    void initialize() {};
    test_serialization() = default;
    SERIALIZATION_MACRO(test_serialization, d_);

protected:
    double d_{0};
};

class test_derived_serialization final : public test_serialization
{
public:
    test_derived_serialization(double d, std::string n) : test_serialization(d), n_(std::move(n)) {}

    const auto& n() const { return n_; }

private:
    void initialize() {};
    test_derived_serialization() = default;
    SERIALIZATION_MACRO_DERIVED(test_derived_serialization, test_serialization, n_);

    std::string n_;
};

SERIALIZATION_REGISTER_DERIVED_SERIALIZATION(test_derived_serialization);
}  // namespace test

//=============================================================================
// JSON Serialization Tests
//=============================================================================

class JsonSerializationTest : public ::testing::Test
{
protected:
    serialization::json buffer;

    void SetUp() override
    {
        // Clear buffer before each test
        buffer = serialization::json();
    }
};

//=============================================================================
// Basic Type Tests
//=============================================================================

TEST_F(JsonSerializationTest, StringSerialization)
{
    std::string a_in = "name";
    std::string a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(JsonSerializationTest, PairSerialization)
{
    std::pair<float, float> a_in = std::make_pair(1.0f, 1.0f);
    std::pair<float, float> a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

//=============================================================================
// Container Tests
//=============================================================================

TEST_F(JsonSerializationTest, VectorSerialization)
{
    std::vector<double> rhs = {1, 2, 4, 6, 8};
    std::vector<double> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

TEST_F(JsonSerializationTest, SetSerialization)
{
    std::set<int> rhs{1, 2, 3, 4, 5};
    std::set<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs.size(), rhs.size());
    EXPECT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

TEST_F(JsonSerializationTest, ArraySerialization)
{
    std::array<unsigned int, 5> rhs{1, 2, 3, 4, 5};
    std::array<unsigned int, 5> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs.size(), rhs.size());
    EXPECT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

TEST_F(JsonSerializationTest, MapSerialization)
{
    std::map<int64_t, int> rhs{{1, 1}, {2, 2}};
    std::map<int64_t, int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs.size(), rhs.size());
    EXPECT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

TEST_F(JsonSerializationTest, UnorderedMapSerialization)
{
    std::unordered_map<uint64_t, float> rhs{{1, 1.0f}, {2, 2.0f}, {3, 1.3f}, {4, 2.3f}};
    std::unordered_map<uint64_t, float> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs, rhs);
}

//=============================================================================
// Smart Pointer Tests
//=============================================================================

TEST_F(JsonSerializationTest, UniquePtrSerialization)
{
    auto rhs = serialization::util::make_ptr_unique_mutable<test::test_serialization>(5.6);
    std::unique_ptr<test::test_serialization> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs->d(), lhs->d());
}

TEST_F(JsonSerializationTest, SharedPtrSerialization)
{
    serialization::ptr_mutable<test::test_serialization> rhs =
        std::make_shared<test::test_serialization>(6.7);
    serialization::ptr_const<test::test_serialization> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs->d(), lhs->d());
}

//=============================================================================
// Polymorphic Type Tests
//=============================================================================

TEST_F(JsonSerializationTest, DerivedTypeSerialization)
{
    const auto& rhs = std::make_shared<test::test_derived_serialization>(6.7, "me");
    serialization::ptr_const<test::test_serialization> lhs;
    serialization::save(buffer, rhs);
    serialization::serialization_impl::access::write_json(
        "test_derived_serialization.json", buffer);

    serialization::json root;
    serialization::serialization_impl::access::read_json("test_derived_serialization.json", root);
    serialization::load(root, lhs);

    auto lhs_derived = std::dynamic_pointer_cast<const test::test_derived_serialization>(lhs);

    EXPECT_NE(lhs_derived, nullptr);
    EXPECT_EQ(rhs->d(), lhs->d());
    EXPECT_EQ(rhs->n(), lhs_derived->n());
}

//=============================================================================
// Variant Tests
//=============================================================================

TEST_F(JsonSerializationTest, VariantSerialization)
{
    std::variant<int, float, std::string> rhs = 6.5f;
    std::variant<int, float, std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}
