#include "common/heartbeat/HeartbeatMonitor.h"

#include "common/log/Log.h"
#include "common/time/MonotonicClock.h"

#include <thread>

namespace common::heartbeat {

HeartbeatMonitor::HeartbeatMonitor(common::ipc::IpcClient& client, Params params)
    : _client(client), _params(params) {}

HeartbeatMonitor::~HeartbeatMonitor() { stop(); }

void HeartbeatMonitor::start() {
  if (_running.exchange(true)) return;
  _thread = std::thread(&HeartbeatMonitor::run, this);
}

void HeartbeatMonitor::stop() {
  if (!_running.exchange(false)) return;
  if (_thread.joinable()) _thread.join();
}

void HeartbeatMonitor::run() {
  common::log::SetThreadName("heartbeat");

  std::uint64_t seq = 0;
  std::uint32_t misses = 0;
  _healthy.store(false);

  while (_running.load()) {
    if (!_client.isConnected()) {
      _healthy.store(false);
      common::log::Debug("heartbeat", "not connected; skipping ping");
      std::this_thread::sleep_for(_params.interval);
      continue;
    }

    common::ipc::Ping ping;
    ping.seq = ++seq;
    ping.t0MonotonicNs = common::time::NowMonotonicNs();

    (void)_client.sendPing(ping, _params.timeout);

    common::ipc::Pong pong;
    const bool got = _client.tryReceivePong(pong, _params.timeout);

    // Accept Pong for current or recent Ping (late Pong still proves worker is alive; allow up to 2 rounds late)
    constexpr std::uint64_t kMaxRoundsLate = 2;
    const bool seqOk = got && pong.seq <= ping.seq && pong.seq + kMaxRoundsLate >= ping.seq;

    if (!seqOk) {
      ++misses;
      _timeouts.fetch_add(1);
      const bool stillHealthy = misses < _params.missThreshold;
      _healthy.store(stillHealthy);
      const std::string msg = "miss: got=" + std::string(got ? "true" : "false") +
          " pong.seq=" + std::to_string(got ? pong.seq : 0) +
          " ping.seq=" + std::to_string(ping.seq) +
          " misses=" + std::to_string(misses) +
          " threshold=" + std::to_string(_params.missThreshold) +
          " healthy=" + std::string(stillHealthy ? "true" : "false");
      if (stillHealthy) {
        common::log::Debug("heartbeat", msg);
      } else {
        common::log::Warn("heartbeat", msg + " (unhealthy)");
      }
    } else {
      if (misses > 0) {
        common::log::Debug("heartbeat", "pong OK seq=" + std::to_string(pong.seq) + " misses reset to 0");
      }
      misses = 0;
      if (pong.seq == ping.seq) {
        const auto t2 = common::time::NowMonotonicNs();
        _lastRttMs.store(common::time::NsToMs(t2 - ping.t0MonotonicNs));
      }
      _healthy.store(true);
    }

    std::this_thread::sleep_for(_params.interval);
  }
}

} // namespace common::heartbeat
