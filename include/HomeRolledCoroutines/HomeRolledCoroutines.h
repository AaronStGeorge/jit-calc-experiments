#ifndef HOME_ROLLED_COROUTINES_H_
#define HOME_ROLLED_COROUTINES_H_

#include <stdbool.h>
#include <stdint.h>

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

#ifdef __cplusplus
extern "C" {
#endif

bool swap(struct context *from, struct context *to);
void stop(struct context *from, struct context *to);
bool start(struct context *from, struct context *to, void *arg_ptr,
           void *return_ptr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOME_ROLLED_COROUTINES_H_
