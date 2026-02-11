#pragma once

/**
 * 硬件控制接口（预留抽象）
 * 用于设备启停与目标设定，DeviceSimulator 与未来真实硬件适配器均可实现此接口。
 * 扩展方式：实现 setTarget/start/stop，通过依赖注入注入到 Controller。
 */
namespace common::device {

class IHardwareControl {
public:
  virtual ~IHardwareControl() = default;

  virtual void setTarget(double positionOrVelocity) = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
};

} // namespace common::device
