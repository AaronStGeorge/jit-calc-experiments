#ifndef TOWER_RUNTIME_H
#define TOWER_RUNTIME_H

#include <cstddef>
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

struct TowerRuntimeContext;

// launchpad handles setup for, calling into, and returning from the generated
// code.
void launchpad(TowerRuntimeContext *ctx,
               void (*entry)(TowerRuntimeContext *, void *, void *), void *in,
               void *out);

// `TowerRuntimeContext` must be ABI compatible with the MLIR type
// `!tower_runtime.ctx`. MLIR assumes `!tower_runtime.ctx` is laid out as a blob
// with two pointers (main, thread) then two 64-bit integers (count, limit). As
// long as `TowerRuntimeContext` is a POD type C++ guarantees the expected
// layout; assuming no one changes the order of items in the struct. A POD type
// has no constructors, destructors, or really any C++ feature. If you use a C++
// feature the compiler is free to lay the type out any damn way it wants, which
// won't be the way MLIR wants (probably).
struct TowerRuntimeContext {
  MachineState *main;
  MachineState *thread;
  uint64_t count;
  uint64_t limit;
};

// `CPPTowerRuntimeContext` provides a nice way to "hold" a
// `TowerRuntimeContext` in C++. It isn't sent into MLIR, so we can use any C++
// feature we want.
struct CPPTowerRuntimeContext {
  MachineState main;
  MachineState thread;
  TowerRuntimeContext ctx;

  CPPTowerRuntimeContext(uint8_t *stack, size_t stackSize, uint64_t limit,
                         uint64_t count = 0,
                         void (*launch)(TowerRuntimeContext *,
                                        void (*)(TowerRuntimeContext *, void *,
                                                 void *),
                                        void *, void *) = launchpad);
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

} // extern "C"

#endif // TOWER_RUNTIME_H
