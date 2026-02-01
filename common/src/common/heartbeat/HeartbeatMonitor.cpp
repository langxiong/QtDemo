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
  common::log::Log::SetThreadName("heartbeat");

  std::uint64_t seq = 0;
  std::uint32_t misses = 0;
  _healthy.store(false);

  while (_running.load()) {
    if (!_client.isConnected()) {
      _healthy.store(false);
      std::this_thread::sleep_for(_params.interval);
      continue;
    }

    common::ipc::Ping ping;
    ping.seq = ++seq;
    ping.t0MonotonicNs = common::time::NowMonotonicNs();

    (void)_client.sendPing(ping, _params.timeout);

    common::ipc::Pong pong;
    const bool got = _client.tryReceivePong(pong, _params.timeout);

    if (!got || pong.seq != ping.seq) {
      ++misses;
      _timeouts.fetch_add(1);
      _healthy.store(misses < _params.missThreshold);
    } else {
      misses = 0;
      const auto t2 = common::time::NowMonotonicNs();
      _lastRttMs.store(common::time::NsToMs(t2 - ping.t0MonotonicNs));
      _healthy.store(true);
    }

    std::this_thread::sleep_for(_params.interval);
  }
}

} // namespace common::heartbeat
