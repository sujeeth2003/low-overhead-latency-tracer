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

