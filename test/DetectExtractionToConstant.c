#include <stdio.h>

int additionWithConstant(int a, int b) {
  int c = 100 * 5;
  return a + b + c;
}

int main() {
  int y = 100 * 5;
  int x = additionWithConstant(5, 10);
  x * y;
}
