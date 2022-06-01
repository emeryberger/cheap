CXX := clang++
CXXFLAGS := --std=c++17 -fPIC -g -Wall -fvisibility=hidden -O3 -Ivendor/libpfm4/include
LDFLAGS := -fPIC -shared -Lvendor/libpfm4/lib

BUILD_DIRECTORY := build

.PHONY: all
all: $(BUILD_DIRECTORY)/libdetector.so $(BUILD_DIRECTORY)/liblitterer.so $(BUILD_DIRECTORY)/benchmark

.PHONY: clean
clean:
	rm -rf $(BUILD_DIRECTORY)

$(BUILD_DIRECTORY)/libdetector.so: detector.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@ -ldl

$(BUILD_DIRECTORY)/liblitterer.so: litterer.cpp vendor/libpfm4
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@ -ldl -lpfm -DOUTPUT_PERF_DATA

$(BUILD_DIRECTORY)/benchmark: benchmark.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< -std=c++17 -g -Wall -O3 -fno-omit-frame-pointer -o $(BUILD_DIRECTORY)/benchmark

vendor/libpfm4:
	mkdir vendor
	git clone https://github.com/wcohen/libpfm4 $@
	cd $@ && make

.PHONY: format
format:
	clang-format -i detector.cpp litterer.cpp benchmark.cpp constants.hpp