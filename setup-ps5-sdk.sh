#!/usr/bin/env bash
# One-time host setup for building PS5 payloads on this WSL machine.
# Run from your own WSL terminal so sudo can prompt for your password:
#     bash setup-ps5-sdk.sh
# Safe to re-run: each step is skipped if already done.
set -uo pipefail

fail() { echo "!! FAILED at: $1" >&2; exit 1; }

echo "==> [1/5] build tools (cmake, git, unzip, wget)"
sudo apt-get update -qq || fail "apt-get update"
sudo apt-get install -y cmake git unzip wget || fail "install build tools"

echo "==> [2/5] clang-18 + lld-18 (via apt.llvm.org)"
if ! command -v clang-18 >/dev/null 2>&1; then
    wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh || fail "download llvm.sh"
    sudo bash /tmp/llvm.sh 18 || fail "llvm.sh 18"
fi
sudo apt-get install -y lld-18 || fail "install lld-18"
echo "    clang-18: $(clang-18 --version | head -1)"

echo "==> [3/5] PS5 payload SDK -> /opt/ps5-payload-sdk"
if [ ! -d /opt/ps5-payload-sdk ]; then
    wget -q https://github.com/ps5-payload-dev/sdk/releases/latest/download/ps5-payload-sdk.zip \
        -O /tmp/ps5-payload-sdk.zip || fail "download SDK zip"
    sudo unzip -q -d /opt /tmp/ps5-payload-sdk.zip || fail "unzip SDK"
fi
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
[ -f "$PS5_PAYLOAD_SDK/toolchain/prospero.mk" ] || fail "prospero.mk not found - unexpected SDK layout"
echo "    SDK OK: $PS5_PAYLOAD_SDK"

echo "==> [4/5] libc++ for the SDK (if the installer ships libcxx.sh)"
if [ -f "$PS5_PAYLOAD_SDK/libcxx.sh" ] && [ ! -d "$PS5_PAYLOAD_SDK/target/include/c++" ]; then
    sudo -E bash "$PS5_PAYLOAD_SDK/libcxx.sh" || echo "    (libcxx.sh skipped - release may already bundle it)"
fi

echo "==> [5/5] SDL2 into the SDK (build + install)"
if ls "$PS5_PAYLOAD_SDK"/target/user/homebrew/lib/libSDL2* >/dev/null 2>&1; then
    echo "    SDL2 already in the SDK"
else
    # Build SDL for PS5 if not already built.
    if [ ! -f /tmp/ps5-sdl/build-scripts/build-ps5/libSDL2.a ]; then
        rm -rf /tmp/ps5-sdl
        git clone --depth 1 -b release-2.30.x-ps5 \
            https://github.com/ps5-payload-dev/SDL.git /tmp/ps5-sdl || fail "clone SDL"
        source "$PS5_PAYLOAD_SDK/toolchain/prospero.sh"
        # The repo's ps5-payload-sdk.sh also builds a test app that fails on
        # find_package(SDL2); we only need the library, so configure+build it
        # directly (skip the test app).
        cmake -DCMAKE_BUILD_TYPE=Release -DSDL_OPENGL=YES -DSDL_LOADSO=YES \
              -S /tmp/ps5-sdl -B /tmp/ps5-sdl/build-scripts/build-ps5 || fail "cmake configure SDL"
        make -C /tmp/ps5-sdl/build-scripts/build-ps5 -j"$(nproc)" || fail "make SDL"
    fi
    # Install into the SDK sysroot. The install prefix baked into the build
    # is /user/homebrew, so DESTDIR=<sysroot> lands the files at
    # <sysroot>/user/homebrew/{lib,include}, where the prospero wrappers look.
    sudo env DESTDIR="$PS5_PAYLOAD_SDK/target" \
        make -C /tmp/ps5-sdl/build-scripts/build-ps5 install || fail "install SDL into SDK"
fi

echo
echo "ALL DONE. SDL2 libs:"
ls "$PS5_PAYLOAD_SDK"/target/user/homebrew/lib/libSDL2* 2>/dev/null || echo "  (none - check step 5 output)"
echo
echo "Persist the env var for future shells:"
echo "    echo 'export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk' >> ~/.bashrc"
