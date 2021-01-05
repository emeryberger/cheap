all:
	clang++ -flto -I../.. -L../.. -O3 -g -DNDEBUG testjson.cpp -o testjson -lcheapen
	clang++ -flto -DCHEAPEN -I../.. -L../.. -O3 -g -DNDEBUG testjson.cpp -o testjson-cheapened -lcheapen
