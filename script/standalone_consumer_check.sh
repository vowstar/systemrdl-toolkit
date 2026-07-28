#!/bin/sh
# Build a throwaway project against the installed SystemRDL package.
#
# The library can build and pass its own tests while being unusable by anyone
# else: the generic CMake target lived only in the build tree, and a public
# header was missing from the install set. Neither shows up in the normal test
# run, because the normal test run never leaves the build tree.
#
# Usage: script/standalone_consumer_check.sh [build-dir]

set -eu

BUILD_DIR="${1:-build}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "[INFO] installing to $WORK/prefix"
cmake --install "$ROOT/$BUILD_DIR" --prefix "$WORK/prefix" > "$WORK/install.log" 2>&1 || {
    echo "[FAIL] install failed"
    tail -20 "$WORK/install.log"
    exit 1
}

echo "[INFO] headers installed:"
find "$WORK/prefix" -path '*/include/systemrdl/*' -name '*.h' -exec basename {} \; | sed 's/^/       /'

mkdir -p "$WORK/consumer"
cat > "$WORK/consumer/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(systemrdl_consumer CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SystemRDL REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE SystemRDL::systemrdl)
CMAKE

cat > "$WORK/consumer/main.cpp" <<'CPP'
#include <systemrdl_api.h>
#include <iostream>

int main()
{
    const auto result = systemrdl::elaborate_simplified(
        "addrmap top { reg { field { sw=rw; hw=r; } f[7:0] = 8'hFF; } r0 @ 0x0; };");
    if (!result.ok()) {
        std::cerr << "elaboration failed: " << result.error() << "\n";
        return 1;
    }
    if (result.value().find("0xff") == std::string::npos) {
        std::cerr << "expected a reset of 0xff in the model\n";
        return 1;
    }
    std::cout << "consumer elaborated a sized literal correctly\n";
    return 0;
}
CPP

echo "[INFO] configuring consumer"
cmake -S "$WORK/consumer" -B "$WORK/consumer/build" \
      -DCMAKE_PREFIX_PATH="$WORK/prefix" > "$WORK/configure.log" 2>&1 || {
    echo "[FAIL] consumer configure failed"
    tail -20 "$WORK/configure.log"
    exit 1
}

echo "[INFO] building consumer"
cmake --build "$WORK/consumer/build" > "$WORK/build.log" 2>&1 || {
    echo "[FAIL] consumer build failed"
    tail -20 "$WORK/build.log"
    exit 1
}

echo "[INFO] running consumer"
"$WORK/consumer/build/consumer" || {
    echo "[FAIL] consumer ran but did not produce the expected model"
    exit 1
}

echo "[OK] the installed package is usable by an outside project"
