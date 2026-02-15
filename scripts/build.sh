#!/usr/bin/env bash
# Build script for medical_robot_control_demo
# Usage: ./build.sh [--debug|--release] [build_dir]
#   --debug:   CMAKE_BUILD_TYPE=Debug, build dir=build-debug, install=dist-debug
#   --release: CMAKE_BUILD_TYPE=Release, build dir=build, install=dist (default)
#
# Environment (override as needed):
#   QT_PREFIX       - Qt6 install prefix (e.g. ~/Qt/6.10.2/msvc2022_64)
#   POCO_PREFIX     - Poco install prefix (e.g. ~/Opt/Poco)
#   FASTDDS_ROOT    - Fast DDS install prefix (e.g. ~/opt/fastdds)
#   FASTDDSGEN_IMG  - Docker image for fastddsgen (e.g. fastddsgen-arch)

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$DEMO_DIR"

# Load local config if present
[[ -f "$SCRIPT_DIR/config.local" ]] && source "$SCRIPT_DIR/config.local"

# Parse mode
MODE=release
BUILD_DIR="build"
if [[ "${1:-}" == "--debug" ]]; then
  MODE=debug
  BUILD_DIR="build-debug"
  shift
elif [[ "${1:-}" == "--release" ]]; then
  shift
fi
[[ -n "${1:-}" ]] && BUILD_DIR="$1"

# Windows path conversion for Docker/CMake
to_win_path() {
  if command -v cygpath &>/dev/null; then
    cygpath -w "$1"
  else
    echo "$1"
  fi
}

# Defaults (edit for your machine)
QT_PREFIX="${QT_PREFIX:-}"
POCO_PREFIX="${POCO_PREFIX:-}"
FASTDDS_ROOT="${FASTDDS_ROOT:-}"
FASTDDSGEN_IMG="${FASTDDSGEN_IMG:-fastddsgen-arch}"

# Build CMAKE_PREFIX_PATH
CMAKE_PREFIX_PATH=""
[[ -n "$QT_PREFIX" ]] && CMAKE_PREFIX_PATH="$QT_PREFIX"
[[ -n "$POCO_PREFIX" ]] && CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:+$CMAKE_PREFIX_PATH;}${POCO_PREFIX}"
[[ -n "$FASTDDS_ROOT" ]] && CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:+$CMAKE_PREFIX_PATH;}${FASTDDS_ROOT}"

# Build args
CMAKE_ARGS=(-B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH")
if [[ "$(uname -s)" == *"MSYS"* ]] || [[ "$(uname -s)" == *"MINGW"* ]]; then
  CMAKE_ARGS+=(-G "Visual Studio 17 2022" -A x64)
  if [[ "$MODE" == "debug" ]]; then
    CMAKE_ARGS+=(-DCMAKE_INSTALL_PREFIX="$(to_win_path "$DEMO_DIR/dist-debug")")
    CONFIG=Debug
  else
    CMAKE_ARGS+=(-DCMAKE_INSTALL_PREFIX="$(to_win_path "$DEMO_DIR/dist")")
    CONFIG=Release
  fi
else
  if [[ "$MODE" == "debug" ]]; then
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="$DEMO_DIR/dist-debug")
    CONFIG=Debug
  else
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$DEMO_DIR/dist")
    CONFIG=Release
  fi
fi

[[ -n "$POCO_PREFIX" ]] && CMAKE_ARGS+=(-DMRCD_POCO_PREFIX="$POCO_PREFIX")
[[ -n "$FASTDDS_ROOT" ]] && CMAKE_ARGS+=(-DMRCD_FASTDDS_ROOT="$FASTDDS_ROOT" -DMRCD_FASTDDSGEN_DOCKER_IMAGE="$FASTDDSGEN_IMG")
if [[ -n "$QT_PREFIX" ]] && [[ -f "$QT_PREFIX/bin/windeployqt.exe" ]] 2>/dev/null; then
  CMAKE_ARGS+=(-DMRCD_WINDEPLOYQT="$QT_PREFIX/bin/windeployqt.exe")
fi

echo "=== Build ($MODE) ==="
echo "  Build dir: $BUILD_DIR"
echo "  Prefix:    $CMAKE_PREFIX_PATH"
echo ""
cmake "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR" --config "$CONFIG"
echo ""
echo "=== Build done ==="
