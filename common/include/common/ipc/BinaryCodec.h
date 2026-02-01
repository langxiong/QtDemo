#pragma once

#include "common/ipc/Protocol.h"

#include <Poco/BinaryReader.h>
#include <Poco/BinaryWriter.h>
#include <Poco/Exception.h>

#include <cstdint>
#include <ostream>
#include <istream>
#include <stdexcept>

namespace common::ipc {

inline void WriteHeader(Poco::BinaryWriter& w, const FrameHeader& h) {
  w << h.magic;
  w << h.version;
  w << h.type;
  w << h.payloadSize;
}

inline FrameHeader ReadHeader(Poco::BinaryReader& r) {
  FrameHeader h;
  r >> h.magic;
  r >> h.version;
  r >> h.type;
  r >> h.payloadSize;
  if (h.magic != kMagic) {
    throw std::runtime_error("IPC bad magic");
  }
  if (h.version != kVersion) {
    throw std::runtime_error("IPC bad version");
  }
  return h;
}

inline void WritePayload(Poco::BinaryWriter& w, const Ping& p) {
  w << p.seq;
  w << p.t0MonotonicNs;
}

inline void ReadPayload(Poco::BinaryReader& r, Ping& p) {
  r >> p.seq;
  r >> p.t0MonotonicNs;
}

inline void WritePayload(Poco::BinaryWriter& w, const Pong& p) {
  w << p.seq;
  w << p.t0MonotonicNs;
  w << p.t1MonotonicNs;
}

inline void ReadPayload(Poco::BinaryReader& r, Pong& p) {
  r >> p.seq;
  r >> p.t0MonotonicNs;
  r >> p.t1MonotonicNs;
}

inline void WritePayload(Poco::BinaryWriter& w, const SensorFrame& f) {
  w << f.seq;
  w << f.monotonicNs;
  w << f.valueA;
  w << f.valueB;
  w << f.valueC;
}

inline void ReadPayload(Poco::BinaryReader& r, SensorFrame& f) {
  r >> f.seq;
  r >> f.monotonicNs;
  r >> f.valueA;
  r >> f.valueB;
  r >> f.valueC;
}

inline void WritePayload(Poco::BinaryWriter& w, const AlgoResult& a) {
  w << a.sensorSeq;
  w << a.producedMonotonicNs;
  w << a.outValue;
  w << a.latencyMs;
}

inline void ReadPayload(Poco::BinaryReader& r, AlgoResult& a) {
  r >> a.sensorSeq;
  r >> a.producedMonotonicNs;
  r >> a.outValue;
  r >> a.latencyMs;
}

inline void WritePayload(Poco::BinaryWriter& w, const StatusFrame& s) {
  w << s.monotonicNs;
  w << s.statusCode;
}

inline void ReadPayload(Poco::BinaryReader& r, StatusFrame& s) {
  r >> s.monotonicNs;
  r >> s.statusCode;
}

} // namespace common::ipc
