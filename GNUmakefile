SOURCES = libcheaper.cpp libcheapen.cpp cheapen.h

.PHONY: all format test

all:
	-make -f cheapen.mk
	-make -f cheaper.mk

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:  $(SOURCES) test/regional.cpp
	clang++ -O3 -g -fno-inline test/regional.cpp
