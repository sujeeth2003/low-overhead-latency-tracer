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

