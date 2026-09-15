add_executable(SerializationCxxTests TestSerialization.cpp TestProtocol.cpp TestFpmlSerialization.cpp)
target_link_libraries(SerializationCxxTests PRIVATE
    Serialization::Serialization Serialization::Core Serialization::Json Serialization::Xml Serialization::Binary GTest::gtest_main)
if(SERIALIZATION_ENABLE_CLANGTIDY)
    quarisma_target_clang_tidy(SerializationCxxTests)
endif()
# coverage-tool's Clang backend looks for the test binary under bin/ (and
# bin/<Config> on multi-config generators) to pair with libSerialization.*
# — see coverage.toml.
set_target_properties(SerializationCxxTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
include(GoogleTest)
gtest_discover_tests(SerializationCxxTests DISCOVERY_MODE PRE_TEST)
