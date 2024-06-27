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
