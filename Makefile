CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
LDFLAGS  ?= -pthread

all: build/test_tracer build/overhead_bench build/nic_timestamps

build:
	mkdir -p build

build/test_tracer: tests/test_tracer.cpp $(wildcard */*.hpp) | build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

build/overhead_bench: v5_overhead/overhead_bench.cpp $(wildcard */*.hpp) | build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

build/nic_timestamps: v4_nic_timestamps/nic_timestamps.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

test: build/test_tracer
	./build/test_tracer

bench: build/overhead_bench
	./build/overhead_bench 2000000

perf: build/overhead_bench
	sh v5_overhead/perf_overhead.sh

clean:
	rm -rf build
.PHONY: all test bench perf clean
