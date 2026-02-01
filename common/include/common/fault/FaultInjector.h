#pragma once

#include "common/config/Config.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

namespace common::fault {

class FaultInjector {
public:
  struct Params {
    bool enable{false};
    bool crashOnStart{false};
    bool hangOnStart{false};
    int extraDelayMs{0};
  };

  static Params LoadFromConfig(const common::config::Config& cfg);

  explicit FaultInjector(Params p);

  void maybeCrashOnStart();
  void maybeHangOnStart();

  void applyExtraDelay();

private:
  Params _p;
};

} // namespace common::fault
