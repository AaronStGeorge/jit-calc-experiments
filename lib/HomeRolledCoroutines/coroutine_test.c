#include "HomeRolledCoroutines/HomeRolledCoroutines.h"

#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);            \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

static void run(struct context *caller, struct context *self, void *arg,
                void *result) {
  int count = *(int *)arg;
  volatile int i = 0; // Keep live data on the coroutine's stack across yields.
  CHECK(fegetround() == FE_DOWNWARD);
  CHECK(fesetround(FE_UPWARD) == 0);
  for (; i < count; ++i) {
    *(int *)result = i;
    CHECK(!swap(self, caller));
    CHECK(fegetround() == FE_UPWARD);
  }
  *(int *)result = i;
  stop(self, caller);
  CHECK(0); // The caller must not resume a stopped coroutine.
}

int main(void) {
  struct context caller, coroutine;
  char stack[64 * 1024 + 1];
  int original_round = fegetround();
  CHECK(fesetround(FE_DOWNWARD) == 0);

  for (int count = 0; count <= 3; count += 3) {
    int result = -1;
    coroutine_init(&coroutine, stack, sizeof(stack), run);
    bool done = start(&caller, &coroutine, &count, &result);
    for (int i = 0; i < count; ++i) {
      CHECK(!done);
      CHECK(result == i);
      CHECK(fegetround() == FE_DOWNWARD);
      done = swap(&caller, &coroutine);
    }
    CHECK(done);
    CHECK(result == count);
    CHECK(fegetround() == FE_DOWNWARD);
  }

  CHECK(fesetround(original_round) == 0);
  puts("HomeRolledCoroutines: start, yield/resume, stop, and FP state passed");
}
