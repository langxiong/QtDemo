#!/bin/bash
# Minimal test: Docker fastddsgen IDL generation
# Usage: ./run_test.sh [docker_image]
# Example: ./run_test.sh fastddsgen-mrcd

set -e
IMAGE="${1:-fastddsgen-mrcd}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IDL_DIR="$SCRIPT_DIR"
OUT_DIR="$SCRIPT_DIR/generated"

mkdir -p "$OUT_DIR"

echo "=== Docker fastddsgen test ==="
echo "Image: $IMAGE"
echo "IDL:   $IDL_DIR/Test.idl"
echo "Output: $OUT_DIR"
echo ""

echo ">>> Running: docker run --rm -v IDL:/idl -v OUT:/out $IMAGE -ppDisable -d /out -replace /idl/Test.idl"
docker run --rm \
  -v "$IDL_DIR:/idl" \
  -v "$OUT_DIR:/out" \
  "$IMAGE" \
  -ppDisable -d /out -replace /idl/Test.idl

echo ""
echo ">>> Checking generated files"
MISSING=0
for f in Test.hpp TestPubSubTypes.cxx TestPubSubTypes.hpp; do
  if [[ -f "$OUT_DIR/$f" ]]; then
    echo "  OK: $f"
  else
    echo "  Missing: $f"
    MISSING=1
  fi
done

if [[ $MISSING -ne 0 ]]; then
  echo ""
  echo "Test failed: some files are missing"
  exit 1
fi

echo ""
echo "=== Test passed ==="
ls -la "$OUT_DIR"
exit 0
