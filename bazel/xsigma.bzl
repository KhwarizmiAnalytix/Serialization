# =============================================================================
# Serialization — shared Bazel compile/link helpers (standalone)
# =============================================================================
# Copy of XSigma's bazel/xsigma.bzl (same copy vendored by Parallel/
# LinearAlgebra/Logging), trimmed to what this repo uses.
# =============================================================================

def xsigma_copts(cxx_std = "c++20", cstdlib_include = False):
    """Returns common compiler options for Serialization targets.

    Args:
        cxx_std: C++ standard to use (default: c++20, matches CMake's
                 target_compile_features(SerializationCore INTERFACE cxx_std_20)).
        cstdlib_include: Whether to force-include <cstdlib> on non-Windows.
                 Serialization's own CMakeLists.txt does not do this (unlike
                 Parallel/LinearAlgebra), so callers here default it to False.
    """
    return select({
        "@platforms//os:windows": [
            "/std:" + cxx_std,
            "/Zc:__cplusplus",  # expose correct __cplusplus value (mirrors CMake /Zc:__cplusplus)
            "/EHsc",            # structured exception handling
            "/bigobj",          # large object files (mirrors CMake /bigobj on MSVC test target)
            "/utf-8",           # UTF-8 source/output encoding
            "/wd4244",          # narrowing conversion
            "/wd4267",          # size_t -> int conversion
            "/wd4715",          # not all control paths return
            "/wd4018",          # signed/unsigned comparison
        ],
        "//conditions:default": [
            "-std=" + cxx_std,
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        ] + (["-include", "cstdlib"] if cstdlib_include else []),
    })

def xsigma_defines():
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

def xsigma_linkopts():
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
