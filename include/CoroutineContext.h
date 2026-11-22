#ifndef JIT_CALC_COROUTINE_CONTEXT_H_
#define JIT_CALC_COROUTINE_CONTEXT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Register layout shared by the two coroutine APIs and their assembly backend.
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
#error Unsupported coroutine target ABI
#endif
};

static inline void coroutine_init_context(struct context *ctx, void *stack,
                                          size_t stack_size, uintptr_t entry) {
#ifdef __cplusplus
  *ctx = {};
#else
  *ctx = (struct context){0};
#endif
  ctx->sp = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
#if defined(__aarch64__) || defined(__arm64__)
  ctx->lr = entry;
#else
  ctx->ip = entry;
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

// Resume a suspended context. Returns false on yield and true on stop.
bool swap(struct context *from, struct context *to);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // JIT_CALC_COROUTINE_CONTEXT_H_
