all:
	clang++ -flto -I../.. -L../.. -O3 -g -DNDEBUG testjson.cpp -o testjson
	clang++ -flto -DCHEAPEN -I../.. -L../.. -O3 -g -DNDEBUG testjson.cpp -o testjson-cheapened -lcheap

trace:
	clang++ -fno-inline-functions -I../.. -L../.. -O2 -g -DNDEBUG testjson.cpp -o testjson-trace
