#include "common/config/Config.h"
#include "common/fault/FaultInjector.h"
#include "common/ipc/IpcServer.h"
#include "common/ipc/Protocol.h"
#include "common/log/Log.h"
#include "common/time/MonotonicClock.h"

#include <Poco/Net/SocketAddress.h>

#include <chrono>
#include <thread>

static common::ipc::AlgoResult MakeResult(const common::ipc::SensorFrame& in, int computeDelayMs,
                                         common::fault::FaultInjector& fault) {
  const auto t0 = common::time::NowMonotonicNs();

  fault.applyExtraDelay();

  if (computeDelayMs > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(computeDelayMs));
  }

  // Demo algorithm: a simple weighted sum
  common::ipc::AlgoResult out;
  out.sensorSeq = in.seq;
  out.producedMonotonicNs = common::time::NowMonotonicNs();
  out.outValue = 0.6 * in.valueA + 0.3 * in.valueB + 0.1 * in.valueC;
  out.latencyMs = common::time::NsToMs(out.producedMonotonicNs - t0);
  return out;
}

int main() {
  common::log::Log::Init("algo_worker");
  common::log::Log::SetThreadName("main");
  common::log::Log::Info("main", "algo_worker starting");

  common::config::Config cfg;
  if (!cfg.load("config/app.ini")) {
    common::log::Log::Warn("config", "failed to load config/app.ini (using defaults)");
  }

  auto fault = common::fault::FaultInjector(common::fault::FaultInjector::LoadFromConfig(cfg));
  fault.maybeCrashOnStart();
  fault.maybeHangOnStart();

  const std::string host = cfg.getString("ipc.host", "127.0.0.1");
  const int port = cfg.getInt("ipc.port", 45678);
  const int computeDelayMs = cfg.getInt("algo.compute_delay_ms", 10);

  Poco::Net::SocketAddress bindAddr(host, port);
  common::ipc::IpcServer server(bindAddr);

  common::log::Log::Info("ipc", "waiting for controller connection...");
  while (!server.acceptOne(std::chrono::milliseconds(1000))) {
  }
  common::log::Log::Info("ipc", "controller connected");

  while (server.isConnected()) {
    common::ipc::Ping ping;
    if (server.tryReceivePing(ping, std::chrono::milliseconds(5))) {
      common::ipc::Pong pong;
      pong.seq = ping.seq;
      pong.t0MonotonicNs = ping.t0MonotonicNs;
      pong.t1MonotonicNs = common::time::NowMonotonicNs();
      (void)server.sendPong(pong, std::chrono::milliseconds(50));
      continue;
    }

    common::ipc::SensorFrame frame;
    if (server.tryReceiveSensorFrame(frame, std::chrono::milliseconds(5))) {
      const auto res = MakeResult(frame, computeDelayMs, fault);
      (void)server.sendAlgoResult(res, std::chrono::milliseconds(50));
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  common::log::Log::Info("main", "algo_worker exiting");
  return 0;
}
