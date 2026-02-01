#pragma once

#include "common/control/ControlModels.h"

#include <mutex>

namespace common::control {

class ActuatorSimulator {
public:
  void apply(const ControlCommand& cmd, double dtSeconds);
  ActuatorState state() const;

private:
  mutable std::mutex _mu;
  ActuatorState _state;
};

} // namespace common::control
