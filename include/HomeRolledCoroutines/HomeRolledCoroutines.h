// C API and saved register layout for standalone cooperative coroutines.

#ifndef HOME_ROLLED_COROUTINES_H_
#define HOME_ROLLED_COROUTINES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Keep this layout in sync with home_rolled_coroutines_<arch>.S.
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

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*coroutine_entry)(struct context *caller, struct context *self,
                                void *arg_ptr, void *return_ptr);

// The caller owns the stack and must keep it alive until the coroutine stops.
// Supply a buffer large enough for the entry function and everything it calls.
// Contexts are cooperative and must stay on the same OS thread.
static inline void coroutine_init(struct context *ctx, void *stack,
                                  size_t stack_size, coroutine_entry entry) {
#ifdef __cplusplus
  *ctx = {};
#else
  *ctx = (struct context){0};
#endif
  ctx->sp = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
#if defined(__aarch64__) || defined(__arm64__)
  ctx->lr = (uintptr_t)entry;
#else
  ctx->ip = (uintptr_t)entry;
#endif
}

// Resume a suspended context. Returns false on yield and true on stop.
bool swap(struct context *from, struct context *to);

// Start a fresh context. Returns false on yield and true on stop.
bool start(struct context *from, struct context *to, void *arg_ptr,
           void *return_ptr);
// Entry functions must stop instead of returning. Do not resume a stopped context.
void stop(struct context *from, struct context *to);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOME_ROLLED_COROUTINES_H_
