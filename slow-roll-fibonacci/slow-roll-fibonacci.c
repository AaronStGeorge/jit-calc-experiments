#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

static

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

    struct t {
  ucontext_t uctx_main, uctx_func1, uctx_func2;
};

static void func1(struct t *thing) {
  printf("%s: started\n", __func__);
  printf("%s: swapcontext(&uctx_func1, &uctx_func2)\n", __func__);
  if (swapcontext(&thing->uctx_func1, &thing->uctx_func2) == -1)
    handle_error("swapcontext");
  printf("%s: returning\n", __func__);
}

static void cleanup(struct t *thing) {
  printf("%s: started\n", __func__);
  printf("%s: swapcontext(&uctx_func2, &uctx_func1)\n", __func__);
  if (swapcontext(&thing->uctx_func2, &thing->uctx_func1) == -1)
    handle_error("swapcontext");
  printf("%s: returning\n", __func__);
}

int main(int argc, char *argv[]) {
  char func1_stack[16384];
  char func2_stack[16384];

  struct t thing;

  if (getcontext(&thing.uctx_func1) == -1)
    handle_error("getcontext");
  thing.uctx_func1.uc_stack.ss_sp = func1_stack;
  thing.uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
  thing.uctx_func1.uc_link = &thing.uctx_main;
  makecontext(&thing.uctx_func1, (void(*))func1, 1, &thing);

  if (getcontext(&thing.uctx_func2) == -1)
    handle_error("getcontext");
  thing.uctx_func2.uc_stack.ss_sp = func2_stack;
  thing.uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
  /* Successor context is f1(), unless argc > 1 */
  thing.uctx_func2.uc_link = (argc > 1) ? NULL : &thing.uctx_func1;
  makecontext(&thing.uctx_func2, (void(*))cleanup, 1, &thing);

  printf("%s: swapcontext(&uctx_main, &uctx_func2)\n", __func__);
  if (swapcontext(&thing.uctx_main, &thing.uctx_func2) == -1)
    handle_error("swapcontext");

  printf("%s: exiting\n", __func__);
  exit(EXIT_SUCCESS);
}