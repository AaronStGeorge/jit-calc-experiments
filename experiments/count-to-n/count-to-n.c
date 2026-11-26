// Demonstrate HomeRolledCoroutines by yielding between counts and returning
// the final count to the caller.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "HomeRolledCoroutines/HomeRolledCoroutines.h"

static void thread_fn(struct context *main_ctx, struct context *thread_ctx,
                      void *arg_ptr, void *return_ptr) {
  int n = *(int *)arg_ptr;
  int i = 0;
  for (; i < n; i++) {
    printf("in thread i=%d\n", i);
    swap(thread_ctx, main_ctx);
  }
  printf("in thread i=%d\n", i);
  *(int *)return_ptr = n;
  stop(thread_ctx, main_ctx);
}

int main(int argc, char **argv) {

  struct context main_ctx;
  struct context thread_ctx;

  printf("Starting main\n");
  size_t stack_size = 64 * 1024;
  void *stack = malloc(stack_size);
  if (!stack) {
    perror("malloc coroutine stack");
    return EXIT_FAILURE;
  }
  coroutine_init(&thread_ctx, stack, stack_size, thread_fn);

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
  free(stack);
}