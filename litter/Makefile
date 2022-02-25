CXX := clang++
CXXFLAGS := --std=c++20 -fPIC -g -Wall -fvisibility=hidden -O3
LDFLAGS := -fPIC -shared

BUILD_DIRECTORY := build

.PHONY: all
all: $(BUILD_DIRECTORY)/libdetector.so $(BUILD_DIRECTORY)/liblitterer.so $(BUILD_DIRECTORY)/liblitterer-allocate.so $(BUILD_DIRECTORY)/benchmark

$(BUILD_DIRECTORY)/libdetector.so: detector.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@ -ldl

$(BUILD_DIRECTORY)/liblitterer.so: litterer.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@ -ldl

$(BUILD_DIRECTORY)/benchmark:
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) -std=c++20 -g -Wall -O3 -fno-omit-frame-pointer -o $(BUILD_DIRECTORY)/benchmark benchmark.cpp
