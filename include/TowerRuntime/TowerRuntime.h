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

// bool swap(S* s);
void stop(S *s);
bool start(S *s, void *...);
void dump(S *s);
void check_swap(S *s);

} // extern "C"

#endif // TOWER_RUNTIME_H
