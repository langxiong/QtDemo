#include <Poco/Util/Application.h>
#include <Poco/Util/OptionSet.h>

#include "common/config/ConfigPoco.h"
#include "common/log/Log.h"
#include "common/rt/DoubleBufferChannel.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

using Poco::Util::Application;
using Poco::Util::OptionSet;

struct Payload {
  std::uint64_t seq{0};
  double value{0.0};
};

class StressTestApp : public Application {
public:
  StressTestApp() = default;

protected:
  void initialize(Application& self) override {
    loadConfiguration();
    Application::initialize(self);
    common::log::InitFromConfig(common::config::WrapPocoConfig(config()), this->commandName());
  }

  int main(const std::vector<std::string>& args) override {
    common::log::SetThreadName("main");
    common::log::Info("main", "stress_test starting");

    common::rt::DoubleBufferChannel<Payload> ch;

    std::atomic<bool> running{true};
    std::atomic<std::uint64_t> produced{0};
    std::atomic<std::uint64_t> c1Reads{0};
    std::atomic<std::uint64_t> c2Reads{0};
    std::atomic<std::uint64_t> c1MonotonicViolations{0};
    std::atomic<std::uint64_t> c2MonotonicViolations{0};

    std::thread producer([&]() {
      common::log::SetThreadName("producer");
      std::uint64_t seq = 0;
      while (running.load()) {
        auto& back = ch.back();
        back.seq = ++seq;
        back.value = static_cast<double>(seq);
        ch.publishSwap();
        produced.store(seq);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });

    auto consumerFn = [&](const char* name, std::atomic<std::uint64_t>& reads,
                          std::atomic<std::uint64_t>& violations) {
      common::log::SetThreadName(name);
      std::uint64_t last = 0;
      while (running.load()) {
        const auto snap = ch.readSnapshot();
        if (snap.seq > 0 && snap.seq < last) { // Ignore first read
          violations.fetch_add(1);
        }
        last = snap.seq;
        reads.fetch_add(1);
      }
    };

    std::thread c1([&]() { consumerFn("consumer1", c1Reads, c1MonotonicViolations); });
    std::thread c2([&]() { consumerFn("consumer2", c2Reads, c2MonotonicViolations); });

    const int durationSec = config().getInt("stress_test.duration_sec", 10);
    common::log::Info("main", "Running stress test for " + std::to_string(durationSec) + " seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(durationSec));
    running.store(false);

    producer.join();
    c1.join();
    c2.join();

    common::log::Info("main", "--- Results ---");
    common::log::Info("main", "Produced: " + std::to_string(produced.load()));
    common::log::Info("main", "PublishCount: " + std::to_string(ch.publishCount()));
    common::log::Info("main", "Consumer1 reads: " + std::to_string(c1Reads.load()) +
                                    ", monotonic violations: " +
                                    std::to_string(c1MonotonicViolations.load()));
    common::log::Info("main", "Consumer2 reads: " + std::to_string(c2Reads.load()) +
                                    ", monotonic violations: " +
                                    std::to_string(c2MonotonicViolations.load()));

    common::log::Info("main", "stress_test exiting");
    return Application::EXIT_OK;
  }
};

POCO_APP_MAIN(StressTestApp)
