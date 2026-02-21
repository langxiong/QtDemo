# CEF + React 集成说明

## 当前状态

- **cef_api**：已完成（DTO、ApplicationHandler、StreamHandler、APIInterface、观察者模式）
- **cef_host**：CEF 宿主，需设置 `MRCD_CEF_ROOT`；当前可运行 cef_api 校验
- **frontend**：Vite + React 工程，API 客户端与基本 UI（启停、Reset、状态、日志）

## 快速体验

```bash
# 1. 运行 cef_host（验证 cef_api）
cd demo/build/bin/Release
cef_host.exe

# 2. 前端（浏览器 mock 模式）
cd demo/frontend
npm install
npm run dev
# 打开 http://localhost:5173
```

## CEF SDK 安装（完整 CEF 宿主）

1. 下载 CEF 预编译包：https://cef-builds.spotifycdn.com/
   - 选择 Windows 64-bit，如 `cef_binary_xxx_windows64`
2. 解压到某目录，如 `C:/Opt/cef`
3. 在 CEF 根目录构建 libcef_dll_wrapper：
   ```cmd
   mkdir build && cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```
4. 配置本工程：`-DMRCD_CEF_ROOT=C:/Opt/cef`
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

**方式一：脚本（Windows）**

```cmd
demo\scripts\run-demo.bat
```

**方式二：手动**

```bash
# 终端 1：启动后端
demo/build/bin/Release/cef_host.exe

# 终端 2：启动前端
cd demo/frontend && npm install && npm run dev
# 浏览器打开 http://localhost:5173
```

**验证项**（cef_host API 校验 + 前端 mock 模式）：

1. 启停：点击 Start → 状态变为 running；Stop → stopped
2. Reset：点击 Reset → 状态变为 stopped
3. readApp：控制面板显示 ApplicationData（id, name, status）
4. readStream：图表区轮询显示 StreamData（payload, timestamp）
5. 日志：操作后显示对应 readStream 或状态变更日志

当 `MRCD_CEF_ROOT` 已配置并构建 libcef_dll_wrapper 后，cef_host 将加载 React（localhost:5173 或 file://），实现 window.api 端到端调用。

## 架构

```
React (frontend)  ──[window.api]──>  CEF 桥接  ──>  APIInterface  ──>  ApplicationHandler / StreamHandler
                                        (待实现)
```
