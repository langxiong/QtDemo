# 本 Demo 与复杂系统经验的映射说明

本文档说明本演示程序中的设计如何对应到大型客户端或工业软件中的常见概念，便于面试或评审时关联「复杂系统经验」。

## 数据流：传感器 → Model → 信号 → View

- **本 Demo**：DeviceSimulator（模拟设备）每周期步进后发出 `stateUpdated`；Controller 将 signal 连接到 View 的 slot，更新标签与图表。数据单向流动：Model 为唯一数据源，View 仅通过 slot 接收更新。
- **大型系统对应**：可类比为「数据采集服务 / 设备驱动」产生数据 → 「领域模型 / 状态聚合」更新 → 通过「事件 / 消息总线」或进程内信号槽投递 → 「UI 模块 / 展示层」订阅并刷新。本 Demo 的 Qt 信号槽即进程内的轻量「事件通道」，无需引入独立消息中间件即可体现同一逻辑。

## 多进程 / 多线程与隔离

- **本 Demo**：controller_app 与 algo_worker 为多进程；SensorPipeline、ControlLoop、HeartbeatMonitor 等运行在独立 std::thread；DeviceSimulator 与 UI 运行在主线程，通过 QTimer 与 signal/slot 通信。
- **大型系统对应**：多进程对应「服务隔离」（如控制进程与算法进程分离，单进程崩溃不拖垮整体）；多线程对应「并发与职责分离」（采集、控制、通信、UI 各司其职）。本 Demo 的进程/线程划分即简化版的服务与并发模型。

## 接口抽象与可替换实现

- **本 Demo**：IHardwareControl、IDataAcquisition 为纯虚接口；DeviceSimulator 实现二者。Controller 依赖 Model（可注入接口类型），未来可替换为真实硬件或另一套仿真而不改 View 与 Controller 的交互方式。
- **大型系统对应**：接口对应「契约 / 端口」；可替换实现对应「测试替身、多厂商驱动、多数据源切换」。本 Demo 的接口设计即缩小版的「依赖倒置」与可扩展架构。

## 小结

| Demo 概念           | 复杂系统中常见对应       |
|---------------------|--------------------------|
| DeviceSimulator + signal | 数据采集 / 设备驱动 + 事件或消息 |
| DemoController 连接 Model 与 View | 应用层 / 编排层连接领域与 UI |
| IHardwareControl / IDataAcquisition | 硬件抽象层 / 数据端口接口 |
| 主线程 UI + QueuedConnection | UI 线程安全与跨线程事件投递 |

不要求本 Demo 实现完整事件总线或 RPC；上述映射用于概念对应与讲解，便于将实现经验迁移到更大系统。
