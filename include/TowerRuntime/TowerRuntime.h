#ifndef TOWER_RUNTIME_H
#define TOWER_RUNTIME_H

#include <cstdint>

struct MachineState {
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

struct TowerRuntimeContext {
  MachineState *main;
  MachineState *thread;
  uint64_t count;
  uint64_t limit;
};

extern "C" {

// Intended to be called from main. The thread context should jump into the
// "launchpad" function that calls the generated entrypoint with arguments
// packed in `in` parameter, and set results packed in `out` parameter (note
// that "out" is the name of the argument, though it is an out parameter). See
// CreateShimFunction.cpp for more information on how the input and output
// parameters are munged around.
bool start(TowerRuntimeContext *ctx,
           void (*entry)(TowerRuntimeContext *, void *, void *), void *in,
           void *out);

// Swap from MachineState in `from`, to MachineState in `to`
bool swap(MachineState *from, MachineState *to);

// Intended to be called from the launchpad function, let's the main know thread
// is done.
void stop(TowerRuntimeContext *ctx);

void dump(TowerRuntimeContext *ctx);
void check_swap(TowerRuntimeContext *ctx);

} // extern "C"

#endif // TOWER_RUNTIME_H
