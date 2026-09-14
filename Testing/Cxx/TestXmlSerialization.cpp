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
// XML Serialization Tests
//=============================================================================

class XmlSerializationTest : public ::testing::Test
{
protected:
    pugi::xml_document doc;
    pugi::xml_node     buffer;

    void SetUp() override
    {
        // Clear document and create root node before each test
        doc.reset();
        buffer = doc.append_child("test");
    }
};

//=============================================================================
// Basic Type Tests
//=============================================================================

TEST_F(XmlSerializationTest, StringSerialization)
{
    std::string a_in = "name";
    std::string a_out;
    serialization::save(buffer, a_in);
    serialization::load(buffer, a_out);
    EXPECT_EQ(a_in, a_out);
}

TEST_F(XmlSerializationTest, PairSerialization)
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

TEST_F(XmlSerializationTest, VectorSerialization)
{
    std::vector<double> rhs = {1, 2, 4, 6, 8};
    std::vector<double> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

TEST_F(XmlSerializationTest, SetSerialization)
{
    std::set<int> rhs{1, 2, 3, 4, 5};
    std::set<int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs.size(), rhs.size());
    EXPECT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

TEST_F(XmlSerializationTest, ArraySerialization)
{
    std::array<unsigned int, 5> rhs{1, 2, 3, 4, 5};
    std::array<unsigned int, 5> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs.size(), rhs.size());
    EXPECT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

TEST_F(XmlSerializationTest, MapSerialization)
{
    std::map<int64_t, int> rhs{{1, 1}, {2, 2}};
    std::map<int64_t, int> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(lhs.size(), rhs.size());
    EXPECT_TRUE(std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

TEST_F(XmlSerializationTest, UnorderedMapSerialization)
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

TEST_F(XmlSerializationTest, UniquePtrSerialization)
{
    auto rhs = serialization::util::make_ptr_unique_mutable<test::test_serialization>(5.6);
    std::unique_ptr<test::test_serialization> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs->d(), lhs->d());
}

TEST_F(XmlSerializationTest, SharedPtrSerialization)
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

TEST_F(XmlSerializationTest, DerivedTypeSerialization)
{
    const auto& rhs = std::make_shared<test::test_derived_serialization>(6.7, "me");
    serialization::ptr_const<test::test_serialization> lhs;

    pugi::xml_document save_doc;
    auto               root_node = save_doc.append_child("root");
    serialization::save(root_node, rhs);
    serialization::serialization_impl::access::write_xml(
        "test_derived_serialization.xml", save_doc);

    pugi::xml_document load_doc;
    serialization::serialization_impl::access::read_xml("test_derived_serialization.xml", load_doc);
    auto loaded_root = load_doc.child("root");
    serialization::load(loaded_root, lhs);

    auto lhs_derived = std::dynamic_pointer_cast<const test::test_derived_serialization>(lhs);

    EXPECT_NE(lhs_derived, nullptr);
    EXPECT_EQ(rhs->d(), lhs->d());
    EXPECT_EQ(rhs->n(), lhs_derived->n());
}

//=============================================================================
// Variant Tests
//=============================================================================

TEST_F(XmlSerializationTest, VariantSerialization)
{
    std::variant<int, float, std::string> rhs = 6.5f;
    std::variant<int, float, std::string> lhs;
    serialization::save(buffer, rhs);
    serialization::load(buffer, lhs);
    EXPECT_EQ(rhs, lhs);
}

//=============================================================================
// FpML XML File Round-Trip Test
//=============================================================================

TEST_F(XmlSerializationTest, FpmlXmlRoundTrip)
{
    // Load the original FpML XML file
    pugi::xml_document original_doc;
    serialization::serialization_impl::access::read_xml(
        "../../Testing/Cxx/fpml_example_from_claud.xml", original_doc);

    // Verify the document loaded successfully
    ASSERT_FALSE(original_doc.empty());

    // Get the root FpML node
    auto fpml_node = original_doc.child("FpML");
    ASSERT_TRUE(fpml_node);

    // Verify some key elements exist
    EXPECT_TRUE(fpml_node.child("header"));
    EXPECT_TRUE(fpml_node.child("trade"));
    EXPECT_TRUE(fpml_node.child("party"));

    // Write to a new XML file
    const std::string output_file = "test_fpml_output.xml";
    serialization::serialization_impl::access::write_xml(output_file, original_doc);

    // Read the file back
    pugi::xml_document reloaded_doc;
    serialization::serialization_impl::access::read_xml(output_file, reloaded_doc);

    // Verify the reloaded document has the same structure
    auto reloaded_fpml = reloaded_doc.child("FpML");
    ASSERT_TRUE(reloaded_fpml);

    // Verify header information
    auto original_header = fpml_node.child("header");
    auto reloaded_header = reloaded_fpml.child("header");
    EXPECT_STREQ(
        original_header.child("messageId").child_value(),
        reloaded_header.child("messageId").child_value());
    EXPECT_STREQ(
        original_header.child("sentBy").child_value(),
        reloaded_header.child("sentBy").child_value());

    // Verify trade information exists
    auto original_trade = fpml_node.child("trade");
    auto reloaded_trade = reloaded_fpml.child("trade");
    EXPECT_TRUE(original_trade.child("tradeHeader"));
    EXPECT_TRUE(reloaded_trade.child("tradeHeader"));
    EXPECT_TRUE(original_trade.child("swap"));
    EXPECT_TRUE(reloaded_trade.child("swap"));

    // Verify party information
    auto original_party1 = fpml_node.find_child_by_attribute("party", "id", "party1");
    auto reloaded_party1 = reloaded_fpml.find_child_by_attribute("party", "id", "party1");
    EXPECT_STREQ(
        original_party1.child("partyName").child_value(),
        reloaded_party1.child("partyName").child_value());

    auto original_party2 = fpml_node.find_child_by_attribute("party", "id", "party2");
    auto reloaded_party2 = reloaded_fpml.find_child_by_attribute("party", "id", "party2");
    EXPECT_STREQ(
        original_party2.child("partyName").child_value(),
        reloaded_party2.child("partyName").child_value());
}
