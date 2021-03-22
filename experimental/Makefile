BDE = bde
BDL = $(BDE)/groups/bdl
BSL = $(BDE)/groups/bsl

INCLUDES = -I$(BDL)/bdlma -I$(BDL)/bdlscm -I$(BSL)/bsl+bslhdrs -I$(BSL)/bsla -I$(BSL)/bsls -I$(BSL)/bslmf -I$(BSL)/bslscm -I$(BSL)/bslma

LIBROOT = $(BDE)/_build/unix-linux-aarch64-5.10.0-clang-11.0.1-2Target:-opt_exc_mt_64_cpp03/

LIBS = -L$(LIBROOT) -lbdl -lbsl

all:
	clang++ -std=c++14 $(INCLUDES) -O3 -g -DNDEBUG test-shim-allocator.cpp -o test-shim-allocator $(LIBS)
