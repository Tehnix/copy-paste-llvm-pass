#include <stdio.h>

int addition(int a, int b);

int main() {
  int a = 2;
  int b = 5;
  int ab = addition(a, b);
  printf("Hello World\n");
  printf("%i + %i = %i\n", a, b, ab);
}

int addition(int a, int b) {
  return a + b;
}
