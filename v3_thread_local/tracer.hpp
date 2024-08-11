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
