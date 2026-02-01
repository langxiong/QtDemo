#include "common/control/ActuatorSimulator.h"

namespace common::control {

void ActuatorSimulator::apply(const ControlCommand& cmd, double dtSeconds) {
  std::lock_guard<std::mutex> lk(_mu);

  // Very simple integrator model for demo purposes.
  _state.velocity = cmd.cmdValue;
  _state.position += _state.velocity * dtSeconds;
}

ActuatorState ActuatorSimulator::state() const {
  std::lock_guard<std::mutex> lk(_mu);
  return _state;
}

} // namespace common::control
