#pragma once
#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "serializer.h"
#include "metadata/macro.h"
#include "adapters/binary.h"
#include "adapters/json.h"
#include "adapters/xml.h"

namespace test_support
{
struct Binary
{
    using Storage = std::vector<std::byte>;
    using Writer = serialization::adapters::binary_writer;
    using Reader = serialization::adapters::binary_reader;
    static Writer writer(Storage& storage) { return Writer(storage); }
    static Reader reader(const Storage& storage) { return Reader(storage); }
};
struct Json
{
    using Storage = serialization::adapters::json;
    using Writer = serialization::adapters::json_writer;
    using Reader = serialization::adapters::json_reader;
    static Writer writer(Storage& storage) { return Writer{storage}; }
    static Reader reader(const Storage& storage) { return Reader{storage}; }
};
struct Xml
{
    using Storage = pugi::xml_document;
    using Writer = serialization::adapters::xml_writer;
    using Reader = serialization::adapters::xml_reader;
    static Writer writer(Storage& storage) { return Writer{storage.append_child("value")}; }
    static Reader reader(const Storage& storage) { return Reader{storage.child("value")}; }
};

template<class Backend, class T> T round_trip(const T& value)
{
    typename Backend::Storage storage;
    auto writer = Backend::writer(storage);
    serialization::serializer codec;
    codec.save(writer, value);
    T loaded{};
    auto reader = Backend::reader(storage);
    codec.load(reader, loaded);
    return loaded;
}
template<class F> void expect_error(serialization::error_code code, F&& operation)
{
    try { operation(); FAIL() << "Expected serialization_error"; }
    catch (const serialization::serialization_error& error) { EXPECT_EQ(error.code(), code); }
}

struct Day
{
    double value = 0;
    explicit Day(double v = 0) : value(v) {}
    explicit operator double() const { return value; }
    bool operator==(const Day&) const = default;
};
struct Tenor
{
    std::string value;
    explicit Tenor(std::string v = {}) : value(std::move(v)) {}
    std::string to_string() const { return value; }
    bool operator==(const Tenor&) const = default;
};
}
SERIALIZATION_NATIVE_CAST(test_support::Day, double)
SERIALIZATION_NATIVE_STRING(test_support::Tenor)

namespace test_support
{
class Record
{
public:
    Record() = default;
    Record(int n, std::string text) : number_(n), text_(std::move(text)) {}
    int number() const { return number_; }
    const std::string& text() const { return text_; }
    int cache() const { return cache_; }
    int initializations() const { return initializations_; }
    bool operator==(const Record& other) const { return number_ == other.number_ && text_ == other.text_; }
private:
    int number_ = 0;
    std::string text_;
    int cache_ = -1;
    int initializations_ = 0;
    void initialize() { cache_ = number_ * 2; ++initializations_; }
    SERIALIZATION_MACRO(Record, number_, text_);
};
struct Empty
{
    int initializations = 0;
private:
    void initialize() { ++initializations; }
    SERIALIZATION_MACRO_EMPTY(Empty);
};
struct NativeRecord
{
    Day day{42.5}; Tenor tenor{"3M"};
    bool operator==(const NativeRecord&) const = default;
private:
    void initialize() {}
    SERIALIZATION_MACRO(NativeRecord, day, tenor);
};
class PrivateRecord
{
public:
    static std::unique_ptr<PrivateRecord> make(int n) { return std::unique_ptr<PrivateRecord>(new PrivateRecord(n)); }
    int value() const { return value_; }
private:
    PrivateRecord() = default;
    explicit PrivateRecord(int n) : value_(n) {}
    int value_ = 0;
    void initialize() {}
    SERIALIZATION_MACRO(PrivateRecord, value_);
};
class Base
{
public:
    virtual ~Base() = default;
    int value() const { return value_; }
private:
    int value_ = 7;
    void initialize() {}
    SERIALIZATION_MACRO(Base, value_);
};
// An extra polymorphic base exercises pointer adjustment during registry dispatch.
struct OtherBase { virtual ~OtherBase() = default; int unrelated = 99; };
class Derived : public OtherBase, public Base
{
public:
    int extra() const { return extra_; }
private:
    int extra_ = 11;
    void initialize() {}
    SERIALIZATION_MACRO_DERIVED(Derived, Base, extra_);
};
struct Node
{
    int value = 1;
    std::shared_ptr<Node> next;
private:
    void initialize() {}
    SERIALIZATION_MACRO(Node, value, next);
};
}
