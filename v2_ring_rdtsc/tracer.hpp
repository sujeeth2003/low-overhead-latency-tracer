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
