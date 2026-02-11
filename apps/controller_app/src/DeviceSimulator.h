#pragma once

#include "common/device/IDataAcquisition.h"
#include "common/device/IHardwareControl.h"

#include <QObject>

/**
 * 设备模拟 Model：目标位置/速度、当前位置/速度、运行状态；
 * 比例逼近逻辑，步进后发出 stateUpdated 供 View/Controller 连接。
 * 实现硬件控制接口与数据采集接口（预留扩展）。
 */
class DeviceSimulator : public QObject, public common::device::IHardwareControl, public common::device::IDataAcquisition {
  Q_OBJECT
public:
  explicit DeviceSimulator(QObject* parent = nullptr);

  void setTarget(double positionOrVelocity) override;
  void start() override;
  void stop() override;

  common::device::DeviceSnapshot latestSnapshot() const override;

  /** 每周期步进，由 QTimer 或 Controller 调用（主线程）。 */
  void step(double dtSeconds);

  bool isRunning() const { return _running; }
  double targetPosition() const { return _targetPosition; }
  double currentPosition() const { return _position; }
  double currentVelocity() const { return _velocity; }
  double currentSensorValue() const { return _sensorValue; }

signals:
  void stateUpdated(double position, double velocity, double sensorValue);
  void started();
  void stopped();

private:
  double _targetPosition{0.0};
  double _position{0.0};
  double _velocity{0.0};
  double _sensorValue{0.0};
  bool _running{false};
  mutable unsigned _rng{1}; // 简单 LCG 用于传感器噪声
};
