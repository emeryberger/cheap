SOURCES = libcheaper.cpp libcheap.cpp cheap.h

.PHONY: all format test

all: vendor
	-make -f cheap.mk
	-make -f cheaper.mk

vendor:
	mkdir vendor
	cd vendor && git clone https://github.com/ianlancetaylor/libbacktrace.git
	cd vendor/libbacktrace && ./configure CFLAGS='-arch x86_64 -fPIC' CC='clang' && make
# cd vendor/libbacktrace && ./configure CFLAGS='-arch x86_64 -arch arm64' CC='clang' && make

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:  $(SOURCES) test/regional.cpp testcheapen.cpp
	clang++ -std=c++14 -Wl,-no_pie -O0 -fno-inline -g -fno-inline-functions test/regional.cpp -o regional
	clang++ -std=c++14 -O0 -DTEST -IHeap-Layers -fno-inline-functions -fno-inline -g testcheapen.cpp -o testcheapen-trace
	clang++ -std=c++14 -flto -O3 -g -IHeap-Layers -DNDEBUG -DCHEAPEN=1 testcheapen.cpp -o testcheapen-cheapen -lpthread -L. -lcheap
	clang++ -std=c++14 -fno-inline-functions -fno-inline -O0 -g -IHeap-Layers -DCHEAPEN=1 testcheapen.cpp -o testcheapen-cheapen-debug -lpthread -L. -lcheap
	clang++ -std=c++14 -flto -O3 -g -IHeap-Layers -DNDEBUG testcheapen.cpp -o testcheapen -lpthread
