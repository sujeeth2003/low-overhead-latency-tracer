#pragma once
// v1: the naive baseline. Human-friendly string names, std::chrono clock,
// a mutex and a growing vector. Every call can allocate (long std::string
// names defeat small-string optimisation) and takes a lock.
#include <chrono>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace v1 {
struct Tracer {
  using clk = std::chrono::high_resolution_clock;
  std::mutex m;
  std::vector<std::pair<std::string, clk::time_point>> events;

  void trace(const std::string& name) {
    auto t = clk::now();
    std::lock_guard<std::mutex> g(m);
    events.emplace_back(name, t);
  }
  size_t size() const { return events.size(); }
};
}  // namespace v1
