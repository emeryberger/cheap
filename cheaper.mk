PYTHON = python3
SOURCES = libcheaper.cpp libcheapen.cpp printf.cpp
LIBNAME = cheaper
include heaplayers-make.mk

.PHONY: format test

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:	$(SOURCES) test/regional.cpp
	clang++ -O3 -g -fno-inline test/regional.cpp
