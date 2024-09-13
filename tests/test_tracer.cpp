#include <chrono>
#include <cstdio>
#include <thread>
#include "../v1_chrono_log/tracer.hpp"
#include "../v2_ring_rdtsc/tracer.hpp"
#include "../v3_thread_local/tracer.hpp"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

