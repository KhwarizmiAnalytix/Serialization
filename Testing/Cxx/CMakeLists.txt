# Automatically discover all test files in this directory
file(GLOB TEST_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Build one combined test executable rather than one-per-file. This matches
# the coverage-tool convention of a single "<Library>CxxTests" binary per
# library (see coverage.toml at the repo root: modules = ["Serialization"]
# expects exactly one SerializationCxxTests executable to exist alongside
# libSerialization.*  -- coverage-tool's Clang/LLVM backend runs one test
# binary per module entry, so four separate executables would only let it
# measure whichever one binary it happened to match, undercounting the rest).
# Each *.cpp above only contains TEST()/TEST_F() cases (no its own main()),
# so they link cleanly into a single binary against one shared gtest_main.
set(TEST_NAME SerializationCxxTests)
add_executable(${TEST_NAME} ${TEST_FILES})

# Homebrew fmt (pulled in via Logging) adds -isystem /opt/homebrew/include,
# which can shadow FetchContent GoogleTest with a different gtest install.
# Keep our gtest headers first.
target_include_directories(${TEST_NAME}
    SYSTEM BEFORE PRIVATE
        "${googletest_SOURCE_DIR}/googletest/include"
)

# Link against the Serialization library and GTest
target_link_libraries(${TEST_NAME}
    PRIVATE
        Serialization
        gtest
        gtest_main
)

if(APPLE)
    # Ensure proper C++ standard library linking for Homebrew LLVM
    # Detect if using Homebrew LLVM and link to its libc++ library
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(COMPILER_PARENT "${COMPILER_DIR}" DIRECTORY)
    if(EXISTS "${COMPILER_PARENT}/lib/c++/libc++.dylib")
        # Using Homebrew LLVM - explicitly link to its libc++
        target_link_libraries(${TEST_NAME}
            PRIVATE
                "${COMPILER_PARENT}/lib/c++/libc++.dylib"
                "${COMPILER_PARENT}/lib/c++/libc++abi.dylib"
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        # Force linking with the C++ standard library
        target_link_options(${TEST_NAME} PRIVATE -stdlib=libc++)
    endif()
endif()

# Enable RTTI (required for dynamic_cast and typeid)
if(MSVC)
    target_compile_options(${TEST_NAME} PRIVATE /GR)
else()
    target_compile_options(${TEST_NAME} PRIVATE -frtti)
    # Ensure we use libc++ for both compilation and linking
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${TEST_NAME} PRIVATE -stdlib=libc++)
    endif()
endif()

# Set C++ standard and output directory for test executable
set_target_properties(${TEST_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)

# Discover and register GTest tests with CTest
include(GoogleTest)
gtest_discover_tests(${TEST_NAME}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)
