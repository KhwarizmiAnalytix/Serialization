cmake_minimum_required(VERSION 3.15)

project(Serialization
    VERSION 1.0.0
    LANGUAGES CXX
)

# Add cmake modules directory to the module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include build speed optimization (ccache, sccache, buildcache)
include(cache)

# Include clang-tidy static analysis configuration
include(clang_tidy)

# Include code coverage instrumentation configuration (SERIALIZATION_ENABLE_COVERAGE)
include(coverage)

# Include spell-checking configuration (SERIALIZATION_ENABLE_SPELL)
include(spell)

# Enable CTest for testing
enable_testing()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add nlohmann/json as a header-only INTERFACE library
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/nlohmann_json/include
)

# Add pugixml library
add_library(pugixml STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/pugixml/pugixml.cpp
)
target_include_directories(pugixml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/pugixml
)

# Automatically discover all source files in include/ directory
file(GLOB_RECURSE SERIALIZATION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)

# Exclude Testing directory files from the library sources
list(FILTER SERIALIZATION_SOURCES EXCLUDE REGEX ".*/Testing/.*")

# Create shared library version
add_library(Serialization SHARED
    ${SERIALIZATION_SOURCES}
)

# Set output name for shared library and enable automatic symbol export on Windows
#
# LIBRARY_OUTPUT_DIRECTORY (governs the macOS/Linux .so/.dylib) is placed
# under lib/ -- matching the coverage-tool convention (its find_library()
# hardcodes a search under <build_dir>/lib on macOS/Linux, <build_dir>/bin
# on Windows; see coverage.toml at the repo root). This is a separate
# directory from RUNTIME_OUTPUT_DIRECTORY (deliberately left unchanged
# below, still colocated with the test executable for Windows .dll lookup)
# -- moving the .so/.dylib is safe at runtime because CMake's default
# build-tree RPATH points directly at the actual build location of each
# linked library, wherever that is.
set_target_properties(Serialization PROPERTIES
    OUTPUT_NAME Serialization
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

# Enable RTTI (required for typeid and dynamic_cast in serialization)
if(MSVC)
    target_compile_options(Serialization PRIVATE /GR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
else()
    target_compile_options(Serialization PRIVATE -frtti)
endif()

# Include directories for Serialization library
target_include_directories(Serialization
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
)



# Link nlohmann/json and pugixml to Serialization library
target_link_libraries(Serialization
    PUBLIC
        nlohmann_json
        pugixml
)

# Add the Testing/Cxx subdirectory to build test executables
add_subdirectory(include/Testing/Cxx)
