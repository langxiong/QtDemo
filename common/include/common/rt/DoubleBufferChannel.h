#pragma once

#include <mutex>
#include <utility>

namespace common::rt {

template <typename T>
class DoubleBufferChannel {
public:
  DoubleBufferChannel() = default;

  T& back() { return _back; }
  const T& front() const { return _front; }

  void publishSwap() {
    std::lock_guard<std::mutex> lk(_mu);
    using std::swap;
    swap(_front, _back);
    ++_publishCount;
  }

  T readSnapshot() const {
    std::lock_guard<std::mutex> lk(_mu);
    return _front;
  }

  std::uint64_t publishCount() const {
    std::lock_guard<std::mutex> lk(_mu);
    return _publishCount;
  }

private:
  mutable std::mutex _mu;
  T _front{};
  T _back{};
  std::uint64_t _publishCount{0};
};

} // namespace common::rt
