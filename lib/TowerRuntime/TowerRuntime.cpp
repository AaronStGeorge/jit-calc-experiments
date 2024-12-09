#include "TowerRuntime/TowerRuntime.h"

#include <cassert>
#include <cstdio>

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
  assert(false && "This should be unreachable! If you're here, you probably "
                  "restarted a stoped thread, don't do that.");
}

CPPTowerRuntimeContext::CPPTowerRuntimeContext(
    uint8_t *stack, size_t stackSize, uint64_t limit, uint64_t count,
    void (*launchpad)(TowerRuntimeContext *ctx,
                      void (*)(TowerRuntimeContext *, void *, void *), void *,
                      void *))
    : main{}, // this will be populated by the `start` function.
      thread{
          // stack pointer
          .sp = (uint64_t)&stack[stackSize],

          // set the link register to our setup/launch function. ARM's RET
          // instruction will return (aka jump) to whatever function we set
          // here.  You can't set the link register like this from C, but the
          // co-routine start function is written in assembly.
          .lr = (uint64_t)launchpad,

          // frame pointer. On ARM this should point to where the old frame
          // pointer was pushed on the stack, but we don't really have a calling
          // function so ¯\_(ツ)_/¯
          .fp = 0,

          // It shouldn't matter what we set the callee saved registers to.
          // This is a fresh machine state, we will never "return" to the
          // calling function, we swap back to the `main` context which will
          // have the correct callee saved registers set.
          .x19 = 0,
          .x20 = 0,
          .x21 = 0,
          .x22 = 0,
          .x23 = 0,
          .x24 = 0,
          .x25 = 0,
          .x26 = 0,
          .x27 = 0,
          .x28 = 0,
      },
      ctx{
          .main = &main,
          .thread = &thread,
          .count = count,
          .limit = limit,
      } {}
