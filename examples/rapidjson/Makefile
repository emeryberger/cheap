all:
	clang++ -flto -I../.. -L../.. -O3 -g -DNDEBUG testrapidjson.cpp -o testrapidjson
	clang++ -flto -DCHEAPEN=1 -I../.. -I../../Heap-Layers -L../.. -O3 -g -DNDEBUG testrapidjson.cpp -o testrapidjson-cheapened -lcheap

trace:
	clang++ -fno-inline-functions -I../.. -L../.. -O2 -g -DNDEBUG testrapidjson.cpp -o testrapidjson-trace
