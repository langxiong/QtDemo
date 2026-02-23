# CEF + React 集成说明

## 当前状态

- **cef_api**：已完成（DTO、ApplicationHandler、StreamHandler、APIInterface、观察者模式）
- **cef_host**：CEF 宿主，需设置 `MRCD_CEF_ROOT`；当前可运行 cef_api 校验
- **frontend**：Vite + React 工程，API 客户端与基本 UI（启停、Reset、状态、日志）

## 快速体验

```bash
# 1. 运行 cef_host（验证 cef_api）
cd build_release/bin   # 或 build_debug/bin
cef_host.exe

# 2. 前端（浏览器 mock 模式）
cd demo/frontend
npm install
npm run dev
# 打开 http://localhost:5173
```

## CEF SDK 安装（完整 CEF 宿主）

### 方式一：从 CDN 自动获取（推荐）

CEF 预编译包**仅含 Release** 产物；libcef_dll_wrapper 随主工程构建，`build_debug` 产出 Debug wrapper，`build_release` 产出 Release wrapper，与 deps 的 Debug/Release 分离策略一致。

当 `MRCD_USE_DEPS_SUBMODULES=ON` 时，CMake 会在 deps bootstrap 阶段自动拉取 CEF（`MRCD_FETCH_CEF=ON`，可 `-DMRCD_FETCH_CEF=OFF` 关闭）。需 Python 3。

或手动运行：
```bash
python demo/scripts/fetch_cef.py
```

脚本会从 https://cef-builds.spotifycdn.com 下载最新 stable Windows 64-bit 包，解压到 `demo/prebuilt/cef/`。CMake 会自动检测并设置 `MRCD_CEF_ROOT`。无需单独构建 libcef_dll_wrapper，其随主工程 `add_subdirectory` 构建。

```bash
# 使用 preset 构建
cmake --preset build_release -S demo
cmake --build --preset build_release
```

### 方式二：手动下载

1. 下载 CEF 预编译包：https://cef-builds.spotifycdn.com/
   - 选择 Windows 64-bit，如 `cef_binary_xxx_windows64`
2. 解压到 `demo/prebuilt/cef/` 或任意目录
3. 配置本工程：`-DMRCD_CEF_ROOT=<解压后的 cef_binary_*_windows64 路径>`
4. libcef_dll_wrapper 随主工程构建，无需在 CEF 目录内单独构建

5. cef_host 将加载 http://localhost:5173（开发）或可改为 file:// 加载 frontend/dist

## 前端构建

```bash
cd demo/frontend
npm run build
# 产物在 frontend/dist/
```

## ControlLoop 集成（可选）

当 cef_api 与控制器（含 ControlLoop）集成时：

- 使用 `APIInterface::setOnApplicationRunningChange(std::function<void(bool)>)` 在应用启停时回调
- 在回调中启动/停止 ControlLoop，**务必在独立线程中运行 ControlLoop**，避免阻塞 CEF 主线程消息循环
- CEF 消息循环与 ControlLoop 线程/进程分离，互不冲突

## 一键启动与验证

**方式一：脚本**

```bash
./demo/scripts/run-demo.sh
```

**方式二：手动**

```bash
# 终端 1：启动 controller_app（IPC 服务端）
build_release/bin/controller_app.exe   # 或 build_debug/bin
# 可选：配置 config/controller_app.ini 中 [ipc] api_port=9123

# 终端 2：启动 cef_host（CEF + IPC 客户端）
set MRCD_CONTROLLER_IPC_ADDR=127.0.0.1:9123   # Windows
# export MRCD_CONTROLLER_IPC_ADDR=127.0.0.1:9123  # Linux (stub mode)
build_release/bin/cef_host.exe

# 终端 3：启动前端
cd demo/frontend && npm install && npm run dev
# 浏览器打开 http://localhost:5173
```

cef_host 使用原生 SetAsPopup 独立窗口，与 DuiLib 无耦合；DuiLib 宿主窗口属于 controller_app。

**验证项**（cef_host API 校验 + 前端 mock 模式）：

1. 启停：点击 Start → 状态变为 running；Stop → stopped
2. Reset：点击 Reset → 状态变为 stopped
3. readApp：控制面板显示 ApplicationData（id, name, status）
4. readStream：图表区轮询显示 StreamData（payload, timestamp）
5. 日志：操作后显示对应 readStream 或状态变更日志

当 `MRCD_CEF_ROOT` 已配置并构建 libcef_dll_wrapper 后，cef_host 将加载 React（localhost:5173 或 file://），实现 window.api 端到端调用。

## 架构

```
React (frontend)  ──[window.api]──>  CEF 桥接  ──>  APIInterface  ──[IPC/local]──>  controller_app / 本地 mock
                                                    │
                                                    └── IpcBackend (TCP 127.0.0.1:9123) 或 LocalBackend
```
