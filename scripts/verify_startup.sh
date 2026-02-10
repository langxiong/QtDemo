#!/usr/bin/env sh
set -eu

# Build and startup handshake verification (POSIX shell).
# Builds the project and runs controller_app --verify-startup to confirm
# controller_app can start algo_worker and complete the startup handshake.
#
# Usage (run from demo/ directory):
#   ./scripts/verify_startup.sh
#   ./scripts/verify_startup.sh build-debug
#
# Exit code: 0 if build and handshake succeed; non-zero on build failure,
# launch failure, or handshake timeout.

BUILD_DIR="${1:-build-debug}"

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
DEMO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
CONFIG_DIR="$DEMO_ROOT/config"

BIN_DIR="$DEMO_ROOT/$BUILD_DIR/bin"
BIN_DEBUG="$DEMO_ROOT/$BUILD_DIR/bin/Debug"

if [ -d "$BIN_DEBUG" ]; then
  RUN_DIR="$BIN_DEBUG"
elif [ -d "$BIN_DIR" ]; then
  RUN_DIR="$BIN_DIR"
else
  printf '%s\n' "Build output not found. Run from demo/ and build first, or pass build dir." >&2
  exit 1
fi

printf 'Building: %s\n' "$BUILD_DIR"
(
  cd "$DEMO_ROOT"
  cmake --build "$BUILD_DIR" --config Debug
)

# Copy config so controller_app finds controller_app.ini next to exe
if [ -f "$CONFIG_DIR/controller_app.ini" ]; then
  cp "$CONFIG_DIR/controller_app.ini" "$RUN_DIR/"
fi
if [ -f "$CONFIG_DIR/algo_worker.ini" ]; then
  cp "$CONFIG_DIR/algo_worker.ini" "$RUN_DIR/"
fi

printf 'Running controller_app --verify-startup from %s\n' "$RUN_DIR"
(
  cd "$RUN_DIR"
  ./controller_app --verify-startup
)

#!/usr/bin/env bash

# Build and startup handshake verification (POSIX shell version).
# Builds the project and runs controller_app --verify-startup to confirm
# controller_app can start algo_worker and complete the handshake.
#
# Usage (run from demo/ directory):
#   ./scripts/verify_startup.sh
#   ./scripts/verify_startup.sh build-debug
#
# Exit code: 0 if build and handshake succeed; non-zero on build failure,
# launch failure, or handshake timeout.

set -euo pipefail

BUILD_DIR="${1:-build-debug}"
DEMO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_DIR="${DEMO_ROOT}/config"

BIN_DIR="${DEMO_ROOT}/${BUILD_DIR}/bin"
BIN_DEBUG="${DEMO_ROOT}/${BUILD_DIR}/bin/Debug"

if [[ -x "${BIN_DEBUG}/controller_app" || -x "${BIN_DEBUG}/controller_app.exe" ]]; then
  RUN_DIR="${BIN_DEBUG}"
elif [[ -x "${BIN_DIR}/controller_app" || -x "${BIN_DIR}/controller_app.exe" ]]; then
  RUN_DIR="${BIN_DIR}"
else
  echo "Build output not found. Run from demo/ and build first, or pass build dir." >&2
  exit 1
fi

cd "${DEMO_ROOT}"
echo "Building: ${BUILD_DIR}"
cmake --build "${BUILD_DIR}" || exit $?

# Copy config so controller_app finds controller_app.ini next to exe
cp -f "${CONFIG_DIR}/controller_app.ini" "${RUN_DIR}/" 2>/dev/null || true
cp -f "${CONFIG_DIR}/algo_worker.ini" "${RUN_DIR}/" 2>/dev/null || true

echo "Running controller_app --verify-startup from ${RUN_DIR}"
cd "${RUN_DIR}"
if [[ -x "./controller_app" ]]; then
  ./controller_app --verify-startup
else
  ./controller_app.exe --verify-startup
fi

