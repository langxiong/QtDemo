#pragma once

/**
 * 数据采集接口（预留抽象）
 * 用于获取当前状态快照，DeviceSimulator 与 SensorPipeline 均可实现或适配此接口。
 * 扩展方式：实现 latestSnapshot()，通过依赖注入注入到需要读取数据的模块。
 */
namespace common::device {

struct DeviceSnapshot {
  double position{0.0};
  double velocity{0.0};
  double sensorValue{0.0};
  bool running{false};
};

class IDataAcquisition {
public:
  virtual ~IDataAcquisition() = default;

  virtual DeviceSnapshot latestSnapshot() const = 0;
};

} // namespace common::device
