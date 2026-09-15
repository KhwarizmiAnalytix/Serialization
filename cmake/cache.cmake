# =============================================================================
# Serialization Compiler Cache Configuration
# =============================================================================
# Enables configurable compiler caching (ccache, sccache, buildcache).
# Supports GCC, Clang, and MSVC on Linux, macOS, and Windows.
# =============================================================================

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Distributed compilation with Icecream
option(SERIALIZATION_ENABLE_ICECC "Use Icecream distributed compilation" OFF)
mark_as_advanced(SERIALIZATION_ENABLE_ICECC)

if(SERIALIZATION_ENABLE_ICECC)
    find_program(ICECC_EXECUTABLE icecc)
    if(ICECC_EXECUTABLE)
        set(CMAKE_C_COMPILER_LAUNCHER ${ICECC_EXECUTABLE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${ICECC_EXECUTABLE})
        message(STATUS "Using Icecream: ${ICECC_EXECUTABLE}")
    endif()
endif()

# Controls whether the selected compiler cache is enabled.
option(SERIALIZATION_ENABLE_CACHE "Enable compiler caching for faster builds" ON)
mark_as_advanced(SERIALIZATION_ENABLE_CACHE)

# Cache Type Configuration Selects which compiler cache to use: none, ccache, sccache, or buildcache
set(SERIALIZATION_CACHE_TYPE "none"
    CACHE STRING "Compiler cache type to use. Options: none, ccache, sccache, buildcache"
)
set_property(CACHE SERIALIZATION_CACHE_TYPE PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(SERIALIZATION_CACHE_TYPE)

# Validate cache type
if(NOT SERIALIZATION_CACHE_TYPE MATCHES "^(none|ccache|sccache|buildcache)$")
    message(
        FATAL_ERROR
            "Invalid SERIALIZATION_CACHE_TYPE: ${SERIALIZATION_CACHE_TYPE}. Valid options are: none, ccache, sccache, buildcache"
    )
endif()

if(NOT SERIALIZATION_ENABLE_CACHE)
    message(WARNING "Build speed cache optimization configuration complete")
    return()
endif()

message(STATUS "Configuring build speed optimizations with cache type: ${SERIALIZATION_CACHE_TYPE}")

# ============================================================================ Compiler Cache
# Configuration
# ============================================================================
#
# NOTE: Compiler caches are configured globally as compiler launchers because they need to intercept
# all compilation commands, including those for third-party dependencies. This is safe because
# caches only cache compilation results and don't affect the actual compilation flags or behavior.
set(SERIALIZATION_CACHE_PROGRAM "none")
if(SERIALIZATION_CACHE_TYPE STREQUAL "none")
    message(STATUS "No compiler cache selected")
else()
    # Preserve the per-backend cache variables so callers can supply a binary path.
    string(TOUPPER "${SERIALIZATION_CACHE_TYPE}" _serialization_cache_backend)
    set(_serialization_cache_variable "${_serialization_cache_backend}_PROGRAM")
    find_program(${_serialization_cache_variable} NAMES "${SERIALIZATION_CACHE_TYPE}")
    set(SERIALIZATION_CACHE_PROGRAM "${${_serialization_cache_variable}}")
    if(SERIALIZATION_CACHE_PROGRAM)
        message(STATUS "Found ${SERIALIZATION_CACHE_TYPE}: ${SERIALIZATION_CACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${SERIALIZATION_CACHE_PROGRAM}" CACHE STRING
                                                                             "C compiler launcher"
        )
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SERIALIZATION_CACHE_PROGRAM}"
            CACHE STRING "CXX compiler launcher"
        )
        message(STATUS "${SERIALIZATION_CACHE_TYPE} enabled for C/C++ compilation")
    else()
        message(WARNING "${SERIALIZATION_CACHE_TYPE} not found - compiler caching disabled")
    endif()
    unset(_serialization_cache_backend)
    unset(_serialization_cache_variable)
endif()

message("  -cache: ENABLED. program: ${SERIALIZATION_CACHE_PROGRAM}")
mark_as_advanced(SERIALIZATION_CACHE_PROGRAM)
