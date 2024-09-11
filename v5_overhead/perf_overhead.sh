#!/bin/sh
# Linux: confirm the tracer's cost with hardware counters instead of trusting
# wall-clock alone. Compare instructions, branch-misses and cache-misses between
# the tracing-off run and each tracer. For Intel VTune:
#   vtune -collect hotspots -result-dir r001 -- ./build/overhead_bench
# then look at where the traced workload spends its cycles.
set -e
[ -x build/overhead_bench ] || make build/overhead_bench
perf stat -e instructions,cycles,branch-misses,cache-misses,L1-dcache-load-misses \
  ./build/overhead_bench 2000000
