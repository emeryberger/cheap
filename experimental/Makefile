BDE = bde
BDL = $(BDE)/groups/bdl
BSL = $(BDE)/groups/bsl

INCLUDES = -I$(BDL)/bdlma -I$(BDL)/bdlscm -I$(BSL)/bsl+bslhdrs -I$(BSL)/bsla -I$(BSL)/bsls -I$(BSL)/bslmf -I$(BSL)/bslscm -I$(BSL)/bslma -I$(BSL)/bslstl -I$(BSL)/bslh -I$(BSL)/bslalg

# You will need to change this once BDE is built. Would be nice if this was easy to add automatically.

# LIBROOT = $(BDE)/_build/unix-darwin-x86_64-19.6.0-gcc-default-9-opt_exc_mt_64_cpp14
# LIBROOT = $(BDE)/_build/unix-linux-aarch64-5.10.0-clang-11.0.1-2Target:-opt_exc_mt_64_cpp03/
# LIBROOT = $(BDE)/_build/unix-darwin-arm64-20.3.0-clang-12.0.0-opt_exc_mt_64_cpp03
LIBROOT = $(BDE)/_build/unix-darwin-arm64-20.5.0-clang-12.0.5-opt_exc_mt_64_cpp17

LIBS = -L$(LIBROOT) -lbdl -lbsl -lbsls
CXXFLAGS = -std=c++14 $(INCLUDES) -g -O3 -DNDEBUG # -fsanitize=address

all: test-buffer-manager

test-buffer-manager: bde test-shim-buffermanager.cpp Makefile shim_buffermanager.hpp simregion.hpp
	clang++ $(CXXFLAGS) test-shim-buffermanager.cpp -o test-shim-buffermanager $(LIBS)

other:
	clang++ -DUSE_MALLOC=1 $(CXXFLAGS) test-shim-allocator.cpp -o test-malloc-allocator-shuffled $(LIBS)
	clang++ -DUSE_MALLOC=0 $(CXXFLAGS) test-shim-allocator.cpp -o test-shim-allocator-shuffled $(LIBS)
	clang++ -DUSE_SHUFFLE=0 -DUSE_MALLOC=1 $(CXXFLAGS) test-shim-allocator.cpp -o test-malloc-allocator $(LIBS)
	clang++ -DUSE_SHUFFLE=0 -DUSE_MALLOC=0 $(CXXFLAGS) test-shim-allocator.cpp -o test-shim-allocator $(LIBS)

PATH := $(PWD)/bde-tools/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

bde:	bde-tools
	git clone https://github.com/emeryberger/bde # fork of https://github.com/bloomberg/bde.git with patch for M1
	cd bde && eval `bde_build_env.py --build-type=Release` && cmake_build.py configure && cmake_build.py build

bde-tools:
	git clone https://github.com/bloomberg/bde-tools.git
