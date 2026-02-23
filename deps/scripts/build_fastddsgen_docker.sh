#!/bin/bash
# Build Fast-DDS-Gen Docker image for IDL code generation.
# Usage: ./build_fastddsgen_docker.sh [image_name]
# Default image: fastddsgen-mrcd
# Then use: cmake ... -DMRCD_FASTDDSGEN_DOCKER_IMAGE=fastddsgen-mrcd

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="$(dirname "$SCRIPT_DIR")"
IMAGE="${1:-fastddsgen-mrcd}"

echo "Building Fast-DDS-Gen Docker image: $IMAGE"
docker build -t "$IMAGE" -f "$DEPS_DIR/fastddsgen/Dockerfile" "$DEPS_DIR/fastddsgen"
echo "Done. Use with: -DMRCD_FASTDDSGEN_DOCKER_IMAGE=$IMAGE"
