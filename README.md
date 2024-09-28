# Low-Overhead Latency Tracer (C++20, x86-64)

How do you time a 50 ns operation when the timer itself costs 100 ns? Five versions of an event tracer, each one cheaper than the last, ending with a measurement of the tracer's own footprint. **The measuring tool can change the thing you measure**, so it has to be measured too.

| Version | Idea | Cost per `trace()` (this machine) |
|---|---|---|
| [v1_chrono_log](v1_chrono_log/tracer.hpp) | `std::chrono` + `std::string` name + mutex + growing vector (baseline) | ~141 ns |
| [v2_ring_rdtsc](v2_ring_rdtsc/tracer.hpp) | Preallocated ring, integer event ids, `rdtsc` with calibration; names resolved offline | ~7.7 ns |
| [v3_thread_local](v3_thread_local/tracer.hpp) | `thread_local` buffer per thread, merged by timestamp offline; no sharing on the hot path | ~10.5 ns |
| [v4_nic_timestamps](v4_nic_timestamps/nic_timestamps.cpp) | `SO_TIMESTAMPING` NIC/kernel receive timestamp vs application timestamp isolates software-stack latency | n/a (Linux tool) |
| [v5_overhead](v5_overhead/overhead_bench.cpp) | Measure tracer overhead itself: per-call cost and how much it slows a fixed workload; `perf`/VTune recipe | - |

## Results
11th-gen Core i5-1135G7, Windows 11, clang 21 `-O2`, 2M calls (v1: 500k). Single run, unpinned; re-run on your machine for numbers you will quote.

```
cost per trace() call        (empty-loop cost subtracted)
  v1 chrono+mutex+str   141.2 ns
  v2 ring + rdtsc         7.7 ns
  v3 thread_local ring   10.5 ns

fixed ~53 ns workload, two trace points per iteration
  tracing off            53.0 ns/iter
  v1 on                 305.6 ns/iter  (+476%)   <- the measurement dominates the thing measured
  v2 on                  61.5 ns/iter  (+16%)
  v3 on                  63.1 ns/iter  (+19%)
```
v3 is slightly *slower* than v2 per call here (an extra thread-local owner check) - its value is being correct and contention-free with many threads, not being faster on one.

## Design notes
- **`rdtsc`, not `clock_gettime`.** No syscall or vDSO call; requires an invariant TSC. Calibrated once against `steady_clock` (`common/tsc.hpp`, error well under 0.1% with the default 50 ms window). `rdtsc` is not serializing: for measuring instruction-level latency add `lfence` or use `rdtscp`.
- **Nothing on the hot path allocates, locks, or formats.** All formatting and name lookup happen offline.
- **Ring buffer** overwrites the oldest events, so tracing can stay on in production and be dumped after an incident.
- **v3** takes a mutex exactly once per thread (buffer registration). Assumes one active tracer per thread.
- **v4** needs a NIC with hardware timestamping (`ethtool -T`); otherwise it reports kernel software timestamps and says so. NIC clocks are typically PTP-domain, so sync clocks or compare deltas. **Compiled for Linux but not run here (no Linux box); treat it as untested at runtime.**
- The tracer's own measurement, especially for `perf stat` counters, is in `v5_overhead/perf_overhead.sh` (Linux) and was not run on this machine.

