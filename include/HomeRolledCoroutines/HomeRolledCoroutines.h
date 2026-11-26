// C API for standalone cooperative coroutines.

#ifndef HOME_ROLLED_COROUTINES_H_
#define HOME_ROLLED_COROUTINES_H_

#include <stdbool.h>
#include <stddef.h>

#include "CoroutineContext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*coroutine_entry)(struct context *caller, struct context *self,
                                void *arg_ptr, void *return_ptr);

// Prepare a context on a caller-owned stack large enough for the entry function
// and its callees. Keep the stack alive until the coroutine stops, and use the
// context only on the same OS thread.
void coroutine_init(struct context *ctx, void *stack, size_t stack_size,
                    coroutine_entry entry);

// Start a fresh context. Returns false on yield and true on stop.
bool start(struct context *from, struct context *to, void *arg_ptr,
           void *return_ptr);

// Resume a suspended context. Returns false on yield and true on stop.
bool swap(struct context *from, struct context *to);

// Finish the current coroutine and resume its caller. An entry function must
// call stop instead of returning. Do not resume a stopped context.
void stop(struct context *from, struct context *to);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOME_ROLLED_COROUTINES_H_
