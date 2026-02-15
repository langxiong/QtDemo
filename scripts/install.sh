#!/usr/bin/env bash
# Install script for medical_robot_control_demo
# Usage: ./install.sh [--debug|--release] [build_dir]
#   --debug:   install from build-debug, config=Debug, output=dist-debug
#   --release: install from build, config=Release, output=dist (default)

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$DEMO_DIR"

MODE=release
BUILD_DIR="build"
CONFIG=Release
if [[ "${1:-}" == "--debug" ]]; then
  MODE=debug
  BUILD_DIR="build-debug"
  CONFIG=Debug
  shift
elif [[ "${1:-}" == "--release" ]]; then
  shift
fi
[[ -n "${1:-}" ]] && BUILD_DIR="$1"

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "Error: Build dir '$BUILD_DIR' not found. Run ./scripts/build.sh first."
  exit 1
fi

echo "=== Install ($MODE) ==="
echo "  From: $BUILD_DIR"
echo "  Config: $CONFIG"
echo ""
cmake --install "$BUILD_DIR" --config "$CONFIG"
echo ""
echo "=== Install done ==="
