#pragma once
// v3: one buffer per thread (thread_local), merged offline by timestamp.
// The hot path touches only this thread's private cache lines: no sharing, no
// false sharing, no atomics. A mutex is taken exactly once per thread, when its
// buffer is registered.
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include "../common/tsc.hpp"

namespace v3 {
struct Event { uint64_t tsc; uint32_t id; uint32_t thread; };

constexpr size_t kPerThread = 1u << 20;  // events per thread (ring)

struct ThreadBuf {
  std::unique_ptr<Event[]> ev = std::make_unique<Event[]>(kPerThread);
  size_t n = 0;
  uint32_t thread_idx = 0;
};

class Tracer {
  std::mutex reg_m_;
  std::vector<std::unique_ptr<ThreadBuf>> bufs_;

  ThreadBuf* attach() {
    std::lock_guard<std::mutex> g(reg_m_);
    bufs_.push_back(std::make_unique<ThreadBuf>());
    bufs_.back()->thread_idx = (uint32_t)bufs_.size() - 1;
    return bufs_.back().get();
  }

  static uint64_t next_uid() { static std::atomic<uint64_t> n{0}; return ++n; }
  const uint64_t uid_ = next_uid();

