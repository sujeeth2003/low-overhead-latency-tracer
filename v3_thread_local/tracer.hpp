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

 public:
  inline void trace(uint32_t id) {
    // Per-thread cache of "my buffer in tracer #owner". The owner check makes a
    // thread that outlives one Tracer re-attach to the next one instead of
    // writing through a dangling pointer. Assumes one active tracer per thread.
    struct Cache { uint64_t owner = 0; ThreadBuf* tb = nullptr; };
    static thread_local Cache c;
    if (__builtin_expect(c.owner != uid_, 0)) { c.tb = attach(); c.owner = uid_; }
    ThreadBuf* tb = c.tb;
    tb->ev[tb->n++ & (kPerThread - 1)] = Event{rdtsc(), id, tb->thread_idx};
  }
  // Offline: merge all threads into one timeline. Call after tracing threads stop.
  std::vector<Event> merge() const {
    std::vector<Event> all;
    for (auto& b : bufs_) {
      size_t cnt = std::min(b->n, kPerThread), start = b->n < kPerThread ? 0 : b->n & (kPerThread - 1);
      for (size_t i = 0; i < cnt; ++i) all.push_back(b->ev[(start + i) & (kPerThread - 1)]);
    }
    std::sort(all.begin(), all.end(), [](const Event& a, const Event& b) { return a.tsc < b.tsc; });
    return all;
  }
};
}  // namespace v3
