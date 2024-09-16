#include <chrono>
#include <cstdio>
#include <thread>
#include "../v1_chrono_log/tracer.hpp"
#include "../v2_ring_rdtsc/tracer.hpp"
#include "../v3_thread_local/tracer.hpp"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
  // v1 records in order
  { v1::Tracer t; t.trace("a"); t.trace("b"); CHECK(t.size() == 2 && t.events[0].first == "a"); }

  // v2: ids kept in order, timestamps monotonic, ring wraps correctly
  { v2::Tracer<8> t; for (uint32_t i = 0; i < 20; ++i) t.trace(i);
    auto s = t.snapshot();
    CHECK(s.size() == 8 && s.front().id == 12 && s.back().id == 19);
    for (size_t i = 1; i < s.size(); ++i) CHECK(s[i].tsc >= s[i - 1].tsc); }

  // v3: 4 threads, merged timeline is sorted and complete
  { v3::Tracer t;
    std::vector<std::thread> th;
    for (int k = 0; k < 4; ++k) th.emplace_back([&, k] { for (int i = 0; i < 1000; ++i) t.trace(k); });
    for (auto& x : th) x.join();
    auto m = t.merge();
    CHECK(m.size() == 4000);
    for (size_t i = 1; i < m.size(); ++i) CHECK(m[i].tsc >= m[i - 1].tsc); }

  // v3: a second tracer created on the same thread after the first is destroyed
  // (regression: cached thread_local pointer used to dangle)
  { for (int round = 0; round < 3; ++round) { v3::Tracer t; t.trace(1); t.trace(2); CHECK(t.merge().size() == 2); } }

  // calibration: 100 ms sleep should measure ~100 ms (allow generous scheduler slack)
  { TscCal cal(50);
    uint64_t a = rdtsc();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    double ms = cal.delta_ns(a, rdtsc()) / 1e6;
    std::printf("100 ms sleep measured as %.2f ms\n", ms);
    CHECK(ms > 99 && ms < 130); }

  std::puts(failures ? "FAILED" : "tracer tests ok");
  return failures ? 1 : 0;
}
