# ============================================================================= Quarisma Sanitizer
# Configuration Module
# =============================================================================
# Clang/GCC AddressSanitizer and UndefinedBehaviorSanitizer. Applied via
# directory compile/link options so Serialization, Logging, pugixml, and tests
# are all instrumented. MSVC is unsupported.
# =============================================================================

include_guard(GLOBAL)

option(SERIALIZATION_ENABLE_SANITIZER "Build Serialization with a Clang/GCC sanitizer" OFF)
mark_as_advanced(SERIALIZATION_ENABLE_SANITIZER)

set(SERIALIZATION_SANITIZER_TYPE "address" CACHE STRING
                                                 "Sanitizer: address, undefined, leak, thread"
)
set_property(CACHE SERIALIZATION_SANITIZER_TYPE PROPERTY STRINGS address undefined leak thread)
mark_as_advanced(SERIALIZATION_SANITIZER_TYPE)

if(NOT SERIALIZATION_ENABLE_SANITIZER)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    message(WARNING "Sanitizers require GCC or Clang — SERIALIZATION_ENABLE_SANITIZER ignored")
    return()
endif()

message(STATUS "Serialization: sanitizer enabled (${SERIALIZATION_SANITIZER_TYPE})")

add_compile_options(
    -O1 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls
    "-fsanitize=${SERIALIZATION_SANITIZER_TYPE}"
)
add_link_options("-fsanitize=${SERIALIZATION_SANITIZER_TYPE}")
