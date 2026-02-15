#include "ControllerRuntimeDds.h"
#include "DDSNode.hpp"
#include "JointState.hpp"
#include "ControlCommand.hpp"

#include "common/control/ControlModels.h"
#include "common/log/Log.h"
#include "common/status/Models.h"
#include "common/time/MonotonicClock.h"

#include <Poco/Exception.h>
#include <Poco/Path.h>
#include <Poco/Process.h>

#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <chrono>
#include <thread>

namespace controller_app {

ControllerRuntimeDds::ControllerRuntimeDds(const common::config::Config& cfg,
                                           const common::sensor::SensorPipeline& sensor,
                                           common::status::StatusStore& status,
                                           const std::string& applicationDirPath)
    : _sensor(sensor),
      _status(status),
      _algoExePath(Poco::Path(applicationDirPath).append("algo_worker").toString()),
      _domainId(cfg.getInt("dds.domain_id", 0)),
      _sensorRateHz(cfg.getInt("sensor.rate_hz", 200)),
      _algoTimeoutMs(cfg.getInt("ipc.heartbeat_timeout_ms", 500)) {}

ControllerRuntimeDds::~ControllerRuntimeDds() {
  stop();
}

void ControllerRuntimeDds::start() {
  if (_running.exchange(true)) return;

  try {
    Poco::Process::Args args;
    Poco::Process::launch(_algoExePath, args);
    common::log::Info("controller_dds", "launched algo_worker");
  } catch (const Poco::Exception& e) {
    common::log::Warn("controller_dds", "failed to launch algo_worker: " + e.displayText());
  }

  _node = std::make_unique<dds_core::DDSNode>(_domainId);
  auto* pub = _node->createPublisher();
  auto* sub = _node->createSubscriber();
  if (!pub || !sub) {
    common::log::Error("controller_dds", "Failed to create DDS pub/sub");
    _running = false;
    return;
  }
  _jsWriter = _node->createJointStateWriter(pub);
  _ccReader = _node->createControlCommandReader(sub);
  if (!_jsWriter || !_ccReader) {
    common::log::Error("controller_dds", "Failed to create DDS writer/reader");
    _node->shutdown();
    _running = false;
    return;
  }

  auto* writer = static_cast<eprosima::fastdds::dds::DataWriter*>(_jsWriter);
  auto* reader = static_cast<eprosima::fastdds::dds::DataReader*>(_ccReader);

  _pubThread = std::thread([this]() { runPublisher(); });
  _subThread = std::thread([this]() { runSubscriber(); });
  _statusThread = std::thread([this]() { runStatusUpdater(); });
}

void ControllerRuntimeDds::stop() {
  if (!_running.exchange(false)) return;
  if (_pubThread.joinable()) _pubThread.join();
  if (_subThread.joinable()) _subThread.join();
  if (_statusThread.joinable()) _statusThread.join();
  if (_node) _node->shutdown();
}

void ControllerRuntimeDds::runPublisher() {
  common::log::SetThreadName("dds_pub");
  const auto period = std::chrono::microseconds(2000);  // 2ms
  auto* writer = static_cast<eprosima::fastdds::dds::DataWriter*>(_jsWriter);

  while (_running.load()) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto snap = _sensor.latest();

    JointState js;
    js.position()[0] = snap.latest.valueA;
    js.position()[1] = snap.latest.valueB;
    js.position()[2] = snap.latest.valueC;
    js.position()[3] = 0;
    js.position()[4] = 0;
    js.position()[5] = 0;
    js.velocity()[0] = js.velocity()[1] = js.velocity()[2] = 0;
    js.velocity()[3] = js.velocity()[4] = js.velocity()[5] = 0;
    js.timestamp(common::time::NowMonotonicNs());

    writer->write(&js);

    const auto elapsed = std::chrono::steady_clock::now() - t0;
    if (elapsed < period) {
      std::this_thread::sleep_for(period - elapsed);
    }
  }
}

void ControllerRuntimeDds::runSubscriber() {
  common::log::SetThreadName("dds_sub");
  auto* reader = static_cast<eprosima::fastdds::dds::DataReader*>(_ccReader);
  ::ControlCommand cmd;  // DDS type
  eprosima::fastdds::dds::SampleInfo info;
  const auto timeout = eprosima::fastdds::dds::Duration_t(0, 50000000);  // 50ms

  while (_running.load()) {
    if (reader->wait_for_unread_message(timeout)) {
      while (eprosima::fastdds::dds::RETCODE_OK == reader->take_next_sample(&cmd, &info)) {
        if (info.valid_data) {
          double sum = 0;
          for (int i = 0; i < 6; ++i) sum += cmd.target_position()[i];
          _lastCmdValue.store(sum / 6.0);
          _lastCmdTimestamp.store(cmd.timestamp());
          _hasAlgo.store(true);
        }
      }
    }
  }
}

void ControllerRuntimeDds::runStatusUpdater() {
  common::log::SetThreadName("dds_status");
  const double dt = 1.0 / std::max(1, _sensorRateHz);
  uint64_t lastSeq = 0;

  while (_running.load()) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto snap = _sensor.latest();
    if (snap.latest.seq != lastSeq) {
      lastSeq = snap.latest.seq;
      common::control::ControlCommand internalCmd;
      internalCmd.basedOnSensorSeq = snap.latest.seq;
      internalCmd.cmdValue = _lastCmdValue.load();
      _actuator.apply(internalCmd, dt);
    }

    auto st = _status.read();
    st.systemState = common::status::SystemState::Running;
    st.algoHealth = _hasAlgo.load() ? common::status::AlgoHealthState::Healthy
                                    : common::status::AlgoHealthState::Disconnected;
    st.lastCommand = _lastCmdValue.load();
    const auto act = _actuator.state();
    st.actuatorPosition = act.position;
    st.actuatorVelocity = act.velocity;
    _status.update(st);

    const auto elapsed = std::chrono::steady_clock::now() - t0;
    const auto period = std::chrono::duration<double>(dt);
    if (elapsed < period) {
      std::this_thread::sleep_for(period - elapsed);
    }
  }
}

}  // namespace controller_app
