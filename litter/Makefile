CXX := clang++
CXXFLAGS := --std=c++20 -fPIC -g -Wall -fvisibility=hidden -O3
LDFLAGS := -fPIC -shared

BUILD_DIRECTORY := build

.PHONY: all
all: detector litterer benchmark

.PHONY: detector
detector: detector.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $(CXXFLAGS) $(USER_FLAGS) -c -o $(BUILD_DIRECTORY)/detector.o detector.cpp
	$(CXX) $(LDFLAGS) -ldl -o $(BUILD_DIRECTORY)/libdetector.so $(BUILD_DIRECTORY)/detector.o

.PHONY: litterer
litterer: litterer.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $(CXXFLAGS) $(USER_FLAGS) -c -o $(BUILD_DIRECTORY)/litterer.o litterer.cpp
	$(CXX) $(LDFLAGS) -o $(BUILD_DIRECTORY)/liblitterer.so $(BUILD_DIRECTORY)/litterer.o

.PHONY: benchmark
benchmark:
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) -std=c++20 -g -Wall -O3 -fno-omit-frame-pointer -o $(BUILD_DIRECTORY)/benchmark benchmark.cpp
