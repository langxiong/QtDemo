# Build Scripts

辅助构建与安装的 Bash 脚本，支持 Debug / Release 模式。

## 前置条件

- CMake 3.16+
- Qt6、Poco、Fast DDS 已安装（启用 DDS 时）
- Windows: Visual Studio 2022 (msvc)
- Linux: gcc/clang, Qt6, Poco 包

## 环境变量

在运行脚本前可设置（或复制为 `config.local` 后 `source`）：

```bash
export QT_PREFIX="~/Qt/6.10.2/msvc2022_64"       # Qt6 安装目录
export POCO_PREFIX="~/Opt/Poco"                  # Poco 安装目录
export FASTDDS_ROOT="~/opt/fastdds"              # Fast DDS 安装目录
export FASTDDSGEN_IMG="fastddsgen-arch"          # fastddsgen Docker 镜像
```

## 用法

```bash
# Release 构建（默认）
./scripts/build.sh
./scripts/install.sh

# 显式指定 Release
./scripts/build.sh --release
./scripts/install.sh --release

# Debug 构建
./scripts/build.sh --debug
./scripts/install.sh --debug

# 指定构建目录
./scripts/build.sh --release my-build
./scripts/install.sh --release my-build
```

## 输出目录

| 模式    | 构建目录    | 安装目录   |
|---------|-------------|------------|
| Release | `build`     | `dist`     |
| Debug   | `build-debug` | `dist-debug` |
