#pragma once

#include "common/rt/DoubleBufferChannel.h"
#include "common/sensor/SensorModels.h"

#include <atomic>
#include <thread>

namespace common::sensor {

class SensorPipeline {
public:
  struct Params {
    int rateHz{200};
  };

  explicit SensorPipeline(Params params);
  ~SensorPipeline();

  void start();
  void stop();

  SensorSnapshot latest() const;

private:
  void run();

  Params _params;
  mutable common::rt::DoubleBufferChannel<SensorSnapshot> _channel;

  std::atomic<bool> _running{false};
  std::thread _thread;
};

} // namespace common::sensor
