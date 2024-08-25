#ifndef TOWER_RUNTIME_H
#define TOWER_RUNTIME_H

#include <cstdint>

struct context {
  // stack pointer
  uint64_t sp;
  // link register
  uint64_t lr;
  // frame pointer
  uint64_t fp;
  // callee-saved registers
  uint64_t x19;
  uint64_t x20;
  uint64_t x21;
  uint64_t x22;
  uint64_t x23;
  uint64_t x24;
  uint64_t x25;
  uint64_t x26;
  uint64_t x27;
  uint64_t x28;
};

typedef struct {
  struct context *main;
  struct context *thread;
  uint64_t count;
  uint64_t limit;
} S;

extern "C" {

// Intended to be called from main. The passed context should jump into the
// "launchpad" function that expects the generated entrypoint as an argument.
bool start(S *s, void (*entry)(S *));
// Intended to be called from main, swaps into previously paused thread.
bool swap_to_thread(S* s);

// Intended to be called from the launchpad function, let's the main know thread
// is done.
void stop(S *s);
// Intended to be called from thread, let's the main know thread has used up its
// cycles but it still has work to do.
void swap_to_main(S* s);

void dump(S *s);
void check_swap(S *s);

} // extern "C"

#endif // TOWER_RUNTIME_H
