BDE = bde
BDL = $(BDE)/groups/bdl
BSL = $(BDE)/groups/bsl

INCLUDES = -I$(BDL)/bdlma -I$(BDL)/bdlscm -I$(BSL)/bsl+bslhdrs -I$(BSL)/bsla -I$(BSL)/bsls -I$(BSL)/bslmf -I$(BSL)/bslscm -I$(BSL)/bslma

LIBROOT = $(BDE)/_build/unix-linux-aarch64-5.10.0-clang-11.0.1-2Target:-opt_exc_mt_64_cpp03/

LIBS = -L$(LIBROOT) -lbdl -lbsl
CXXFLAGS = -std=c++14 $(INCLUDES) -O3 -g -DNDEBUG

all:
	clang++ -DUSE_MALLOC=1 $(CXXFLAGS) test-shim-allocator.cpp -o test-malloc-allocator-shuffled $(LIBS)
	clang++ -DUSE_MALLOC=0 $(CXXFLAGS) test-shim-allocator.cpp -o test-shim-allocator-shuffled $(LIBS)
	clang++ -DUSE_SHUFFLE=0 -DUSE_MALLOC=1 $(CXXFLAGS) test-shim-allocator.cpp -o test-malloc-allocator $(LIBS)
	clang++ -DUSE_SHUFFLE=0 -DUSE_MALLOC=0 $(CXXFLAGS) test-shim-allocator.cpp -o test-shim-allocator $(LIBS)
