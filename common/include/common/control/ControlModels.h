#pragma once

#include <cstdint>

namespace common::control {

struct ControlCommand {
  std::uint64_t basedOnSensorSeq{0};
  double cmdValue{0.0};
};

struct ActuatorState {
  double position{0.0};
  double velocity{0.0};
};

} // namespace common::control
