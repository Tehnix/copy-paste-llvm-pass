#include <stdio.h>

void ifAndElse(int n) {
  if (n < 10) {
    printf("Oh no!");
  }
  if (n > 10) {
    printf("Hey!");
  } else {
    printf("Hey!");
  }
}

void ifAndElse2(int n) {
  if (n < 10) {
    printf("Oh no!");
  }
  if (n > 10) {
    printf("Hey!");
  } else {
    printf("Hey!");
  }
}

int main() {
  ifAndElse(20);
  ifAndElse2(18);
}
