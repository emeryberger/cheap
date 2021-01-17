INCLUDES = -I../../Heap-Layers -I../.. -I. # -I/usr/include/boost

all:
	clang++ -flto $(INCLUDES) -L../.. -O3 -g -DNDEBUG testboostjson.cpp -o testboostjson
	clang++ -flto -DCHEAPEN=1 $(INCLUDES) -L../.. -O3 -g -DNDEBUG testboostjson.cpp -o testboostjson-cheapened -lcheap

trace:
	clang++ -fno-inline-functions $(INCLUDES) -L../.. -O2 -g -DNDEBUG testboostjson.cpp -o testboostjson-trace

test:
	clang++ -fno-inline-functions -DCHEAPEN=1 $(INCLUDES) -L../.. -O0 -g testboostjson.cpp -o testboostjson-test -lcheap
