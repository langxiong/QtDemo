#pragma once

#include <cstdint>

namespace common::ipc {

constexpr std::uint32_t kMagic = 0x4D524344; // 'MRCD'
constexpr std::uint16_t kVersion = 1;

enum class MsgType : std::uint16_t {
  Ping = 1,
  Pong = 2,
  SensorFrame = 3,
  AlgoResult = 4,
  StatusFrame = 5
};

#pragma pack(push, 1)
struct FrameHeader {
  std::uint32_t magic{kMagic};
  std::uint16_t version{kVersion};
  std::uint16_t type{0};
  std::uint32_t payloadSize{0};
};
#pragma pack(pop)

struct Ping {
  std::uint64_t seq{0};
  std::uint64_t t0MonotonicNs{0};
};

struct Pong {
  std::uint64_t seq{0};
  std::uint64_t t0MonotonicNs{0};
  std::uint64_t t1MonotonicNs{0};
};

struct SensorFrame {
  std::uint64_t seq{0};
  std::uint64_t monotonicNs{0};
  double valueA{0.0};
  double valueB{0.0};
  double valueC{0.0};
};

struct AlgoResult {
  std::uint64_t sensorSeq{0};
  std::uint64_t producedMonotonicNs{0};
  double outValue{0.0};
  double latencyMs{0.0};
};

struct StatusFrame {
  std::uint64_t monotonicNs{0};
  std::uint32_t statusCode{0};
};

} // namespace common::ipc
