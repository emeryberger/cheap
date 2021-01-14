SOURCES = libcheaper.cpp libcheap.cpp cheap.h

.PHONY: all format test

all:
	-make -f cheap.mk
	-make -f cheaper.mk

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:  $(SOURCES) test/regional.cpp testcheapen.cpp
	clang++ -O3 -g -fno-inline-functions test/regional.cpp
	clang++ -O0 -DTEST -IHeap-Layers -fno-inline-functions -fno-inline -g testcheapen.cpp -o testcheapen-trace
	# clang++ -fno-inline-functions -fno-inline -O0 -g -DCHEAPEN=1 testcheapen.cpp -o testcheapen-cheapen -lpthread -L. -lcheap
	clang++ -flto -O3 -IHeap-Layers -DNDEBUG -DCHEAPEN=1 testcheapen.cpp -o testcheapen-cheapen -lpthread -L. -lcheap
	clang++ -flto -O3 -g -IHeap-Layers -DNDEBUG testcheapen.cpp -o testcheapen -lpthread
