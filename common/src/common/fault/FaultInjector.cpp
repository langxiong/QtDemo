#include "common/fault/FaultInjector.h"

#include "common/log/Log.h"

#include <cstdlib>
#include <thread>

namespace common::fault {

FaultInjector::Params FaultInjector::LoadFromConfig(const Poco::Util::AbstractConfiguration& cfg) {
  Params p;
  p.enable = cfg.getBool("fault.enable", false);
  p.crashOnStart = cfg.getBool("fault.crash_on_start", false);
  p.hangOnStart = cfg.getBool("fault.hang_on_start", false);
  p.extraDelayMs = cfg.getInt("fault.extra_delay_ms", 0);
  return p;
}

FaultInjector::FaultInjector(Params p) : _p(p) {}

void FaultInjector::maybeCrashOnStart() {
  if (!_p.enable || !_p.crashOnStart) return;
  common::log::Error("fault", "fault.crash_on_start triggered");
  std::abort();
}

void FaultInjector::maybeHangOnStart() {
  if (!_p.enable || !_p.hangOnStart) return;
  common::log::Error("fault", "fault.hang_on_start triggered");
  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void FaultInjector::applyExtraDelay() {
  if (!_p.enable) return;
  if (_p.extraDelayMs <= 0) return;
  std::this_thread::sleep_for(std::chrono::milliseconds(_p.extraDelayMs));
}

} // namespace common::fault
