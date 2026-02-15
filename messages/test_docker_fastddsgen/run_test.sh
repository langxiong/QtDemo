#!/bin/bash
# 最小测试：Docker fastddsgen 生成 IDL
# 用法: ./run_test.sh [docker_image]
# 例:   ./run_test.sh fastddsgen-arch

set -e
IMAGE="${1:-fastddsgen-arch}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
OUT_DIR="$SCRIPT_DIR/generated"

mkdir -p "$OUT_DIR"
rm -rf "${OUT_DIR};C" 2>/dev/null || true
WORK_MOUNT="$(pwd)"
# Docker on Windows 需要 Windows 路径 (C:\...)，否则挂载失败
if command -v cygpath &>/dev/null; then
  WORK_MOUNT=$(cygpath -w "$WORK_MOUNT")
fi

echo "=== Docker fastddsgen 测试 ==="
echo "镜像:    $IMAGE"
echo "挂载:    -v $WORK_MOUNT:/work"
echo "IDL:     $WORK_MOUNT/Test.idl"
echo "输出:    $WORK_MOUNT/generated"
echo ""

echo ">>> [1/3] 检查挂载后容器内 /work 内容"
docker run --rm -v "$WORK_MOUNT:/work" --entrypoint sh "$IMAGE" -c "ls -la /work && echo '---' && (ls -la /work/generated 2>/dev/null || echo '/work/generated 不存在')"
echo ""

echo ">>> [2/3] 执行 fastddsgen (先输出到容器内 /tmp/out 再复制到挂载目录)"
docker run --rm \
  -v "$WORK_MOUNT:/work" \
  --entrypoint sh \
  "$IMAGE" \
  -c "mkdir -p /tmp/out && /opt/fastddsgen/scripts/fastddsgen -ppDisable -d /tmp/out -replace /work/Test.idl && cp -v /tmp/out/* /work/generated/"
echo ""

echo ">>> [3/3] 检查生成文件"
for f in Test.hpp TestPubSubTypes.cxx TestPubSubTypes.hpp; do
  if [ -f "$OUT_DIR/$f" ]; then
    echo "  OK: $f"
  else
    echo "  缺失: $f"
    exit 1
  fi
done

echo ""
echo "=== 测试通过 ==="
ls -la "$OUT_DIR"
