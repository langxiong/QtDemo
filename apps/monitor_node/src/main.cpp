#include "DDSNode.hpp"
#include "ControlCommand.hpp"
#include "JointState.hpp"

#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

static std::atomic<bool> g_shutdown{false};

void on_signal(int) { g_shutdown.store(true); }

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  std::signal(SIGINT, on_signal);
#if defined(_WIN32)
  std::signal(SIGTERM, on_signal);
#else
  std::signal(SIGTERM, on_signal);
#endif

  int domain_id = 0;
  if (const char* s = std::getenv("FASTRTPS_DEFAULT_PROFILES_FILE")) {
    (void)s;
  }
  if (const char* s = std::getenv("DDS_DOMAIN_ID")) {
    domain_id = std::atoi(s);
  }

  std::cout << "[monitor_node] Starting, domain_id=" << domain_id << std::endl;

  dds_core::DDSNode node(domain_id);
  auto* sub = node.createSubscriber();
  if (!sub) {
    std::cerr << "[monitor_node] Failed to create subscriber" << std::endl;
    return 1;
  }
  auto* js_reader = node.createJointStateReader(sub);
  auto* cc_reader = node.createControlCommandReader(sub);
  if (!js_reader || !cc_reader) {
    std::cerr << "[monitor_node] Failed to create readers" << std::endl;
    node.shutdown();
    return 1;
  }

  // Counters for frequency stats
  std::atomic<uint64_t> js_count{0};
  std::atomic<uint64_t> cc_count{0};
  std::atomic<uint64_t> js_last_ts{0};
  std::atomic<uint64_t> cc_last_ts{0};

  // Reader thread: take samples and update counters
  std::thread reader_thread([&]() {
    JointState js;
    ControlCommand cc;
    eprosima::fastdds::dds::SampleInfo info;
    const auto timeout = eprosima::fastdds::dds::Duration_t(0, 100000000);  // 100ms

    while (!g_shutdown.load()) {
      if (js_reader->wait_for_unread_message(timeout)) {
        while (eprosima::fastdds::dds::RETCODE_OK == js_reader->take_next_sample(&js, &info)) {
          if (info.valid_data) {
            js_count.fetch_add(1);
            js_last_ts.store(js.timestamp());
          }
        }
      }
      if (cc_reader->wait_for_unread_message(timeout)) {
        while (eprosima::fastdds::dds::RETCODE_OK == cc_reader->take_next_sample(&cc, &info)) {
          if (info.valid_data) {
            cc_count.fetch_add(1);
            cc_last_ts.store(cc.timestamp());
          }
        }
      }
    }
  });

  // Stats print thread
  constexpr int stats_interval_ms = 2000;
  auto last_print = std::chrono::steady_clock::now();
  uint64_t last_js = 0, last_cc = 0;

  while (!g_shutdown.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_print).count() >= stats_interval_ms) {
      uint64_t curr_js = js_count.load();
      uint64_t curr_cc = cc_count.load();
      double js_hz = (curr_js - last_js) * 1000.0 / stats_interval_ms;
      double cc_hz = (curr_cc - last_cc) * 1000.0 / stats_interval_ms;
      std::cout << "[monitor_node] JointState: " << js_hz << " Hz, last_ts=" << js_last_ts.load()
                << " | ControlCommand: " << cc_hz << " Hz, last_ts=" << cc_last_ts.load() << std::endl;
      last_js = curr_js;
      last_cc = curr_cc;
      last_print = now;
    }
  }

  reader_thread.join();
  node.shutdown();
  std::cout << "[monitor_node] Exiting" << std::endl;
  return 0;
}
