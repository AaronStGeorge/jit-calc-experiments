#ifndef TOWER_RUNTIME_H
#define TOWER_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

// Keep this layout in sync with tower_runtime_<arch>.S.
struct MachineState {
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
#error Unsupported TowerRuntime target ABI
#endif
};

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

// The assembly uses the first two offsets; MLIR uses the whole layout.
static_assert(offsetof(TowerRuntimeContext, main) == 0);
static_assert(offsetof(TowerRuntimeContext, thread) == 8);
static_assert(offsetof(TowerRuntimeContext, count) == 16);
static_assert(offsetof(TowerRuntimeContext, limit) == 24);
static_assert(sizeof(TowerRuntimeContext) == 32);

// launchpad handles setup for, calling into, and returning from the generated
// code.
void launchpad(TowerRuntimeContext *ctx,
               void (*entry)(TowerRuntimeContext *, void *, void *), void *in,
               void *out);

// `CPPTowerRuntimeContext` provides a nice way to "hold" a
// `TowerRuntimeContext` in C++. As it never ventures into MLIR, we can loose
// the full breadth of C++. Without a tasteful use of SFINAE CRTP
// template<template<template<>> a C++ program really just isn't quite
// :chef-kiss-emoji: ya'know?
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

// Resume a suspended context. Returns false on yield and true on stop.
bool swap(MachineState *from, MachineState *to);

// Intended to be called from the launchpad function, let's the main know thread
// is done.
void stop(TowerRuntimeContext *ctx);

} // extern "C"

#endif // TOWER_RUNTIME_H
