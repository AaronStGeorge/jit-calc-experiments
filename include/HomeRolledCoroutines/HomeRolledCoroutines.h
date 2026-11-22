#ifndef HOME_ROLLED_COROUTINES_H_
#define HOME_ROLLED_COROUTINES_H_

#include "CoroutineContext.h"

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
  coroutine_init_context(ctx, stack, stack_size, (uintptr_t)entry);
}

// Start a fresh context. Returns false on yield and true on stop.
bool start(struct context *from, struct context *to, void *arg_ptr,
           void *return_ptr);
// Entry functions must stop instead of returning. Do not resume a stopped context.
void stop(struct context *from, struct context *to);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOME_ROLLED_COROUTINES_H_
