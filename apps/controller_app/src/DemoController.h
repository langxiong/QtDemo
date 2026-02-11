#pragma once

#include <QObject>

class DeviceSimulator;
class MainWindow;
class QTimer;

/**
 * Controller：创建 Model 与 View，用 QTimer 驱动 step()，
 * 连接 Model 的 signal 到 View 的 slot；连接 View 的按钮/滑块信号到 Model。
 * 不向 View 暴露 Model 指针。
 */
class DemoController : public QObject {
  Q_OBJECT
public:
  explicit DemoController(MainWindow* view, QObject* parent = nullptr);
  ~DemoController();

  /** 启动周期定时器（用于设备模拟步进）。调用前 View 与 Model 已创建并连接。 */
  void startSimulationTimer(int intervalMs);
  void stopSimulationTimer();

  DeviceSimulator* deviceSimulator() const { return _device; }

private slots:
  void onSimulationTimeout();
  void onStateUpdated(double position, double velocity, double sensorValue);
  void onStartClicked();
  void onStopClicked();
  void onTargetChanged(int value);
  void onDeviceStarted();
  void onDeviceStopped();

private:
  MainWindow* _view{nullptr};
  DeviceSimulator* _device{nullptr};
  QTimer* _simTimer{nullptr};
  qint64 _lastStepTimeMs{0};
};
