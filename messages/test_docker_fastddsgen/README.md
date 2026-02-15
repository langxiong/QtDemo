# Docker fastddsgen 最小测试

验证 Docker 镜像能否正确生成 IDL 到宿主机。

## 依赖

- Docker
- 已构建的 fastddsgen 镜像，例如 `fastddsgen-arch`

## 运行 (Git Bash)

```bash
cd demo/messages/test_docker_fastddsgen
chmod +x run_test.sh
./run_test.sh fastddsgen-arch
```

## 运行 (cmd / PowerShell)

```cmd
cd demo\messages\test_docker_fastddsgen
run_test.bat fastddsgen-arch
```

## 预期输出

- `generated/Test.hpp`
- `generated/TestPubSubTypes.cxx`
- `generated/TestPubSubTypes.hpp`

全部存在则测试通过。若失败，可据此调整 Docker 镜像或挂载参数。
