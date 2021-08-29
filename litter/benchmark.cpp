#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>

#define LOOPS 10
#define ITERATIONS 200000
#define LOCALITY_ITERATIONS 500
#define OBJECT_SIZE 16
#define BYTES_TO_READ 1

int main() {
  void** Objects = new void*[ITERATIONS];

  std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < LOOPS; i++) {
    for (int j = 0; j < ITERATIONS; j++) {
      void* Pointer = malloc(OBJECT_SIZE);
      memset(Pointer, 13, OBJECT_SIZE);
      Objects[j] = Pointer;
    }
    
    volatile char Counter;
    for (int i = 0; i < LOCALITY_ITERATIONS; i++) {
      for (int j = 0; j < ITERATIONS; j++) {
        volatile char Buffer[OBJECT_SIZE];
        memcpy((void*) Buffer, Objects[j], BYTES_TO_READ);
        Counter += Buffer[OBJECT_SIZE - 1];
      }
    }

    for (int i = 0; i < ITERATIONS; ++i) {
        free(Objects[i]);
    }
  }

  delete[] Objects;

  std::chrono::high_resolution_clock::time_point EndTime = std::chrono::high_resolution_clock::now();
  std::cout << (EndTime - StartTime).count() << std::endl;
}