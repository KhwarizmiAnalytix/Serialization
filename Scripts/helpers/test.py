"""
Test Execution Helper Module

Handles ctest invocation for Scripts/setup.py.

Serialization's CMakeLists.txt has no sanitizer or Valgrind CMake option
(unlike some sibling repos), so this module is intentionally limited to
plain ctest invocation.
"""

import subprocess


def run_ctest(
    builder: str,
    build_enum: str,
    system: str,
    verbosity: str,
    shell_flag: bool,
) -> int:
    """
    Run tests using ctest.

    Args:
        builder: Build system (ninja, xcodebuild, cmake)
        build_enum: Build type (Release, Debug, RelWithDebInfo)
        system: Operating system (Linux, Darwin, Windows)
        verbosity: Verbosity flag
        shell_flag: Whether to use shell execution

    Returns:
        Exit code (0 for success, non-zero for failure)
    """
    try:
        ctest_cmd = ["ctest"]

        if system == "Windows" and builder != "ninja":
            ctest_cmd.extend(["-C", build_enum])
        if builder == "xcodebuild":
            ctest_cmd.extend(["-C", build_enum])
        if verbosity:
            ctest_cmd.append(verbosity)

        return subprocess.check_call(ctest_cmd, stderr=subprocess.STDOUT, shell=shell_flag)

    except subprocess.CalledProcessError:
        return 1
    except Exception:
        return 1
