#include "DeviceSimulator.h"

#include <cmath>
#include <cstdlib>

namespace {
constexpr double kProportionalGain = 2.0;
constexpr double kSensorNoiseAmplitude = 0.05;
}  // namespace

DeviceSimulator::DeviceSimulator() = default;

void DeviceSimulator::setTarget(double positionOrVelocity) {
  _targetPosition = positionOrVelocity;
}

void DeviceSimulator::start() {
  if (_running) return;
  _running = true;
  if (_onStarted) _onStarted();
}

void DeviceSimulator::stop() {
  if (!_running) return;
  _running = false;
  if (_onStopped) _onStopped();
}

common::device::DeviceSnapshot DeviceSimulator::latestSnapshot() const {
  return {_position, _velocity, _sensorValue, _running};
}

void DeviceSimulator::step(double dtSeconds) {
  if (!_running) return;

  const double error = _targetPosition - _position;
  _velocity = kProportionalGain * error;
  _position += _velocity * dtSeconds;

  _rng = _rng * 1103515245u + 12345u;
  const double noise =
      (static_cast<double>(_rng % 65536) / 65536.0 - 0.5) * 2.0 * kSensorNoiseAmplitude;
  _sensorValue = _position + noise;

  if (_onStateUpdated) _onStateUpdated(_position, _velocity, _sensorValue);
}
