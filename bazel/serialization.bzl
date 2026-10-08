# =============================================================================
# Serialization — Bazel compile/link helpers
# =============================================================================
# Thin wrapper around xsigma.bzl's shared helpers, adding the flags
# CMakeLists.txt applies specifically to SerializationCore:
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

load("//bazel:xsigma.bzl", "xsigma_copts", "xsigma_defines", "xsigma_linkopts")

# C++ standard for Serialization — mirrors CMake's cxx_std_20 requirement.
SERIALIZATION_CXX_STD = "c++20"

def serialization_copts():
    return xsigma_copts(cxx_std = SERIALIZATION_CXX_STD) + select({
        "@platforms//os:windows": [
            "/GR",              # enable RTTI (mirrors CMake /GR)
            "/Zc:__cplusplus",  # already in xsigma_copts(); harmless to repeat per-target
        ],
        "//conditions:default": [
            "-frtti",  # enable RTTI (mirrors CMake -frtti)
        ],
    })

def serialization_defines():
    return xsigma_defines()

def serialization_linkopts():
    return xsigma_linkopts()
