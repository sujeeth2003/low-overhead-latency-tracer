# Low-Overhead Latency Tracer (C++20, x86-64)

How do you time a 50 ns operation when the timer itself costs 100 ns? Five versions of an event tracer, each one cheaper than the last, ending with a measurement of the tracer's own footprint. **The measuring tool can change the thing you measure**, so it has to be measured too.

| Version | Idea | Cost per `trace()` (this machine) |
|---|---|---|
| [v1_chrono_log](v1_chrono_log/tracer.hpp) | `std::chrono` + `std::string` name + mutex + growing vector (baseline) | ~141 ns |
| [v2_ring_rdtsc](v2_ring_rdtsc/tracer.hpp) | Preallocated ring, integer event ids, `rdtsc` with calibration; names resolved offline | ~7.7 ns |
| [v3_thread_local](v3_thread_local/tracer.hpp) | `thread_local` buffer per thread, merged by timestamp offline; no sharing on the hot path | ~10.5 ns |
| [v4_nic_timestamps](v4_nic_timestamps/nic_timestamps.cpp) | `SO_TIMESTAMPING` NIC/kernel receive timestamp vs application timestamp isolates software-stack latency | n/a (Linux tool) |
| [v5_overhead](v5_overhead/overhead_bench.cpp) | Measure tracer overhead itself: per-call cost and how much it slows a fixed workload; `perf`/VTune recipe | - |

