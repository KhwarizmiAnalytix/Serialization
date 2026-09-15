cmake_minimum_required(VERSION 3.20)
project(Serialization VERSION 2.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cache)
include(clang_tidy)
include(coverage)
include(sanitize)
include(spell)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SERIALIZATION_ENABLE_JSON "Build the nlohmann JSON adapter" ON)
option(SERIALIZATION_ENABLE_XML "Build the pugixml adapter" ON)
option(SERIALIZATION_ENABLE_BINARY "Build the structured binary adapter" ON)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_serialization_testing_default ON)
else()
    set(_serialization_testing_default OFF)
endif()
option(SERIALIZATION_BUILD_TESTING "Build new-design serialization tests" ${_serialization_testing_default})

add_library(SerializationCore INTERFACE)
add_library(Serialization::Core ALIAS SerializationCore)
set_target_properties(SerializationCore PROPERTIES EXPORT_NAME Core)
target_compile_features(SerializationCore INTERFACE cxx_std_20)
target_include_directories(SerializationCore INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
if(MSVC)
    target_compile_options(SerializationCore INTERFACE /GR /Zc:__cplusplus)
else()
    target_compile_options(SerializationCore INTERFACE -frtti)
endif()
set(_serialization_targets SerializationCore)

# SerializationCore/Json/Xml/Binary are header-only (INTERFACE): the actual
# serialization logic is templates instantiated directly into whatever
# consumes it, so it has no .cpp of its own and produces no compiled
# artifact. This small SHARED library exists so the project has one real,
# linkable entry point (a runtime version() query) — and, as a side effect,
# a libSerialization.* that coverage-tool's Clang/LLVM backend can pair with
# SerializationCxxTests (see coverage.toml; it requires a real library file
# per module and won't instrument a header-only target directly).
add_library(Serialization SHARED include/version.cpp)
add_library(Serialization::Serialization ALIAS Serialization)
target_link_libraries(Serialization PUBLIC SerializationCore)
target_compile_definitions(Serialization PRIVATE SERIALIZATION_VERSION_STRING="${PROJECT_VERSION}")
set_target_properties(Serialization PROPERTIES
    EXPORT_NAME Serialization
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
list(APPEND _serialization_targets Serialization)

if(SERIALIZATION_ENABLE_JSON)
    add_library(SerializationJson INTERFACE)
    add_library(Serialization::Json ALIAS SerializationJson)
    set_target_properties(SerializationJson PROPERTIES EXPORT_NAME Json)
    target_link_libraries(SerializationJson INTERFACE SerializationCore)
    target_include_directories(SerializationJson INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/nlohmann_json/include>)
    list(APPEND _serialization_targets SerializationJson)
    install(DIRECTORY ThirdParty/nlohmann_json/include/nlohmann DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES include/adapters/json.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adapters)
endif()
if(SERIALIZATION_ENABLE_XML)
    add_library(SerializationPugiXml STATIC ThirdParty/pugixml/pugixml.cpp)
    set_target_properties(SerializationPugiXml PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME PugiXml)
    target_include_directories(SerializationPugiXml PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/pugixml>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    add_library(SerializationXml INTERFACE)
    add_library(Serialization::Xml ALIAS SerializationXml)
    set_target_properties(SerializationXml PROPERTIES EXPORT_NAME Xml)
    target_link_libraries(SerializationXml INTERFACE SerializationCore SerializationPugiXml)
    list(APPEND _serialization_targets SerializationXml SerializationPugiXml)
    install(FILES ThirdParty/pugixml/pugixml.hpp ThirdParty/pugixml/pugiconfig.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES include/adapters/xml.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adapters)
endif()
if(SERIALIZATION_ENABLE_BINARY)
    add_library(SerializationBinary INTERFACE)
    add_library(Serialization::Binary ALIAS SerializationBinary)
    set_target_properties(SerializationBinary PROPERTIES EXPORT_NAME Binary)
    target_link_libraries(SerializationBinary INTERFACE SerializationCore)
    list(APPEND _serialization_targets SerializationBinary)
    install(FILES include/adapters/binary.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adapters)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "adapters" EXCLUDE)
install(TARGETS ${_serialization_targets} EXPORT SerializationTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(EXPORT SerializationTargets NAMESPACE Serialization:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Serialization)
configure_package_config_file(cmake/SerializationConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SerializationConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Serialization)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/SerializationConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SerializationConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SerializationConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Serialization)

if(SERIALIZATION_BUILD_TESTING)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(Testing/Cxx)
endif()
