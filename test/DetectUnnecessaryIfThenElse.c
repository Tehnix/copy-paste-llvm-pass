#include <stdio.h>

int addTwoNo(a, b) {
  return a + b;
}

void ifAndElse(int n) {
  if (n < 10) {
    addTwoNo(2, 2);
  }
  if (n > 10) {
    addTwoNo(3, 3);
  } else {
    addTwoNo(3, 3);
  }
}

int main() {
  ifAndElse(20);
}
