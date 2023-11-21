#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "HomeRolledCoroutines/HomeRolledCoroutines.h"

struct context {
  uint64_t rsp;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbx;
  uint64_t rbp;
};


void thread_fn(struct context *main_ctx, struct context *thread_ctx,
               int *arg_ptr, int *return_ptr) {
  int n = *arg_ptr;
  int i = 0;
  for (; i < n; i++) {
    printf("in thread i=%d\n", i);
    swap(thread_ctx, main_ctx);
  }
  printf("in thread i=%d\n", i);
  *return_ptr = n;
  stop(thread_ctx, main_ctx);
}

int main(int argc, char **argv) {

  struct context main_ctx;
  struct context thread_ctx;

  printf("Starting main\n");
  size_t stack_size = 4 * 1024;
  char *stack = calloc(stack_size, sizeof(char));
  *(uint64_t *)&stack[stack_size - 16] = (uint64_t)thread_fn;

  thread_ctx.rsp = (uint64_t)&stack[stack_size - 16];

  int swaps = 9;
  int return_val = -1;
  int i = 0;
  bool done = start(&main_ctx, &thread_ctx, &swaps, &return_val);
  printf("in main i=%d\n", i);
  while (!done) {
    i++;
    done = swap(&main_ctx, &thread_ctx);
    printf("in main i=%d\n", i);
  }

  printf("Exiting from main, return_val=%d\n", return_val);
}