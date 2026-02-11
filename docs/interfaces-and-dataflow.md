# 预留接口与数据流说明

## 硬件控制接口 / 数据采集接口

- **IHardwareControl**（`common/device/IHardwareControl.h`）：**硬件控制接口**的预留抽象。提供 `setTarget(double)`、`start()`、`stop()`。DeviceSimulator 与未来真实硬件适配器均可实现此接口。扩展方式：实现该接口并通过依赖注入注入到 Controller（或等效调用方），即可替换为真实设备而不改 View/Controller 的交互逻辑。
- **IDataAcquisition**（`common/device/IDataAcquisition.h`）：**数据采集接口**的预留抽象。提供 `latestSnapshot()` 返回 `DeviceSnapshot`（position, velocity, sensorValue, running）。DeviceSimulator 与现有 SensorPipeline 均可实现或适配此接口。扩展方式：实现该接口并注入到需要读取数据的模块，便于统一「最新状态」的获取方式与测试替身。

## 数据流与线程安全

- **DeviceSimulator → signal → Controller → View slot**  
  DeviceSimulator 运行在主线程，由 QTimer 周期调用 `step(dt)`；每步后发出 `stateUpdated(position, velocity, sensorValue)`。DemoController 将该 signal 连接到自己的 slot `onStateUpdated`，在其中调用 `MainWindow::updateState(...)` 更新标签与图表。因 Model 与 View 均在主线程，使用 **DirectConnection**（默认）即可，无跨线程访问。

- **主线程与 QueuedConnection**  
  若将来从工作线程（如 SensorPipeline、ControlLoop）向 View 推送更新，必须通过 Qt 信号槽且使用 **Qt::QueuedConnection**，由 Qt 事件循环将调用投递到主线程执行，避免在非 UI 线程直接操作 QWidget。示例：`connect(sender, &Sender::dataReady, view, &MainWindow::appendLog, Qt::QueuedConnection)`。

- **界面日志与 common::log**  
  当前界面日志由 Controller 在用户操作与 Model 状态变化时调用 `View::appendLog` 写入。若需将 `common::log` 的输出也显示到界面，需在 log 层增加可配置 sink（回调）；在回调中使用 `QMetaObject::invokeMethod(view, "appendLog", Qt::QueuedConnection, Q_ARG(QString, ...))` 将日志行投递到主线程，以保证非 UI 线程写日志时的线程安全。
