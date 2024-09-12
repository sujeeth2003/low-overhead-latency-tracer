// v5: measure the measuring tool. The tracer must not change what it measures.
//
// Part 1: raw cost per trace() call for v1, v2, v3 (ns/call, lower is better).
// Part 2: perturbation - run a small fixed workload with tracing OFF, then with
//         each tracer ON (two trace points per iteration) and report the slowdown.
// For hardware-counter confirmation run:  sh v5_overhead/perf_overhead.sh (Linux)
//
// usage: overhead_bench [iterations=2000000]
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include "../common/tsc.hpp"
#include "../v1_chrono_log/tracer.hpp"
#include "../v2_ring_rdtsc/tracer.hpp"
#include "../v3_thread_local/tracer.hpp"

static double now_s() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
// ~tens of ns of dependent integer work the tracer could disturb
static inline uint64_t work(uint64_t x) {
  for (int i = 0; i < 24; ++i) { x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 32; }
  return x;
}

template <class F> double per_call_ns(size_t n, F&& f) {
  double t0 = now_s();
  for (size_t i = 0; i < n; ++i) f(i);
  return (now_s() - t0) * 1e9 / (double)n;
}
template <class F> double workload_s(size_t n, F&& f) {
  volatile uint64_t sink = 0;
  uint64_t x = 1;
  double t0 = now_s();
  for (size_t i = 0; i < n; ++i) { f(0); x = work(x); f(1); }
  sink = x; (void)sink;
  return now_s() - t0;
}

int main(int argc, char** argv) {
  size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 2'000'000;
  std::puts("== part 1: cost per trace() call ==");
  {
    double base = per_call_ns(n, [](size_t i) { asm volatile("" ::"r"(i)); });
    std::printf("empty loop            %7.2f ns\n", base);
    v1::Tracer t1; size_t n1 = std::min<size_t>(n, 500'000);
    std::printf("v1 chrono+mutex+str   %7.2f ns\n", per_call_ns(n1, [&](size_t) { t1.trace("order_received_from_gateway_handler"); }) - base);
    v2::Tracer<> t2;
    std::printf("v2 ring + rdtsc       %7.2f ns\n", per_call_ns(n, [&](size_t i) { t2.trace((uint32_t)i & 7); }) - base);
    v3::Tracer t3;
    std::printf("v3 thread_local ring  %7.2f ns\n", per_call_ns(n, [&](size_t i) { t3.trace((uint32_t)i & 7); }) - base);
  }
  std::puts("== part 2: perturbation of a fixed workload ==");
  double off = workload_s(n, [](int) {});
  std::printf("tracing off           %7.1f ns/iter\n", off * 1e9 / n);
  {
    v1::Tracer t; size_t n1 = std::min<size_t>(n, 500'000);
    double s = workload_s(n1, [&](int k) { t.trace(k ? "iteration_end_marker_long_name" : "iteration_start_marker_long_name"); });
    std::printf("v1 on                 %7.1f ns/iter  (+%.0f%%)\n", s * 1e9 / n1, (s / n1 / (off / n) - 1) * 100);
  }
  { v2::Tracer<> t; double s = workload_s(n, [&](int k) { t.trace(k); });
    std::printf("v2 on                 %7.1f ns/iter  (+%.0f%%)\n", s * 1e9 / n, (s / off - 1) * 100); }
  { v3::Tracer t; double s = workload_s(n, [&](int k) { t.trace(k); });
    std::printf("v3 on                 %7.1f ns/iter  (+%.0f%%)\n", s * 1e9 / n, (s / off - 1) * 100); }
}
