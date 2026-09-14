# Testing subdirectory
# Note: Tests depend on external Serialization modules that are not present in this repository
# You may need to adjust paths or disable testing if dependencies are not available

# Check if we can build tests (requires external dependencies)
set(CAN_BUILD_TESTS OFF)

# Uncomment this if you have the required Serialization test infrastructure:
# set(CAN_BUILD_TESTS ON)

if(CAN_BUILD_TESTS)
    add_subdirectory(Cxx)
else()
    message(STATUS "Skipping tests - external Serialization dependencies not found")
    message(STATUS "Tests require: serializationTest.h, TestingDataSerializer.h, and other Serialization modules")
endif()
