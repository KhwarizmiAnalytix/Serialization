#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "common/serialization_macros.h"
#include "serialization.h"
#include "serialization_impl.h"
#include "util/multi_process_stream.h"

namespace client
{

class datetime
{
public:
    datetime() = default;
    explicit datetime(double excel_serial) : excel_serial_(excel_serial) {}

    explicit operator double() const { return excel_serial_; }

    bool operator==(const datetime&) const = default;

private:
    double excel_serial_{0};
};

class tenor
{
public:
    tenor() = default;
    explicit tenor(std::string value) : value_(std::move(value)) {}

    std::string to_string() const { return value_; }

    bool operator==(const tenor&) const = default;

private:
    std::string value_;
};

class key
{
public:
    key() = default;
    explicit key(std::string id) : id_(std::move(id)) {}

    std::string to_string() const { return id_; }

    bool operator==(const key&) const = default;

private:
    std::string id_;
};

}  // namespace client

SERIALIZATION_NATIVE_CAST(client::datetime, double);
SERIALIZATION_NATIVE_STRING(client::tenor);
SERIALIZATION_NATIVE_STRING(client::key);

namespace client
{

class quote
{
public:
    quote() = default;
    quote(datetime as_of, tenor period, key id)
        : as_of_(as_of), period_(std::move(period)), id_(std::move(id))
    {
    }

    const datetime& as_of() const { return as_of_; }
    const tenor&    period() const { return period_; }
    const key&      id() const { return id_; }

private:
    void initialize() {}
    SERIALIZATION_MACRO(quote, as_of_, period_, id_);

    datetime as_of_;
    tenor    period_;
    key      id_;
};

}  // namespace client

TEST(NativeSerialization, JsonWritesDatetimeAsNumber)
{
    serialization::json archive;
    client::datetime    in{44927.5};
    client::datetime    out;
    serialization::save(archive, in);
    serialization::load(archive, out);

    EXPECT_TRUE(archive.is_number());
    EXPECT_DOUBLE_EQ(archive.get<double>(), 44927.5);
    EXPECT_EQ(in, out);
}

TEST(NativeSerialization, JsonWritesTenorAndKeyAsString)
{
    serialization::json archive;
    client::tenor       in{"3M"};
    client::tenor       out;
    serialization::save(archive, in);
    serialization::load(archive, out);

    EXPECT_TRUE(archive.is_string());
    EXPECT_EQ(archive.get<std::string>(), "3M");
    EXPECT_EQ(in, out);

    serialization::json key_archive;
    client::key         key_in{"USD-LIBOR"};
    client::key         key_out;
    serialization::save(key_archive, key_in);
    serialization::load(key_archive, key_out);
    EXPECT_EQ(key_archive.get<std::string>(), "USD-LIBOR");
    EXPECT_EQ(key_in, key_out);
}

TEST(NativeSerialization, JsonObjectKeepsNativeMembersScalar)
{
    serialization::json archive;
    client::quote       in{client::datetime{44927.0}, client::tenor{"6M"}, client::key{"deal-1"}};
    client::quote       out;
    serialization::save(archive, in);
    serialization::load(archive, out);

    EXPECT_TRUE(archive["as_of_"].is_number());
    EXPECT_TRUE(archive["period_"].is_string());
    EXPECT_TRUE(archive["id_"].is_string());
    EXPECT_EQ(in.as_of(), out.as_of());
    EXPECT_EQ(in.period(), out.period());
    EXPECT_EQ(in.id(), out.id());
}

TEST(NativeSerialization, XmlRoundTrip)
{
    pugi::xml_document doc;
    auto               root = doc.append_child("root");
    client::datetime   in{100.25};
    client::datetime   out;
    serialization::save(root, in);
    serialization::load(root, out);
    EXPECT_EQ(in, out);

    auto          tenor_node = doc.append_child("tenor");
    client::tenor tenor_in{"1Y"};
    client::tenor tenor_out;
    serialization::save(tenor_node, tenor_in);
    serialization::load(tenor_node, tenor_out);
    EXPECT_EQ(tenor_in, tenor_out);
}

TEST(NativeSerialization, BinaryRoundTrip)
{
    serialization::multi_process_stream buffer;
    client::quote in{client::datetime{12.0}, client::tenor{"1W"}, client::key{"k"}};
    client::quote out;
    serialization::save(buffer, in);
    serialization::load(buffer, out);
    EXPECT_EQ(in.as_of(), out.as_of());
    EXPECT_EQ(in.period(), out.period());
    EXPECT_EQ(in.id(), out.id());
}
