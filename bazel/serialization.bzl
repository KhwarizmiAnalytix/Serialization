# =============================================================================
# Serialization — Bazel compile/link helpers
# =============================================================================
# Standalone helpers, including the flags CMakeLists.txt applies
# specifically to SerializationCore:
#
#   target_compile_features(SerializationCore INTERFACE cxx_std_20)
#   if(MSVC)
#       target_compile_options(SerializationCore INTERFACE /GR /Zc:__cplusplus)
#   else()
#       target_compile_options(SerializationCore INTERFACE -frtti)
#   endif()
#
# Bazel's `copts` is private to the compiling target (it is not propagated to
# dependents the way CMake's INTERFACE compile options are), so every library
# target in //BUILD.bazel calls serialization_copts() itself rather than
# relying on a single call against SerializationCore to reach consumers.
# =============================================================================

# C++ standard for Serialization — mirrors CMake's cxx_std_20 requirement.
SERIALIZATION_CXX_STD = "c++20"

def serialization_copts():
    return select({
        "@platforms//os:windows": [
            "/std:" + SERIALIZATION_CXX_STD,
            "/Zc:__cplusplus",  # expose correct __cplusplus value
            "/EHsc",  # structured exception handling
            "/bigobj",  # large object files
            "/utf-8",  # UTF-8 source/output encoding
            "/wd4244",  # narrowing conversion
            "/wd4267",  # size_t -> int conversion
            "/wd4715",  # not all control paths return
            "/wd4018",  # signed/unsigned comparison
        ],
        "//conditions:default": [
            "-std=" + SERIALIZATION_CXX_STD,
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        ],
    }) + select({
        "@platforms//os:windows": [
            "/GR",              # enable RTTI (mirrors CMake /GR)
            "/Zc:__cplusplus",  # already above; harmless to repeat per-target
        ],
        "//conditions:default": [
            "-frtti",  # enable RTTI (mirrors CMake -frtti)
        ],
    })

def serialization_defines():
    """Returns project-wide preprocessor defines (Windows CRT safety)."""
    return select({
        "@platforms//os:windows": [
            "_CRT_SECURE_NO_DEPRECATE",
            "_CRT_NONSTDC_NO_DEPRECATE",
            "_CRT_SECURE_NO_WARNINGS",
            "_SCL_SECURE_NO_DEPRECATE",
            "_SCL_SECURE_NO_WARNINGS",
        ],
        "//conditions:default": [],
    })

def serialization_linkopts():
    """Returns common linker options for Serialization targets."""
    return select({
        "@platforms//os:windows": [],
        "@platforms//os:macos": [
            "-undefined",
            "dynamic_lookup",
        ],
        "//conditions:default": [
            "-lpthread",
            "-ldl",
        ],
    })
