INCLUDES = -I../.. -I../../Heap-Layers -I. # -I/usr/include/boost
CXXFLAGS = -std=c++20 -g

all:
	clang++ -flto $(INCLUDES) -L../.. $(CXXFLAGS) -O3 -DNDEBUG testboostjson.cpp -o testboostjson
	clang++ -flto -DCHEAPEN=1 $(INCLUDES) -L../.. $(CXXFLAGS) -O3 -DNDEBUG testboostjson.cpp -o testboostjson-cheapened -lcheap

trace:
	clang++ -fno-inline-functions $(INCLUDES) -L../.. $(CXXFLAGS) -O2 -DNDEBUG testboostjson.cpp -o testboostjson-trace

test:
	clang++ -fno-inline-functions -DCHEAPEN=1 $(INCLUDES) -L../.. $(CXXFLAGS) -O0 testboostjson.cpp -o testboostjson-test -lcheap
