INCLUDES = -I../../Heap-Layers -I../..
# CXXFLAGS = -O0 -g -lpthread
CXXFLAGS = -std=c++20 -flto -O3 -g -DNDEBUG -lpthread
CXXFLAGS_DEBUG = -std=c++20 -O2 -g -lpthread

all:
	clang++ $(CXXFLAGS) $(INCLUDES) -L../..  testrapidjson.cpp -o testrapidjson
	clang++ -DCHEAPEN=1 $(CXXFLAGS) $(INCLUDES) -L../.. testrapidjson.cpp -o testrapidjson-cheapened -lcheap

trace:
	clang++ -fno-inline-functions $(INCLUDES) -L../.. $(CXXFLAGS_DEBUG) testrapidjson.cpp -o testrapidjson-trace
