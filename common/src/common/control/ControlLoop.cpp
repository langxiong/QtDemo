#include "common/control/ControlLoop.h"

#include "common/log/Log.h"
#include "common/time/MonotonicClock.h"

#include <chrono>

namespace common::control {

ControlLoop::ControlLoop(const common::sensor::SensorPipeline& sensor,
                         common::ipc::IpcClient& ipc,
                         ActuatorSimulator& actuator,
                         common::status::StatusStore& status,
                         Params params)
    : _sensor(sensor), _ipc(ipc), _actuator(actuator), _status(status), _params(params) {}

ControlLoop::~ControlLoop() { stop(); }

void ControlLoop::start() {
  if (_running.exchange(true)) return;
  _thread = std::thread(&ControlLoop::run, this);
}

void ControlLoop::stop() {
  if (!_running.exchange(false)) return;
  if (_thread.joinable()) _thread.join();
}

ControlCommand ControlLoop::lastCommand() const {
  std::lock_guard<std::mutex> lk(_mu);
  return _lastCmd;
}

ControlCommand ControlLoop::computeCommand(const common::sensor::SensorSnapshot& s,
                                          const common::ipc::AlgoResult* algo) {
  ControlCommand cmd;
  cmd.basedOnSensorSeq = s.latest.seq;

  if (!algo) {
    // Safe default command: deterministic 0 output.
    cmd.cmdValue = 0.0;
    return cmd;
  }

  // Demo mapping: pass algo output through as command.
  cmd.cmdValue = algo->outValue;
  return cmd;
}

void ControlLoop::run() {
  common::log::Log::SetThreadName("control");

  const double dt = 1.0 / static_cast<double>(std::max(1, _params.rateHz));
  const auto period = std::chrono::duration<double>(dt);

  std::uint64_t lastSeq = 0;

  while (_running.load()) {
    const auto t0 = std::chrono::steady_clock::now();

    const auto snap = _sensor.latest();

    // Sync to sensor: only compute when we see a new seq.
    if (snap.latest.seq == lastSeq) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    lastSeq = snap.latest.seq;

    // Send sensor frame to algo worker (best-effort).
    if (_ipc.isConnected()) {
      common::ipc::SensorFrame sf;
      sf.seq = snap.latest.seq;
      sf.monotonicNs = snap.latest.monotonicNs;
      sf.valueA = snap.latest.valueA;
      sf.valueB = snap.latest.valueB;
      sf.valueC = snap.latest.valueC;
      (void)_ipc.sendSensorFrame(sf, std::chrono::milliseconds(5));
    }

    // Try receive latest AlgoResult.
    common::ipc::AlgoResult algo;
    common::ipc::AlgoResult* algoPtr = nullptr;

    if (_ipc.isConnected() && _ipc.tryReceiveAlgoResult(algo, _params.algoReadTimeout)) {
      _lastAlgo = algo;
      _hasAlgo.store(true);
      algoPtr = &_lastAlgo;
    } else if (_hasAlgo.load()) {
      algoPtr = &_lastAlgo;
    }

    const auto cmd = computeCommand(snap, algoPtr);

    {
      std::lock_guard<std::mutex> lk(_mu);
      _lastCmd = cmd;
    }

    _actuator.apply(cmd, dt);
    const auto act = _actuator.state();

    // Update status snapshot with control metrics.
    auto st = _status.read();
    st.controlLoopHz = static_cast<double>(_params.rateHz);
    st.algoLatencyMs = algoPtr ? algoPtr->latencyMs : 0.0;
    st.lastCommand = cmd.cmdValue;
    st.actuatorPosition = act.position;
    st.actuatorVelocity = act.velocity;
    _status.update(st);

    const auto t1 = std::chrono::steady_clock::now();
    const auto elapsed = t1 - t0;
    if (elapsed < period) {
      std::this_thread::sleep_for(period - elapsed);
    }
  }
}

} // namespace common::control
