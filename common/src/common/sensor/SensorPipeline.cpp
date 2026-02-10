#include "common/sensor/SensorPipeline.h"

#include "common/log/Log.h"
#include "common/sensor/SensorSimulator.h"

#include <chrono>

namespace common::sensor {

SensorPipeline::SensorPipeline(Params params) : _params(params) {}

SensorPipeline::~SensorPipeline() { stop(); }

void SensorPipeline::start() {
  if (_running.exchange(true)) return;
  _thread = std::thread(&SensorPipeline::run, this);
}

void SensorPipeline::stop() {
  if (!_running.exchange(false)) return;
  if (_thread.joinable()) _thread.join();
}

SensorSnapshot SensorPipeline::latest() const { return _channel.readSnapshot(); }

void SensorPipeline::run() {
  common::log::SetThreadName("sensor");

  SensorSimulator sim(SensorSimulator::Params{_params.rateHz});

  const auto period = std::chrono::duration<double>(1.0 / static_cast<double>(std::max(1, _params.rateHz)));

  while (_running.load()) {
    const auto t0 = std::chrono::steady_clock::now();

    auto& back = _channel.back();
    back.latest = sim.generate();
    back.effectiveRateHz = sim.effectiveRateHz();
    back.missedDeadlines = sim.missedDeadlines();

    _channel.publishSwap();

    const auto t1 = std::chrono::steady_clock::now();
    const auto elapsed = t1 - t0;
    if (elapsed < period) {
      std::this_thread::sleep_for(period - elapsed);
    }
  }
}

} // namespace common::sensor
