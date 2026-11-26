// Saved register layout for HomeRolledCoroutines. Keep it in sync with
// home_rolled_coroutines_<arch>.S. Callers allocate contexts but should not
// access their fields directly.

#ifndef HOME_ROLLED_COROUTINES_CONTEXT_H_
#define HOME_ROLLED_COROUTINES_CONTEXT_H_

#include <stdint.h>

struct context {
  // Stack pointer.
  uint64_t sp;
#if defined(__aarch64__) || defined(__arm64__)
  // Link register.
  uint64_t lr;
  // Frame pointer.
  uint64_t fp;
  // Callee-saved general-purpose registers.
  uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
  // Callee-saved floating-point registers and FP control register.
  uint64_t d8_to_d15[8];
  uint64_t fpcr;
#elif defined(__x86_64__) && !defined(__ILP32__)
  // Address at which to resume execution.
  uint64_t ip;
  // Callee-saved general-purpose registers, including the frame pointer rbp.
  uint64_t rbp, rbx, r12, r13, r14, r15;
  // SSE and x87 floating-point control state.
  uint32_t mxcsr;
  uint16_t x87_control;
#else
#error Unsupported HomeRolledCoroutines target ABI
#endif
};

#endif // HOME_ROLLED_COROUTINES_CONTEXT_H_
