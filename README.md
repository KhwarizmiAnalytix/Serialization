# C++20 Serialization Library

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

A high-performance, type-safe serialization library for C++20 featuring compile-time reflection, concepts-based type constraints, and support for JSON, Binary, and XML formats.

Developed and maintained by [QuarismAnalytix](https://github.com/QuarismAnalytix).

The proposed [extensible serialization design](Docs/extensible_serialization_design.md)
describes a backend-independent template core, optional archive adapters, and
shared metadata interfaces for macro reflection and AST generation. It is a
design and migration plan; the APIs and targets it proposes are not implemented.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Supported Types](#supported-types)
- [Unsupported Types](#unsupported-types)
- [Quick Start](#quick-start)
- [Reflection System](#reflection-system)
- [Polymorphic Serialization](#polymorphic-serialization)
- [Usage Examples](#usage-examples)
- [API Reference](#api-reference)
- [Contributing](#contributing)
- [License](#license)
- [Author](#author)

## Overview

This library provides a modern C++20 approach to serialization with:

- **Compile-time type safety** using C++20 concepts
- **Zero-overhead abstractions** with constexpr and template metaprogramming
- **Multiple serialization formats** (JSON via nlohmann::json, binary via multi_process_stream)
- **Automatic reflection** for custom types using macros
- **Polymorphic serialization** with type registry
- **Thread-safe operations** with cached type information

The library is designed for high performance with features like cached type names (10,000x faster than repeated demangling), compile-time type validation, and minimal runtime overhead.

## Features

- - **C++20 Concepts** - Type-safe serialization with compile-time checking
- - **Reflection System** - Automatic serialization of custom classes
- - **Multiple Formats** - JSON and binary serialization
- - **Polymorphism Support** - Serialize derived classes through base pointers
- - **Container Support** - Comprehensive support for STL containers
- - **Smart Pointers** - Full support for std::unique_ptr and std::shared_ptr
- - **Advanced Types** - std::tuple, std::pair, std::variant, std::array
- - **Thread Safety** - Thread-safe registry and cached operations
- - **Error Handling** - Enhanced error messages with std::format and source_location
- - **Performance** - Optimized with compile-time computation and caching

## Requirements

- **C++20 or later** (requires concepts, std::format, std::source_location)
- **Compiler**: GCC 11+, Clang 13+, MSVC 19.29+
- **CMake**: 3.15 or later
- **Dependencies**:
  - nlohmann/json (for JSON serialization)
  - pugixml (for XML serialization)
  - Standard library with C++20 support

## Installation

### Using CMake

1. Clone the repository:
```bash
git clone https://github.com/QuarismAnalytix/Serialization.git
cd Serialization
```

2. Build the library:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

3. Run tests:
```bash
ctest
# Or run individual tests
./TestJsonSerialization
./TestBinarySerialization
./TestXmlSerialization
```

### Integration with Your Project

Add to your `CMakeLists.txt`:
```cmake
add_subdirectory(path/to/Serialization)
target_link_libraries(your_target PRIVATE Serialization)
```

## Supported Types

### Primitive Types (BaseSerializable)

All arithmetic types and strings are directly serializable:

```cpp
#include "core/serialization_impl.h"
using namespace serialization;

// Integers
int i = 42;
long long ll = 123456789LL;
size_t sz = 1024;

// Floating point
float f = 3.14f;
double d = 2.71828;

// Characters and strings
char c = 'A';
bool b = true;
std::string str = "Hello, World!";
const char* cstr = "C-style string";

// Enums
enum class Color { Red, Green, Blue };
Color color = Color::Red;

// JSON serialization
json archive;
save(archive, i);
save(archive, str);
save(archive, color);
```

### Sequential Containers

Supports all standard sequential containers:

```cpp
// Vector
std::vector<int> vec{1, 2, 3, 4, 5};
save(archive, vec);

// List
std::list<double> lst{1.1, 2.2, 3.3};
save(archive, lst);

// Deque
std::deque<std::string> deq{"one", "two", "three"};
save(archive, deq);
```

### Associative Containers

Full support for maps and sets:

```cpp
// Set
std::set<int> s{1, 2, 3, 4, 5};
save(archive, s);

// Unordered set
std::unordered_set<std::string> us{"apple", "banana", "orange"};
save(archive, us);

// Map
std::map<int, std::string> m{{1, "one"}, {2, "two"}, {3, "three"}};
save(archive, m);

// Unordered map
std::unordered_map<std::string, int> um{{"one", 1}, {"two", 2}};
save(archive, um);

// Multimap and multiset also supported
std::multimap<int, std::string> mm{{1, "a"}, {1, "b"}};
std::multiset<int> ms{1, 1, 2, 3};
```

### Fixed-Size Arrays

```cpp
// std::array
std::array<int, 5> arr{1, 2, 3, 4, 5};
save(archive, arr);

// Size is validated during deserialization
std::array<int, 5> loaded_arr;
load(archive, loaded_arr);  // OK: same size
// std::array<int, 6> wrong_size;
// load(archive, wrong_size);  // Error: size mismatch
```

### Pairs and Tuples

```cpp
// std::pair
std::pair<int, std::string> p{42, "answer"};
save(archive, p);

// std::tuple
std::tuple<int, double, std::string> t{1, 3.14, "pi"};
save(archive, t);

// Nested tuples
std::tuple<std::pair<int, int>, std::string> nested{{1, 2}, "nested"};
save(archive, nested);
```

### Smart Pointers

```cpp
// std::unique_ptr
auto uptr = std::make_unique<MyClass>(42);
save(archive, uptr);

std::unique_ptr<MyClass> loaded_uptr;
load(archive, loaded_uptr);

// std::shared_ptr
auto sptr = std::make_shared<MyClass>(42);
save(archive, sptr);

std::shared_ptr<MyClass> loaded_sptr;
load(archive, loaded_sptr);

// Const pointers (removes const during deserialization)
std::shared_ptr<const MyClass> const_ptr = std::make_shared<MyClass>(42);
save(archive, const_ptr);
```

### Variants

```cpp
// std::variant
std::variant<int, double, std::string> v = "hello";
save(archive, v);

std::variant<int, double, std::string> loaded_v;
load(archive, loaded_v);  // loaded_v holds "hello"

// Variants with complex types
using ComplexVariant = std::variant<
    int,
    std::vector<double>,
    std::map<std::string, int>
>;
ComplexVariant cv = std::vector<double>{1.1, 2.2, 3.3};
save(archive, cv);
```

### std::optional

```cpp
// std::optional with value
std::optional<int> opt1 = 42;
save(archive, opt1);

// std::optional without value (std::nullopt)
std::optional<int> opt2 = std::nullopt;
save(archive, opt2);

// Round-trip preserves has_value state
std::optional<std::string> opt3 = "hello";
save(archive, opt3);

std::optional<std::string> loaded;
load(archive, loaded);
assert(loaded.has_value() && *loaded == "hello");

// Complex types in optional
std::optional<std::vector<int>> opt4 = std::vector<int>{1, 2, 3};
save(archive, opt4);
```

### Custom Classes with Reflection

See [Reflection System](#reflection-system) for detailed explanation.

```cpp
class Person
{
public:
    Person(std::string name, int age)
        : name_(std::move(name)), age_(age) {}

private:
    void initialize() {}  // Called after deserialization
    Person() = default;   // Required for deserialization

    // Add reflection metadata
    SERIALIZATION_MACRO( Person, name_, age_);

    std::string name_;
    int age_;

    friend struct serialization::access::serializer;
};

// Usage
Person p{"Alice", 30};
save(archive, p);
```

## Unsupported Types

The following types are **NOT currently supported**:


### std::span

**Status**: ❌ Not supported
**Reason**: `std::span` is a non-owning view, serializing it would lose data
**Workaround**: Convert to owning container or serialize underlying data

```cpp
// ❌ Not supported
std::vector<int> data{1, 2, 3, 4, 5};
std::span<int> sp(data);
// save(archive, sp);  // Error

// - Workaround: serialize the underlying container
save(archive, data);
```

### std::string_view

**Status**: ❌ Not recommended
**Reason**: Non-owning view, underlying data may be deallocated
**Workaround**: Convert to `std::string`

```cpp
// ❌ Dangerous
std::string_view sv = "temporary";
// The underlying data may be destroyed after serialization

// - Safe: convert to std::string
std::string str(sv);
save(archive, str);
```

### std::chrono Types

**Status**: ❌ Not supported
**Reason**: No specializations for time_point, duration, etc.
**Workaround**: Convert to numeric representation

```cpp
// ❌ Not supported
auto now = std::chrono::system_clock::now();
// save(archive, now);  // Error

// - Workaround: serialize as count
auto timestamp = now.time_since_epoch().count();
save(archive, timestamp);
```

### Raw Pointers

**Status**: ❌ Not supported (by design)
**Reason**: Ownership semantics are unclear, potential memory safety issues
**Workaround**: Use smart pointers

```cpp
// ❌ Not supported
int* raw_ptr = new int(42);
// save(archive, raw_ptr);  // Error

// - Use smart pointers
auto smart_ptr = std::make_unique<int>(42);
save(archive, smart_ptr);
```

### C-style Arrays

**Status**: Note: Partially supported
**Note**: Use `std::array` or `std::vector` instead

```cpp
// Note: Limited support
int arr[5] = {1, 2, 3, 4, 5};
// Not directly serializable

// - Use std::array
std::array<int, 5> std_arr{1, 2, 3, 4, 5};
save(archive, std_arr);
```

## Quick Start

### Basic Serialization

```cpp
#include "core/serialization_impl.h"
#include <iostream>

int main() {
    using namespace serialization;

    // Create data
    std::vector<int> data{1, 2, 3, 4, 5};

    // Serialize to JSON
    json archive;
    save(archive, data);

    std::cout << "JSON: " << archive.dump(2) << std::endl;

    // Deserialize
    std::vector<int> loaded;
    load(archive, loaded);

    // Verify
    assert(data == loaded);

    return 0;
}
```

### Binary Serialization

```cpp
#include "core/serialization_impl.h"
#include "stream/multi_process_stream.h"

int main() {
    using namespace serialization;

    // Create data
    std::map<int, std::string> data{{1, "one"}, {2, "two"}};

    // Serialize to binary
    multi_process_stream buffer;
    save(buffer, data);

    // Deserialize
    std::map<int, std::string> loaded;
    load(buffer, loaded);

    assert(data == loaded);

    return 0;
}
```

## Reflection System

### What is Reflection?

The reflection system allows the library to introspect custom classes at compile-time, automatically generating serialization code for all member variables. This eliminates the need to manually write save/load functions.

### How Reflection Works

1. **Compile-time metadata**: The `SERIALIZATION_MACRO` generates a compile-time tuple of member pointers and names
2. **Automatic traversal**: During serialization, the library iterates over this metadata
3. **Type-safe access**: Member variables are accessed through pointer-to-member and serialized recursively

### Using the SERIALIZATION_MACRO

The macro has the following signature:

```cpp
SERIALIZATION_MACRO(ClassName, member1, member2, ...)
```

**Parameters**:
- `export_spec`: Export specification (use `SERIALIZATION_API` or empty)
- `ClassName`: Name of the class
- `member1, member2, ...`: List of member variables to serialize

### Step-by-Step: Making a Class Serializable

#### Step 1: Define Your Class

```cpp
#include "reflection/reflection_macros.h"

class BankAccount
{
public:
    // Public constructors and methods
    BankAccount(std::string owner, double balance)
        : owner_(std::move(owner)), balance_(balance), id_(next_id_++) {}

    const std::string& owner() const { return owner_; }
    double balance() const { return balance_; }
    int id() const { return id_; }
```

#### Step 2: Add Required Private Members

```cpp
private:
    // Required: Default constructor (can be private)
    BankAccount() : id_(0), balance_(0.0) {}

    // Required: Initialize method (called after deserialization)
    void initialize() {
        // Perform any post-deserialization setup
        // e.g., re-establish invariants, update caches, etc.
    }
```

#### Step 3: Add Reflection Macro

```cpp
    // Add the reflection macro with all members to serialize
    SERIALIZATION_MACRO( BankAccount, owner_, balance_, id_);

    // Member variables
    std::string owner_;
    double balance_;
    int id_;

    static inline int next_id_ = 1000;
```

#### Step 4: Grant Friend Access

```cpp
    // Required: Grant access to serialization system
    friend struct serialization::access::serializer;
};
```

### Complete Example

```cpp
#include "reflection/reflection_macros.h"
#include "core/serialization_impl.h"

class BankAccount
{
public:
    BankAccount(std::string owner, double balance)
        : owner_(std::move(owner)), balance_(balance), id_(next_id_++) {}

    const std::string& owner() const { return owner_; }
    double balance() const { return balance_; }
    int id() const { return id_; }

private:
    BankAccount() : id_(0), balance_(0.0) {}

    void initialize() {
        std::cout << "Account " << id_ << " deserialized" << std::endl;
    }

    SERIALIZATION_MACRO( BankAccount, owner_, balance_, id_);

    std::string owner_;
    double balance_;
    int id_;

    static inline int next_id_ = 1000;

    friend struct serialization::access::serializer;
};

// Usage
int main() {
    using namespace serialization;

    BankAccount account("Alice", 1500.50);

    json archive;
    save(archive, account);

    BankAccount loaded;
    load(archive, loaded);
    // Prints: "Account 1000 deserialized"

    assert(loaded.owner() == "Alice");
    assert(loaded.balance() == 1500.50);
    assert(loaded.id() == 1000);
}
```

### The initialize() Method

The `initialize()` method is called automatically after deserialization completes. Use it to:

- Re-establish class invariants
- Update derived/cached data
- Register with external systems
- Perform validation

```cpp
class Employee
{
private:
    void initialize() {
        // Recompute full name after deserialization
        full_name_ = first_name_ + " " + last_name_;

        // Validate data
        if (salary_ < 0) {
            throw std::runtime_error("Invalid salary");
        }
    }

    SERIALIZATION_MACRO( Employee,
                       first_name_, last_name_, salary_);

    std::string first_name_;
    std::string last_name_;
    double salary_;
    std::string full_name_;  // Not serialized, recomputed in initialize()
};
```

### Selective Serialization

You can choose which members to serialize:

```cpp
class UserSession
{
private:
    // Only serialize user_id and login_time
    SERIALIZATION_MACRO( UserSession, user_id_, login_time_);

    int user_id_;
    std::string login_time_;

    // These are NOT serialized (transient data)
    std::vector<std::string> temp_cache_;
    mutable std::mutex mutex_;

    friend struct serialization::access::serializer;
};
```

### Best Practices

1. **Always provide a default constructor** (can be private)
2. **Use initialize() for post-deserialization setup**, not the constructor
3. **Order matters**: List members in the macro in the same order as declaration
4. **Don't serialize transient data**: Only serialize data needed to reconstruct state
5. **Mark large/temporary members as transient** to reduce serialization overhead
6. **Use const member accessors** to prevent modification of deserialized state

## Polymorphic Serialization

### Overview

The library supports serializing derived classes through base class pointers using a type registry system. This is essential for polymorphic object hierarchies.

### How Polymorphic Serialization Works

1. **Type Registration**: Derived types are registered in a global registry at program startup
2. **Type Name Storage**: During serialization, the actual type name is stored alongside the data
3. **Dynamic Dispatch**: During deserialization, the type name is used to create the correct derived type
4. **Automatic Upcasting**: The created object is safely cast to the base type pointer

### Step-by-Step: Polymorphic Classes

#### Step 1: Define Base Class

```cpp
#include "reflection/reflection_macros.h"

class Shape
{
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;

    double x() const { return x_; }
    double y() const { return y_; }

protected:
    Shape(double x, double y) : x_(x), y_(y) {}
    Shape() = default;

    void initialize() {}

    SERIALIZATION_MACRO( Shape, x_, y_);

    double x_ = 0.0;
    double y_ = 0.0;

    friend struct serialization::access::serializer;
};
```

#### Step 2: Define Derived Classes

```cpp
class Circle : public Shape
{
public:
    Circle(double x, double y, double radius)
        : Shape(x, y), radius_(radius) {}

    double area() const override {
        return 3.14159 * radius_ * radius_;
    }

    double radius() const { return radius_; }

private:
    Circle() = default;

    void initialize() {}

    // Include base class members + derived class members
    SERIALIZATION_MACRO( Circle, x_, y_, radius_);

    double radius_ = 0.0;

    friend struct serialization::access::serializer;
};

class Rectangle : public Shape
{
public:
    Rectangle(double x, double y, double w, double h)
        : Shape(x, y), width_(w), height_(h) {}

    double area() const override {
        return width_ * height_;
    }

private:
    Rectangle() = default;

    void initialize() {}

    SERIALIZATION_MACRO( Rectangle, x_, y_, width_, height_);

    double width_ = 0.0;
    double height_ = 0.0;

    friend struct serialization::access::serializer;
};
```

#### Step 3: Register Derived Classes

After defining derived classes, register them with the serialization system:

```cpp
// Register for both JSON and binary serialization
SERIALIZATION_REGISTER_DERIVED_SERIALIZATION(Circle);
SERIALIZATION_REGISTER_DERIVED_SERIALIZATION(Rectangle);
```

This macro expands to:
```cpp
// For JSON
SERIALIZATION_REGISTER_FUNCTION(
    JsonSerializationRegistry,
    Circle,
    &register_serializer_impl<json, Circle>
);

// For binary
SERIALIZATION_REGISTER_FUNCTION(
    BinarySerializationRegistry,
    Circle,
    &register_serializer_impl<multi_process_stream, Circle>
);
```

#### Step 4: Serialize Through Base Pointer

```cpp
#include "core/serialization_impl.h"

int main() {
    using namespace serialization;

    // Create shapes
    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.push_back(std::make_shared<Circle>(1.0, 2.0, 5.0));
    shapes.push_back(std::make_shared<Rectangle>(3.0, 4.0, 10.0, 20.0));

    // Serialize
    json archive;
    save(archive, shapes);

    std::cout << "Serialized:\n" << archive.dump(2) << std::endl;

    // Deserialize - correct types are reconstructed!
    std::vector<std::shared_ptr<Shape>> loaded_shapes;
    load(archive, loaded_shapes);

    // Verify polymorphism works
    for (const auto& shape : loaded_shapes) {
        std::cout << "Area: " << shape->area() << std::endl;
    }

    return 0;
}
```

### Type Registry Internals

The registration system uses static initialization:

```cpp
// Global registry (one per serialization format)
Registry<std::string, SerializationFunction>* JsonSerializationRegistry();

// Registerer class (RAII pattern)
class Registerer {
public:
    Registerer(const std::string& type_name,
              Registry* registry,
              SerializationFunction func) {
        registry->Register(type_name, func);
    }
};

// Static instance triggers registration before main()
static Registerer g_Circle_Registerer(
    "Circle",
    JsonSerializationRegistry(),
    &register_serializer_impl<json, Circle>
);
```

### Advanced: Custom Registration

For more control, you can manually register types:

```cpp
// Manual registration
void register_my_types() {
    using namespace serialization;

    auto* json_registry = JsonSerializationRegistry();
    auto* binary_registry = BinarySerializationRegistry();

    // Register with custom function
    json_registry->Register(
        "MyCustomType",
        [](json& archive, void* obj, bool is_loading) {
            // Custom serialization logic
        }
    );
}
```

### Polymorphism with unique_ptr

```cpp
// Works with unique_ptr too
std::unique_ptr<Shape> shape = std::make_unique<Circle>(0, 0, 10);

json archive;
save(archive, shape);

std::unique_ptr<Shape> loaded_shape;
load(archive, loaded_shape);

// loaded_shape points to a Circle object
assert(dynamic_cast<Circle*>(loaded_shape.get()) != nullptr);
```

### Important Notes

1. **Registration must happen before serialization**: Use static initialization or call registration functions at startup
2. **Type names must be unique**: The registry uses demangled type names as keys
3. **Both base and derived must have reflection**: Use `SERIALIZATION_MACRO` on both
4. **Virtual destructors required**: Base class must have virtual destructor for polymorphism
5. **Include base members in derived**: Derived class reflection must include base class members

## Usage Examples

### Example 1: Serializing Nested Containers

```cpp
#include "core/serialization_impl.h"
#include <iostream>

int main() {
    using namespace serialization;

    // Complex nested structure
    std::map<std::string, std::vector<std::pair<int, double>>> data;
    data["series1"] = {{1, 1.5}, {2, 2.5}, {3, 3.5}};
    data["series2"] = {{10, 10.5}, {20, 20.5}};

    // Serialize to JSON
    json archive;
    save(archive, data);

    std::cout << archive.dump(2) << std::endl;

    // Deserialize
    std::map<std::string, std::vector<std::pair<int, double>>> loaded;
    load(archive, loaded);

    assert(loaded == data);
    return 0;
}
```

### Example 2: Serializing with Variants

```cpp
#include "core/serialization_impl.h"

using Value = std::variant<int, double, std::string, std::vector<int>>;

int main() {
    using namespace serialization;

    std::vector<Value> values;
    values.push_back(42);
    values.push_back(3.14);
    values.push_back(std::string("hello"));
    values.push_back(std::vector<int>{1, 2, 3});

    json archive;
    save(archive, values);

    std::vector<Value> loaded;
    load(archive, loaded);

    // Type information is preserved
    assert(std::holds_alternative<int>(loaded[0]));
    assert(std::holds_alternative<double>(loaded[1]));
    assert(std::holds_alternative<std::string>(loaded[2]));
    assert(std::holds_alternative<std::vector<int>>(loaded[3]));

    return 0;
}
```

### Example 3: Complete Application Example

```cpp
#include "core/serialization_impl.h"
#include "reflection/reflection_macros.h"
#include <fstream>

class User
{
public:
    User(int id, std::string name, std::vector<std::string> roles)
        : id_(id), name_(std::move(name)), roles_(std::move(roles)) {}

private:
    User() = default;
    void initialize() {
        std::cout << "User " << name_ << " loaded" << std::endl;
    }

    SERIALIZATION_MACRO( User, id_, name_, roles_);

    int id_;
    std::string name_;
    std::vector<std::string> roles_;

    friend struct serialization::access::serializer;
};

class Database
{
public:
    void add_user(const User& user) {
        users_.push_back(user);
    }

    void save_to_file(const std::string& filename) {
        using namespace serialization;
        json archive;
        save(archive, users_);

        std::ofstream file(filename);
        file << archive.dump(2);
    }

    void load_from_file(const std::string& filename) {
        using namespace serialization;
        std::ifstream file(filename);
        json archive = json::parse(file);

        load(archive, users_);
    }

private:
    Database() = default;
    void initialize() {}

    SERIALIZATION_MACRO( Database, users_);

    std::vector<User> users_;

    friend struct serialization::access::serializer;
};

int main() {
    // Create database
    Database db;
    db.add_user(User(1, "Alice", {"admin", "user"}));
    db.add_user(User(2, "Bob", {"user"}));

    // Save to file
    db.save_to_file("database.json");

    // Load from file
    Database loaded_db;
    loaded_db.load_from_file("database.json");

    return 0;
}
```

### Example 4: Binary Serialization for IPC

```cpp
#include "core/serialization_impl.h"
#include "stream/multi_process_stream.h"

struct Message
{
    int type;
    std::string payload;
    std::map<std::string, std::string> metadata;

private:
    Message() = default;
    void initialize() {}

    SERIALIZATION_MACRO( Message, type, payload, metadata);

    friend struct serialization::access::serializer;
};

void send_message(const Message& msg) {
    using namespace serialization;

    // Serialize to binary stream
    multi_process_stream buffer;
    save(buffer, msg);

    // Get raw bytes for IPC
    auto raw_data = buffer.GetRawData();

    // Send via socket, shared memory, pipe, etc.
    // send_to_process(raw_data.data(), raw_data.size());
}

Message receive_message(const std::vector<unsigned char>& raw_data) {
    using namespace serialization;

    // Reconstruct stream from raw bytes
    multi_process_stream buffer;
    buffer.SetRawData(raw_data);

    // Deserialize
    Message msg;
    load(buffer, msg);

    return msg;
}
```

## API Reference

### Core Functions

#### save

```cpp
template <typename Archiver, typename T>
    requires BaseSerializable<T> || Container<T> || Reflectable<T> ||
             SmartPointer<T> || TupleLike<T> || VariantLike<T>
void save(Archiver& archive, const T& obj);
```

Serializes an object to the given archive.

**Parameters**:
- `archive`: The archiver (json or multi_process_stream)
- `obj`: The object to serialize

**Constraints**: T must satisfy at least one of the serialization concepts.

#### load

```cpp
template <typename Archiver, typename T>
    requires BaseSerializable<T> || Container<T> || Reflectable<T> ||
             SmartPointer<T> || TupleLike<T> || VariantLike<T>
void load(Archiver& archive, T& obj);
```

Deserializes an object from the given archive.

**Parameters**:
- `archive`: The archiver (json or multi_process_stream)
- `obj`: The object to deserialize into

**Constraints**: T must satisfy at least one of the serialization concepts.

### Concepts

#### BaseSerializable

```cpp
template <typename T>
concept BaseSerializable =
    (std::is_arithmetic_v<T> && !std::is_pointer_v<T> && !std::is_array_v<T>) ||
    std::same_as<T, const char*> ||
    std::same_as<T, std::string> ||
    std::is_enum_v<T>;
```

Satisfied by: int, float, double, bool, char, enums, std::string, const char*

#### Container

```cpp
template <typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::size_type;
    typename T::iterator;
    typename T::const_iterator;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
    { t.size() } -> std::convertible_to<typename T::size_type>;
};
```

Satisfied by: std::vector, std::list, std::deque, std::set, std::map, etc.

#### Reflectable

```cpp
template <typename T>
concept Reflectable = requires {
    { access::serializer::tuple<T>() };
};
```

Satisfied by: Classes with `SERIALIZATION_MACRO`

#### SmartPointer

```cpp
template <typename T>
concept SmartPointer = requires(T t) {
    { t.get() } -> std::convertible_to<typename T::element_type*>;
    { t.reset() } -> std::same_as<void>;
    { static_cast<bool>(t) } -> std::same_as<bool>;
    typename T::element_type;
};
```

Satisfied by: std::unique_ptr, std::shared_ptr

#### TupleLike

```cpp
template <typename T>
concept TupleLike = requires {
    typename std::tuple_size<T>::type;
};
```

Satisfied by: std::tuple, std::pair, std::array

#### VariantLike

```cpp
template <typename T>
concept VariantLike = requires(T t) {
    { t.index() } -> std::convertible_to<std::size_t>;
    { std::visit([](auto&&){}, t) };
};
```

Satisfied by: std::variant

### Macros

#### SERIALIZATION_MACRO

```cpp
SERIALIZATION_MACRO(export_spec, ClassName, member1, member2, ...)
```

Adds reflection metadata to a class.

**Parameters**:
- `export_spec`: Export specification (SERIALIZATION_API or empty)
- `ClassName`: Name of the class
- `member1, member2, ...`: Member variables to serialize

**Requirements**:
- Class must have a default constructor (can be private)
- Class must have an `initialize()` method (can be private)
- Must grant friend access to `serialization::access::serializer`

#### SERIALIZATION_REGISTER_DERIVED_SERIALIZATION

```cpp
SERIALIZATION_REGISTER_DERIVED_SERIALIZATION(ClassName)
```

Registers a derived class for polymorphic serialization.

**Parameters**:
- `ClassName`: The derived class to register

**Usage**: Place after class definition, typically in source file

**Effect**: Registers the type in both JSON and binary serialization registries

### Archive Types

#### json (nlohmann::json)

Human-readable JSON format.

**Advantages**:
- Human-readable
- Portable across platforms
- Easy to debug
- Supports schema validation

**Disadvantages**:
- Larger file size
- Slower than binary

#### multi_process_stream

Efficient binary format for IPC and storage.

**Advantages**:
- Compact binary representation
- Fast serialization/deserialization
- Suitable for inter-process communication

**Disadvantages**:
- Not human-readable
- Platform-dependent (endianness)

## Contributing

Contributions are welcome! Please ensure:

1. Code compiles with C++20 standard
2. All tests pass
3. New features include tests
4. Code follows existing style

To contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

MIT License

Copyright (c) 2024 QuarismAnalytix

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Author

**QuarismAnalytix**

- GitHub: [@QuarismAnalytix](https://github.com/QuarismAnalytix)
- Repository: [Serialization](https://github.com/QuarismAnalytix/Serialization)

## Acknowledgments

- Built on [nlohmann/json](https://github.com/nlohmann/json) for JSON support
- Uses [pugixml](https://pugixml.org/) for XML serialization
- Uses C++20 concepts for type-safe serialization
- Inspired by Boost.Serialization and Cereal
