#include "TowerRuntime/TowerRuntime.h"

#include <cstdlib>

// launchpad handles setup for, calling into, and returning from the generated
// code.
void launchpad(TowerRuntimeContext *ctx,
               void (*entry)(TowerRuntimeContext *, void *, void *), void *in,
               void *out) {
  // Swap out immediately, the first call is just to get things set up so we can
  // call into game code with a simple `swap` later rather than needing `start`
  // with the arguments and stuff.
  swap(ctx->thread, ctx->main);

  entry(ctx, in, out);

  stop(ctx);
  std::abort(); // A stopped coroutine must not be resumed.
}

CPPTowerRuntimeContext::CPPTowerRuntimeContext(
    uint8_t *stack, size_t stackSize, uint64_t limit, uint64_t count,
    void (*launchpad)(TowerRuntimeContext *ctx,
                      void (*)(TowerRuntimeContext *, void *, void *), void *,
                      void *))
    : main{}, thread{}, ctx{&main, &thread, count, limit} {
  thread.sp = (reinterpret_cast<uintptr_t>(stack) + stackSize) & ~uintptr_t{15};
#if defined(__aarch64__) || defined(__arm64__)
  thread.lr = reinterpret_cast<uintptr_t>(launchpad);
#else
  thread.ip = reinterpret_cast<uintptr_t>(launchpad);
#endif
}
