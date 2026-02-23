#pragma once

#include "common/device/IDataAcquisition.h"
#include "common/device/IHardwareControl.h"

#include <functional>

/**
 * 设备模拟 Model：目标位置/速度、当前位置/速度、运行状态；
 * 比例逼近逻辑，步进后通过回调通知。
 * 实现硬件控制接口与数据采集接口（预留扩展）。
 */
class DeviceSimulator : public common::device::IHardwareControl, public common::device::IDataAcquisition {
public:
  using StateCallback = std::function<void(double position, double velocity, double sensorValue)>;
  using StartedCallback = std::function<void()>;
  using StoppedCallback = std::function<void()>;

  DeviceSimulator();
  ~DeviceSimulator() override = default;

  void setTarget(double positionOrVelocity) override;
  void start() override;
  void stop() override;

  common::device::DeviceSnapshot latestSnapshot() const override;

  /** 每周期步进，由仿真线程调用。 */
  void step(double dtSeconds);

  void setStateCallback(StateCallback cb) { _onStateUpdated = std::move(cb); }
  void setStartedCallback(StartedCallback cb) { _onStarted = std::move(cb); }
  void setStoppedCallback(StoppedCallback cb) { _onStopped = std::move(cb); }

  bool isRunning() const { return _running; }
  double targetPosition() const { return _targetPosition; }
  double currentPosition() const { return _position; }
  double currentVelocity() const { return _velocity; }
  double currentSensorValue() const { return _sensorValue; }

private:
  double _targetPosition{0.0};
  double _position{0.0};
  double _velocity{0.0};
  double _sensorValue{0.0};
  bool _running{false};
  mutable unsigned _rng{1};

  StateCallback _onStateUpdated;
  StartedCallback _onStarted;
  StoppedCallback _onStopped;
};
