# C++20 Serialization Library

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

A header-only C++20 serialization core with a template archive parameter: the
same reflection metadata and the same `save`/`load` calls work against JSON,
XML, structured binary, or any archive you write yourself.

Developed and maintained by [KhwarizmiAnalytix](https://github.com/KhwarizmiAnalytix).

The [extensible serialization design](Docs/extensible_serialization_design.md) document
records the architecture decisions and migration rationale behind this design; the
core, adapters, and macros it describes are implemented below.

## Table of Contents

- [Overview](#overview)
- [Requirements](#requirements)
- [Building and Installing](#building-and-installing)
- [Quick Start](#quick-start)
- [Choosing a Backend](#choosing-a-backend)
- [Making a Type Serializable](#making-a-type-serializable)
- [Native Scalars](#native-scalars)
- [Supported and Unsupported Types](#supported-and-unsupported-types)
- [Polymorphic Serialization](#polymorphic-serialization)
- [Error Handling](#error-handling)
- [Limits](#limits)
- [Extending the Library](#extending-the-library)
- [Project Layout](#project-layout)
- [Contributing](#contributing)
- [License](#license)

## Overview

- **One core, many archives.** `serialization::serializer` traverses your
  types once; JSON, XML, and binary adapters (or a custom one you write) each
  implement a small `archive_traits<A>` contract.
- **Two ways to describe members.** `SERIALIZATION_MACRO` for hand-written
  classes, or specialize `serialization::generated::metadata<T>` if you
  generate metadata from an AST tool — both feed the same core.
- **C++20 concepts, not runtime reflection.** `OutputArchive`/`InputArchive`
  gate what an archive must implement; unsupported types fail to compile with
  a `static_assert`, not a runtime surprise.
- **Path-aware errors.** Every thrown `serialization_error` carries a
  `error_code` and a location like `$.orders[2].total`.
- **Built-in guardrails.** Configurable max depth, max element count, and max
  string length; cyclic shared/raw graphs are detected and rejected.

## Requirements

- A C++20 compiler — CI builds Clang, GCC, and MSVC on Linux, macOS, and
  Windows (see `.github/workflows/`).
- CMake 3.20+.
- Optional, only if you enable the corresponding adapter (both vendored under
  `ThirdParty/`, no system install needed):
  - [nlohmann/json](https://github.com/nlohmann/json) for the JSON adapter.
  - [pugixml](https://pugixml.org/) for the XML adapter.

## Building and Installing

```bash
git clone https://github.com/KhwarizmiAnalytix/Serialization.git
cd Serialization
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Build options (all CMake `option()`s, default shown):

| Option | Default | Controls |
| --- | --- | --- |
| `SERIALIZATION_BUILD_TESTING` | `ON` for a top-level build | Builds `SerializationCxxTests` (fetches GoogleTest) |

### Integrating into your project

```cmake
add_subdirectory(path/to/Serialization)
target_link_libraries(your_target PRIVATE
    Serialization::Core     # always available: the header-only template core
    Serialization::Json     # optional: JSON adapter
    Serialization::Xml      # optional: XML adapter
    Serialization::Binary   # optional: structured binary adapter
)
```

`Serialization::Core` is header-only (`INTERFACE`); link only the adapters you
actually use. A `SerializationConfig.cmake` is also installed, so
`find_package(Serialization)` works against an installed copy with the same
target names.

## Quick Start

```cpp
#include "serializer.h"
#include "metadata/macro.h"
#include "adapters/json.h"

class Point
{
public:
    Point(int x, int y) : x_(x), y_(y) {}
    int x() const { return x_; }
    int y() const { return y_; }

private:
    Point() = default;   // required: deserialization constructs, then loads, into this
    void initialize() {} // required: called once after every load

    SERIALIZATION_MACRO(Point, x_, y_);
    int x_ = 0;
    int y_ = 0;
};

int main()
{
    serialization::adapters::json document;                    // nlohmann::ordered_json
    auto                          writer = serialization::adapters::json_writer{document};
    serialization::serializer     codec;

    codec.save(writer, Point{3, 4});
    std::cout << document.dump(2) << "\n";

    auto  reader = serialization::adapters::json_reader{document};
    Point loaded{0, 0};
    codec.load(reader, loaded);
}
```

Containers, `std::optional`, `std::variant`, `std::pair`/`std::tuple`, and
smart pointers work the same way with no extra code — see
[Supported and Unsupported Types](#supported-and-unsupported-types).

## Choosing a Backend

| Backend | Header | Storage type | Notes |
| --- | --- | --- | --- |
| JSON | `adapters/json.h` | `serialization::adapters::json` (`nlohmann::ordered_json`) | Human-readable, easy to diff and debug |
| XML | `adapters/xml.h` | `pugi::xml_document` | Human-readable, useful for interop with XML-based formats |
| Binary | `adapters/binary.h` | `std::vector<std::byte>` | Compact, fixed little-endian encoding, not human-readable |

Each backend has a matching writer/reader pair; `codec.save`/`codec.load` are
identical across all three:

```cpp
// JSON
serialization::adapters::json document;
auto writer = serialization::adapters::json_writer{document};
auto reader = serialization::adapters::json_reader{document};

// XML — writer/reader wrap a pugi::xml_node
pugi::xml_document doc;
auto writer = serialization::adapters::xml_writer{doc.append_child("value")};
auto reader = serialization::adapters::xml_reader{doc.child("value")};

// Binary
std::vector<std::byte> bytes;
serialization::adapters::binary_writer writer(bytes);
serialization::adapters::binary_reader reader(bytes);
```

The binary format is a fixed little-endian encoding with an `SRL1` magic
header; it is portable across machines of either endianness (values are
byte-swapped as needed on read) but is not meant to be human-readable or
edited by hand.

## Making a Type Serializable

Give the class a (possibly private) default constructor, a `void
initialize()` method, and one macro call — the macro grants the serializer
the friend access it needs, so you don't add that yourself:

```cpp
class BankAccount
{
public:
    BankAccount(std::string owner, double balance)
        : owner_(std::move(owner)), balance_(balance) {}

    const std::string& owner() const { return owner_; }
    double balance() const { return balance_; }

private:
    BankAccount() = default;   // required, may be private
    void initialize() {}       // required; runs once after load() populates the members

    SERIALIZATION_MACRO(BankAccount, owner_, balance_);
    std::string owner_;
    double      balance_;
};
```

- **Member order matters**: it's the field order used when writing.
- **Only listed members are serialized**; anything else (caches, mutexes,
  transient state) is naturally excluded.
- **`initialize()` runs after every load**, including nested ones — use it to
  recompute derived state or validate invariants, not for reconstruction the
  constructor should own.

### Classes with no serialized members

```cpp
struct Empty
{
private:
    void initialize() {}
    SERIALIZATION_MACRO_EMPTY(Empty);
};
```

### Derived classes

`SERIALIZATION_MACRO_DERIVED` folds the base class's own `properties()` in
automatically — list only the members `Derived` adds:

```cpp
class Base
{
public:
    virtual ~Base() = default;
    int value() const { return value_; }

private:
    void initialize() {}
    SERIALIZATION_MACRO(Base, value_);
    int value_ = 7;
};

class Derived : public Base
{
public:
    int extra() const { return extra_; }

private:
    void initialize() {}
    SERIALIZATION_MACRO_DERIVED(Derived, Base, extra_);
    int extra_ = 11;
};
```

This also works for classes with private constructors — the library
constructs instances itself during load, bypassing normal access control, so
`Derived` needs no public default constructor either.

## Native Scalars

For a type you don't own (a date, a tenor, a currency code, ...), teach the
library to treat it as a primitive instead of an object with fields:

```cpp
#include "codecs/native.h"

SERIALIZATION_NATIVE_CAST(quant::datetime, double)  // via static_cast<double>/datetime(double)
SERIALIZATION_NATIVE_STRING(quant::tenor)           // via tenor.to_string()/tenor(std::string)
```

After this, `quant::datetime` and `quant::tenor` serialize as a bare number or
string wherever they appear — as a top-level value, a container element, or a
reflected member — with no `SERIALIZATION_MACRO` of their own.

`WireType` in `SERIALIZATION_NATIVE_CAST` must be a library primitive
(arithmetic, enum, or `std::string`) — not another native-cast type.

## Supported and Unsupported Types

Supported without any extra code:

- Arithmetic types, `bool`, `char`, enums, `std::string`
- Anything convertible to `std::string_view` when *saving* (`const char*`,
  `std::string_view`) — copied into a `std::string` on the wire; you cannot
  *load* into a `string_view` itself, since that would leave a dangling view
- Sequential and associative containers that model a range with a
  `value_type` (`std::vector`, `std::list`, `std::deque`, `std::set`,
  `std::multiset`, `std::unordered_set`, `std::map`, `std::multimap`,
  `std::unordered_map`, ...)
- `std::array`, `std::pair`, `std::tuple` (size is checked on load)
- `std::optional<T>`, `std::variant<T...>` (including `std::monostate`)
- `std::shared_ptr<T>`, `std::unique_ptr<T>` — null pointers round-trip as
  absent; polymorphic pointers need [registration](#polymorphic-serialization)
- Classes with `SERIALIZATION_MACRO`/`_EMPTY`/`_DERIVED`, or a
  `serialization::generated::metadata<T>` specialization
- Types with a `native_serializable` specialization (see
  [Native Scalars](#native-scalars))

Not supported (fails to compile with "No serialization metadata or type_codec
for type"):

- **Raw pointers** — ownership is ambiguous; use `std::unique_ptr`/`std::shared_ptr`
- **`std::span`** — a non-owning view; serialize the owning container instead
- **`std::chrono` time points/durations** — convert to a numeric representation first
- **C-style arrays** — use `std::array`

## Polymorphic Serialization

Serializing through a base pointer needs an explicit catalog of concrete
types and a per-archive-direction registry, bound onto a `context`:

```cpp
using Writer = serialization::adapters::json_writer;
using Reader = serialization::adapters::json_reader;

// One string ID per concrete type; change it and you break compatibility
// with data written under the old ID, so treat it like a schema.
serialization::type_catalog<Shape, Circle, Rectangle> catalog{{"circle-v1", "rectangle-v1"}};

serialization::polymorphic_registry<Writer, serialization::macro_metadata, Shape>
    output_registry(catalog);
serialization::polymorphic_registry<Reader, serialization::macro_metadata, Shape>
    input_registry(catalog);

serialization::context<Writer> output;
output.bind(output_registry);
serialization::context<Reader> input;
input.bind(input_registry);

serialization::adapters::json document;
auto writer = Writer{document};
std::shared_ptr<Shape> shape = std::make_shared<Circle>(1.0, 2.0, 5.0);
output.save(writer, shape);

auto reader = Reader{document};
std::shared_ptr<Shape> loaded;
input.load(reader, loaded);
// loaded is a Circle again, safely upcast to shared_ptr<Shape>
```

Requirements:

- `Shape` must have a virtual destructor (`std::has_virtual_destructor_v`).
- Build the catalog and registries once (they're `frozen()` after
  construction) and share the `context` across every `save`/`load` call that
  needs polymorphism — a plain `serialization::serializer` has no registry
  and throws `error_code::unknown_type` for an unregistered dynamic type.
- Both the base and every catalogued derived type need reflection metadata
  (`SERIALIZATION_MACRO`/`_DERIVED`), so their fields have something to save.

## Error Handling

Every failure throws `serialization::serialization_error` (derives
`std::runtime_error`):

```cpp
try
{
    codec.load(reader, value);
}
catch (const serialization::serialization_error& e)
{
    std::cerr << e.code() << " at " << e.path() << ": " << e.message() << "\n";
    // e.what() == "<path>: <message>"
}
```

`error_code` values:

| Code | Meaning |
| --- | --- |
| `missing_field` | A required field was absent while loading |
| `invalid_value` | A value couldn't be parsed, or was out of range for the destination type |
| `size_mismatch` | A fixed-size type, or a depth/element/string limit, didn't match |
| `unsupported_version` | `record_info<T>::version` didn't match what was on the wire |
| `unknown_type` | A polymorphic dynamic type has no matching registry entry |
| `duplicate_type` | Two catalog entries share an ID, or a type was registered twice |
| `truncated_input` | The binary reader ran out of bytes mid-value |
| `depth_limit` | `operation_options::max_depth` was exceeded |
| `cycle` | A shared/raw pointer graph referenced itself during save |
| `invalid_registry` | Used a registry before freezing it, or mutated it after |

`path()` builds up as `$`, then `.field_name` for each reflected member and
`[index]` for each sequence element — e.g. `$.orders[2].total`.

## Limits

Every entry point accepts `operation_options`:

```cpp
serialization::serializer codec(serialization::operation_options{
    .max_depth        = 64,               // default 256
    .max_elements     = 10'000,           // default 1'000'000, per sequence
    .max_string_bytes = 1024 * 1024,      // default 16 MiB
});
```

These exist to make the serializer safe to point at untrusted input: a
maliciously deep or huge document throws `depth_limit`/`size_mismatch`
instead of exhausting memory or the call stack.

## Extending the Library

**A new archive.** Specialize `serialization::archive_traits<YourType>` with
the `write_*`/`read_*` members the `OutputArchive`/`InputArchive` concepts
require (`archive/traits.h`). An archive can implement only one direction —
see `Testing/Cxx/TestProtocol.cpp`'s `external::Writer` for a minimal
output-only example used purely to prove the contract.

**A new container-like type.** Specialize `serialization::type_codec<T>`
(`codecs/type_codec.h`) with `save`/`load` methods that recurse through the
supplied context (`ctx.save`/`ctx.load`/`ctx.save_element`/...). This is how
`std::optional`, `std::variant`, and the smart-pointer support in
`codecs/standard_types.h` are themselves implemented.

**Metadata from an AST tool instead of macros.** Specialize
`serialization::generated::metadata<T>` with a `properties()` returning the
same kind of tuple `SERIALIZATION_MACRO` generates, then select it with
`serialization::basic_serializer<serialization::ast_metadata>` instead of the
default `serializer` alias (which uses `macro_metadata`). Macro- and
AST-provided metadata for the same type are wire-compatible.

**A durable type ID and schema version.** Specialize
`serialization::record_info<T>` (default: empty ID, version `0`, meaning "no
check") to have mismatches rejected with `unknown_type`/`unsupported_version`
instead of silently misreading a record.

## Project Layout

```
include/            Public headers (no "serialization/" prefix — this is the include root)
  serializer.h       Umbrella include: core serializer + built-in codecs
  core/              context, serializer, error_code/serialization_error
  codecs/            type_codec specializations + native scalar casting
  archive/           archive_traits contract + narrowing-safe numeric helpers
  adapters/          json.h, xml.h, binary.h
  metadata/          SERIALIZATION_MACRO family, macro/AST metadata providers
  version.h/.cpp     serialization::version() — the only compiled source in the library
Testing/Cxx/         GoogleTest suite (SerializationCxxTests): one binary covering
                     all three backends plus a larger FpML-shaped example
ThirdParty/          Vendored nlohmann/json and pugixml; Logging as a git submodule
Docs/                Design notes
```

## Contributing

1. Fork the repository and create a feature branch.
2. Keep changes buildable on C++20 and run `ctest` before opening a PR.
3. New behavior needs a test in `Testing/Cxx/`.
4. Run `lintrunner` (config in `.lintrunner.toml`) — CI enforces the same
   clang-format/cmake-format checks.

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

**KhwarizmiAnalytix**

- GitHub: [@KhwarizmiAnalytix](https://github.com/KhwarizmiAnalytix)
- Repository: [Serialization](https://github.com/KhwarizmiAnalytix/Serialization)

## Acknowledgments

- Built on [nlohmann/json](https://github.com/nlohmann/json) for JSON support
- Uses [pugixml](https://pugixml.org/) for XML serialization
- Uses C++20 concepts for compile-time type checking
