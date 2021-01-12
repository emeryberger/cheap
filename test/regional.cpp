#include <iostream>

void doIt() {
  const int num = 2000;
  volatile char * volatile arr[num];
  for (int i = 0; i < num / 2; i++) {
    arr[i] = new char[16];
  }
  for (int i = num / 2; i < num; i++) {
    arr[i] = new char[16]; // i / 100 + 1];
  }
  for (int i = 0; i < num; i++) {
    delete [] arr[i];
  }
}

int
main()
{
  int v;
  std::cout << &v << std::endl;
  for (int i = 0; i < 10; i++) {
    doIt();
  }
  return 0;
}
