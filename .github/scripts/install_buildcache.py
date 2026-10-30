"""Build a pinned buildcache on the runner instead of relying on package aliases."""

import argparse
import os
import shutil
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    source, build = prefix / "source", prefix / "build"
    subprocess.run(
        [
            "git",
            "clone",
            "--depth",
            "1",
            "--branch",
            "v0.33.1",
            "--recurse-submodules",
            "https://gitlab.com/bits-n-bites/buildcache.git",
            str(source),
        ],
        check=True,
    )
    # buildcache's src/base/env_utils.hpp uses int64_t without including
    # <cstdint>, relying on a transitive include that newer standard library
    # headers (e.g. recent libstdc++/libc++) no longer provide. Unfixed as of
    # the latest upstream tag (v0.33.2), so patch it in rather than bump the
    # pin: https://gitlab.com/bits-n-bites/buildcache/-/blob/v0.33.1/src/base/env_utils.hpp
    env_utils_hpp = source / "src" / "base" / "env_utils.hpp"
    contents = env_utils_hpp.read_text(encoding="utf-8")
    if "#include <cstdint>" not in contents:
        env_utils_hpp.write_text(
            contents.replace("#include <string>", "#include <cstdint>\n#include <string>", 1),
            encoding="utf-8",
        )
    subprocess.run(
        [
            "cmake",
            "-S",
            str(source / "src"),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            # CI uses local storage; avoid runner-specific OpenSSL DLLs.
            "-DDISABLE_SSL=ON",
            f"-DCMAKE_INSTALL_PREFIX={prefix}",
            # buildcache vendors third_party/mingw-std-threads, whose
            # cmake_minimum_required predates 3.5; modern CMake refuses to
            # configure without an explicit minimum policy version.
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        ],
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(build), "--target", "buildcache", "--parallel", "4"],
        check=True,
    )
    executable = "buildcache.exe" if os.name == "nt" else "buildcache"
    binaries = [path for path in build.rglob(executable) if path.is_file()]
    if not binaries:
        raise RuntimeError(
            "buildcache built without producing the requested executable"
        )
    destination = prefix / "bin"
    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binaries[0], destination / executable)
    for pattern in ("*.dll", "*.dylib", "*.so*"):
        for library in build.rglob(pattern):
            if library.is_file():
                shutil.copy2(library, destination / library.name)
    subprocess.run([str(destination / executable), "--version"], check=True)
    with Path(os.environ["GITHUB_PATH"]).open("a", encoding="utf-8") as output:
        output.write(str(destination) + "\n")


if __name__ == "__main__":
    main()
