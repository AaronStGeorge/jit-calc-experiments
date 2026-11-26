// Saved register layout for HomeRolledCoroutines. Keep it in sync with
// home_rolled_coroutines_<arch>.S. Callers allocate contexts but should not
// access their fields directly.

#ifndef HOME_ROLLED_COROUTINES_CONTEXT_H_
#define HOME_ROLLED_COROUTINES_CONTEXT_H_

#include <stdint.h>

struct context {
  uint64_t sp;
#if defined(__aarch64__) || defined(__arm64__)
  uint64_t lr;
  uint64_t fp;
  uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
  uint64_t d8_to_d15[8];
  uint64_t fpcr;
#elif defined(__x86_64__) && !defined(__ILP32__)
  uint64_t ip;
  uint64_t rbp, rbx, r12, r13, r14, r15;
  uint32_t mxcsr;
  uint16_t x87_control;
#else
#error Unsupported HomeRolledCoroutines target ABI
#endif
};

#endif // HOME_ROLLED_COROUTINES_CONTEXT_H_
