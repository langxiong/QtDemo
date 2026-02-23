#!/bin/bash
# One-click demo: backend + frontend
# Run from project root or demo directory

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$DEMO_DIR")"
FRONTEND="$DEMO_DIR/frontend"

# Find cef_host binary (try common build layouts)
CEF_HOST=""
for dir in "$PROJECT_ROOT/build_release/bin" \
           "$PROJECT_ROOT/build/bin" \
           "$PROJECT_ROOT/build_debug/bin" \
           "$DEMO_DIR/build/bin" \
           "$DEMO_DIR/build_release/bin"; do
  if [[ -x "$dir/cef_host" ]]; then
    CEF_HOST="$dir/cef_host"
    break
  fi
  if [[ -x "$dir/cef_host.exe" ]]; then
    CEF_HOST="$dir/cef_host.exe"
    break
  fi
done

if [[ -z "$CEF_HOST" ]]; then
  echo "Error: cef_host not found. Build the project first."
  echo "  Try: cmake -B build_release -S demo && cmake --build build_release"
  exit 1
fi

echo "[1/2] Starting cef_host (API-only backend)..."
"$CEF_HOST" &
CEF_PID=$!
sleep 2

echo "[2/2] Starting frontend dev server..."
cd "$FRONTEND"
if [[ ! -d node_modules ]]; then
  echo "Installing npm dependencies..."
  npm install
fi
npm run dev &
NPM_PID=$!
sleep 3

echo ""
echo "Demo launched. Open http://localhost:5173 in your browser."
echo "Verify: Start/Stop/Reset, readApp status, readStream charts, logs."
echo "Press Ctrl+C to stop both processes."
trap "kill $CEF_PID $NPM_PID 2>/dev/null; exit" INT TERM
wait $CEF_PID $NPM_PID 2>/dev/null || true
