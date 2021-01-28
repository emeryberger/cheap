FLAGS = -std=c++17

all:
	clang++ $(FLAGS) -flto -I../.. -L../.. -O3 -g -DNDEBUG testjson.cpp -o testjson
	clang++ $(FLAGS) -flto -DCHEAPEN=1 -I../../Heap-Layers -I../.. -L../.. -O3 -g -DNDEBUG testjson.cpp -o testjson-cheapened -lcheap

trace:
	clang++ $(FLAGS) -fno-inline-functions -I../.. -L../.. -O2 -g -DNDEBUG testjson.cpp -o testjson-trace
