#include <malloc.h>
#include <vector>

int main() {
    volatile auto* a = new std::vector<int>;
    volatile auto* b = malloc(16);
    volatile auto* c = calloc(32, 16);
    volatile auto* d = realloc((void*) b, 32);

    delete a;
    free((void*) c);
    free((void*) d);
}