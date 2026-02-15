#include "DDSNode.hpp"
#include "ControlCommand.hpp"
#include "JointState.hpp"

#include "common/config/ConfigPoco.h"
#include "common/ipc/Protocol.h"
#include "common/log/Log.h"
#include "common/time/MonotonicClock.h"

#include <Poco/Util/ServerApplication.h>

#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

using Poco::Util::Application;
using Poco::Util::ServerApplication;

static std::atomic<bool> g_shutdown{false};

void on_signal(int) { g_shutdown.store(true); }

static ControlCommand makeControlCommand(const JointState& js, int computeDelayMs) {
  ControlCommand cmd;
  for (int i = 0; i < 6; ++i) {
    cmd.target_position()[i] = 0.6 * js.position()[i] + 0.3 * js.velocity()[i] + 0.1 * (i + 1);
  }
  if (computeDelayMs > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(computeDelayMs));
  }
  cmd.timestamp(common::time::NowMonotonicNs());
  return cmd;
}

class AlgoWorkerDdsApp : public ServerApplication {
public:
  AlgoWorkerDdsApp() { setUnixOptions(true); }

protected:
  void initialize(Application& self) override {
    loadConfiguration();
    ServerApplication::initialize(self);
    common::log::InitFromConfig(common::config::WrapPocoConfig(config()), commandName());
  }

  int main(const std::vector<std::string>& args) override {
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    common::log::SetThreadName("main");
    common::log::Info("main", "algo_worker (DDS PlannerNode) starting");

    int domain_id = config().getInt("dds.domain_id", 0);
    int computeDelayMs = config().getInt("algo.compute_delay_ms", 10);

    dds_core::DDSNode node(domain_id);
    auto* pub = node.createPublisher();
    auto* sub = node.createSubscriber();
    if (!pub || !sub) {
      common::log::Error("main", "Failed to create DDS pub/sub");
      return Application::EXIT_SOFTWARE;
    }
    auto* writer = node.createControlCommandWriter(pub);
    auto* reader = node.createJointStateReader(sub);
    if (!writer || !reader) {
      common::log::Error("main", "Failed to create DDS writer/reader");
      node.shutdown();
      return Application::EXIT_SOFTWARE;
    }

    common::log::Info("main", "PlannerNode ready, subscribing JointState, publishing ControlCommand");
    {
      const unsigned char b = common::ipc::kReadyByte;
#if defined(_WIN32)
      ::_write(1, reinterpret_cast<const char*>(&b), 1);
#else
      ::write(1, &b, 1);
#endif
    }

    JointState js;
    ControlCommand cmd;
    eprosima::fastdds::dds::SampleInfo info;
    const auto timeout = eprosima::fastdds::dds::Duration_t(0, 50000000);  // 50ms

    while (!g_shutdown.load()) {
      if (reader->wait_for_unread_message(timeout)) {
        while (eprosima::fastdds::dds::RETCODE_OK == reader->take_next_sample(&js, &info)) {
          if (info.valid_data) {
            cmd = makeControlCommand(js, computeDelayMs);
            writer->write(&cmd);
          }
        }
      }
    }

    common::log::Info("main", "algo_worker (DDS) exiting");
    node.shutdown();
    return Application::EXIT_OK;
  }
};

POCO_SERVER_MAIN(AlgoWorkerDdsApp)
