PYTHON = python3
SOURCES = libcheaper.cpp libcheapen.cpp cheapen.h
LIBNAME = cheapen

include heaplayers-make.mk

.PHONY: format test

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:	$(SOURCES) test/regional.cpp
	clang++ -O3 -g -fno-inline test/regional.cpp
