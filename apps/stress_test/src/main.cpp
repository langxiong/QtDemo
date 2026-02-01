#include "common/log/Log.h"
#include "common/rt/DoubleBufferChannel.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

struct Payload {
  std::uint64_t seq{0};
  double value{0.0};
};

int main() {
  common::log::Log::Init("stress_test");
  common::log::Log::SetThreadName("main");
  common::log::Log::Info("main", "stress_test starting");

  common::rt::DoubleBufferChannel<Payload> ch;

  std::atomic<bool> running{true};
  std::atomic<std::uint64_t> produced{0};
  std::atomic<std::uint64_t> c1Reads{0};
  std::atomic<std::uint64_t> c2Reads{0};
  std::atomic<std::uint64_t> c1MonotonicViolations{0};
  std::atomic<std::uint64_t> c2MonotonicViolations{0};

  std::thread producer([&]() {
    common::log::Log::SetThreadName("producer");
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
    common::log::Log::SetThreadName(name);
    std::uint64_t last = 0;
    while (running.load()) {
      const auto snap = ch.readSnapshot();
      if (snap.seq < last) {
        violations.fetch_add(1);
      }
      last = snap.seq;
      reads.fetch_add(1);
    }
  };

  std::thread c1([&]() { consumerFn("consumer1", c1Reads, c1MonotonicViolations); });
  std::thread c2([&]() { consumerFn("consumer2", c2Reads, c2MonotonicViolations); });

  std::this_thread::sleep_for(std::chrono::seconds(10));
  running.store(false);

  producer.join();
  c1.join();
  c2.join();

  common::log::Log::Info("main", "Produced: " + std::to_string(produced.load()));
  common::log::Log::Info("main", "PublishCount: " + std::to_string(ch.publishCount()));
  common::log::Log::Info("main", "Consumer1 reads: " + std::to_string(c1Reads.load()) +
                                  ", monotonic violations: " +
                                  std::to_string(c1MonotonicViolations.load()));
  common::log::Log::Info("main", "Consumer2 reads: " + std::to_string(c2Reads.load()) +
                                  ", monotonic violations: " +
                                  std::to_string(c2MonotonicViolations.load()));

  common::log::Log::Info("main", "stress_test exiting");
  return 0;
}
