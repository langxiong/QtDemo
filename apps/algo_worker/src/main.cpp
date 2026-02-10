#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Net/SocketAddress.h>

#include "common/fault/FaultInjector.h"
#include "common/ipc/IpcServer.h"
#include "common/ipc/Protocol.h"
#include "common/log/Log.h"
#include "common/time/MonotonicClock.h"

#include <Poco/NamedEvent.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <csignal>
#include <atomic>

using Poco::Util::Application;
using Poco::Util::ServerApplication;
using Poco::Util::Option;
using Poco::Util::OptionSet;

static std::atomic<bool> g_shutdown_requested = false;

void handle_signal(int signum) {
    g_shutdown_requested = true;
}

class AlgoWorkerApp : public ServerApplication {
public:
  AlgoWorkerApp() {
    setUnixOptions(true);
  }

protected:
  void defineOptions(OptionSet& options) override {
    ServerApplication::defineOptions(options);
    options.addOption(
      Option("ready-event", "r", "Named event for controller sync (used when launched by controller)")
        .required(false)
        .argument("name", true));
  }

  void handleOption(const std::string& name, const std::string& value) override {
    ServerApplication::handleOption(name, value);
    if (name == "ready-event") {
      try {
        _readyEvent = std::make_unique<Poco::NamedEvent>(value);
      } catch (const Poco::Exception&) {
        _readyEvent.reset();
      }
    }
  }

  void initialize(Application& self) override {
    loadConfiguration();
    ServerApplication::initialize(self);
    common::log::Init(this->commandName());
  }

  int main(const std::vector<std::string>& args) override {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    common::log::SetThreadName("main");
    common::log::Info("main", "algo_worker starting");

    common::fault::FaultInjector fault(common::fault::FaultInjector::LoadFromConfig(config()));
    fault.maybeCrashOnStart();
    fault.maybeHangOnStart();

    const std::string host = config().getString("ipc.host", "127.0.0.1");
    const int port = config().getInt("ipc.port", 45678);
    const int computeDelayMs = config().getInt("algo.compute_delay_ms", 10);

    Poco::Net::SocketAddress bindAddr(host, static_cast<Poco::UInt16>(port));
    common::ipc::IpcServer server(bindAddr);

    if (_readyEvent) {
      try {
        _readyEvent->set();
      } catch (const Poco::Exception& e) {
        common::log::Warn("ipc", "ready event signal failed: " + e.displayText());
        return Application::EXIT_SOFTWARE;
      }
    }
    common::log::Info("ipc", "waiting for controller connection...");
    while (!server.acceptOne(std::chrono::milliseconds(1000))) {
      if (g_shutdown_requested) break;
    }

    if (server.isConnected()) {
        common::log::Info("ipc", "controller connected");
    }

    while (server.isConnected() && !g_shutdown_requested) {
        // Ping is handled and replied in IpcServer receiver thread so Pong is never delayed by SensorFrame work.
        common::ipc::SensorFrame frame;
        if (server.tryReceiveSensorFrame(frame, std::chrono::milliseconds(5))) {
            const auto res = makeResult(frame, computeDelayMs, fault);
            (void)server.sendAlgoResult(res, std::chrono::milliseconds(50));
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    common::log::Info("main", "algo_worker exiting");
    return Application::EXIT_OK;
  }

private:
    std::unique_ptr<Poco::NamedEvent> _readyEvent;

    common::ipc::AlgoResult makeResult(const common::ipc::SensorFrame& in, int computeDelayMs, common::fault::FaultInjector& fault) {
        const auto t0 = common::time::NowMonotonicNs();

        fault.applyExtraDelay();

        if (computeDelayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(computeDelayMs));
        }

        common::ipc::AlgoResult out;
        out.sensorSeq = in.seq;
        out.producedMonotonicNs = common::time::NowMonotonicNs();
        out.outValue = 0.6 * in.valueA + 0.3 * in.valueB + 0.1 * in.valueC;
        out.latencyMs = common::time::NsToMs(out.producedMonotonicNs - t0);
        return out;
    }
};

POCO_SERVER_MAIN(AlgoWorkerApp)
