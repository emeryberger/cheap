// run with 'unbuffer'!

#include <iostream>

void doIt() {
  const int num = 1000;
  char * arr[num];
  for (int i = 0; i < num; i++) {
    arr[i] = new char[16];
  }
  for (int i = 0; i < num; i++) {
    delete [] arr[i];
  }
  for (int i = 0; i < num; i++) {
    arr[i] = new char[16];
  }
  for (int i = 0; i < num; i++) {
    delete [] arr[i];
  }
}

int
main()
{
  //  std::cout << "SUP\n";
  for (int i = 0; i < 10; i++) {
    doIt();
  }
  return 0;
}
