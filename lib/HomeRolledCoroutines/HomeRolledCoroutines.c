// Initialize a fresh HomeRolledCoroutines context for the assembly switcher.

#include "HomeRolledCoroutines/HomeRolledCoroutines.h"

void coroutine_init(struct context *ctx, void *stack, size_t stack_size,
                    coroutine_entry entry) {
  *ctx = (struct context){0};
  ctx->sp = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
#if defined(__aarch64__) || defined(__arm64__)
  ctx->lr = (uintptr_t)entry;
#else
  ctx->ip = (uintptr_t)entry;
#endif
}
