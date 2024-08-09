#pragma once
// v2: preallocated ring buffer + integer event ids + rdtsc.
// No allocation, no lock, no string, no syscall on the hot path: one rdtsc and
// one 16-byte store. Names live in a side table and are only resolved offline.
// Single-threaded by design (see v3 for threads). Oldest events are overwritten.
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>
#include "../common/tsc.hpp"

namespace v2 {
struct Event { uint64_t tsc; uint32_t id; uint32_t aux; };

template <size_t N = (1u << 20)>
class Tracer {
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
  std::unique_ptr<Event[]> buf_ = std::make_unique<Event[]>(N);  // zero-filled = pages pre-touched
  size_t n_ = 0;

 public:
  inline void trace(uint32_t id, uint32_t aux = 0) noexcept {
    buf_[n_++ & (N - 1)] = Event{rdtsc(), id, aux};
  }
  size_t size() const { return n_ < N ? n_ : N; }
  // Events in chronological order (unwraps the ring). Offline only.
  std::vector<Event> snapshot() const {
    std::vector<Event> out;
    size_t cnt = size(), start = n_ < N ? 0 : n_ & (N - 1);
    out.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i) out.push_back(buf_[(start + i) & (N - 1)]);
    return out;
  }
};
}  // namespace v2
