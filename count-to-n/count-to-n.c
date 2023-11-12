#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

struct context {
  uint64_t rsp;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbx;
  uint64_t rbp;
};

static struct context main_ctx;
static struct context thread_ctx;

bool swap(struct context *from, struct context *to);
void stop(struct context *from, struct context *to);
bool start(struct context *from, struct context *to, void *arg_ptr);

void thread_fn(int *arg_ptr) {
  int i = 0;
  for (; i < *arg_ptr; i++) {
    printf("in thread i=%d\n", i);
    swap(&thread_ctx, &main_ctx);
  }
  printf("in thread i=%d\n", i);
  stop(&thread_ctx, &main_ctx);
}

int main(int argc, char **argv) {
  printf("Starting main\n");
  size_t stack_size = 4 * 1024;
  char *stack = calloc(stack_size, sizeof(char));
  *(uint64_t *)&stack[stack_size - 16] = (uint64_t)thread_fn;

  thread_ctx.rsp = (uint64_t)&stack[stack_size - 16];

  int swaps = 9;
  int i = 0;
  bool s = start(&main_ctx, &thread_ctx, &swaps);
  printf("in main i=%d\n", i);
  while (!s) {
    i++;
    s = swap(&main_ctx, &thread_ctx);
    printf("in main i=%d\n", i);
  }

  printf("Exiting from main\n");
}