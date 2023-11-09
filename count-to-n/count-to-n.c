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

void swap(struct context *from, struct context *to);

void thread_fn() {
  printf("Hello from thread\n");
  swap(&thread_ctx, &main_ctx);
}

int main(int argc, char **argv) {
  size_t stack_size = 4 * 1024;
  char *stack = calloc(stack_size, sizeof(char));
  *(uint64_t *)&stack[stack_size - 16] = (uint64_t)thread_fn;

  thread_ctx.rsp = (uint64_t)&stack[stack_size - 16];

  printf("Hello from main\n");

  swap(&main_ctx, &thread_ctx);

  printf("Hello from main again\n");
}