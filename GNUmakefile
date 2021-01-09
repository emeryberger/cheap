SOURCES = libcheaper.cpp libcheap.cpp cheap.h

.PHONY: all format test

all:
	-make -f cheap.mk
	-make -f cheaper.mk

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:  $(SOURCES) test/regional.cpp
	clang++ -O3 -g -fno-inline test/regional.cpp
