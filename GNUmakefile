LIBNAME = cheaper
PYTHON = python3
SOURCES = libcheaper.cpp
include heaplayers-make.mk

format: $(SOURCES)
	clang-format -i $(SOURCES)
	black cheaper.py

test:	$(SOURCES) test/regional.cpp
	clang++ -O3 -g -fno-inline test/regional.cpp
