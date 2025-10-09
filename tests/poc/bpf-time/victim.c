// victim.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  while (1) {
    // Repeatedly allocate and free memory
    void* ptr = malloc(1024);
    if (ptr) {
      free(ptr);
    }
    sleep(1);
  }
  return 0;
}
