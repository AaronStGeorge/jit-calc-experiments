#include "TowerRuntime/TowerRuntime.h"

#include "gtest/gtest.h"


namespace {
extern "C" void simple_tick_shim(TowerRuntimeContext *, void *, void *);

TEST(IntegrationTests, Simple) {
  // arguments to player code
  int input = true;
  int output = -1;

  size_t stackSize = 8 * 1024;
  auto stack = (uint8_t *)calloc(stackSize, sizeof(char));
  CPPTowerRuntimeContext trctx(stack, stackSize, /*limit*/ 256);

  // Start uses pointers to make the ABI simpler, the function is
  // implemented in assembly so such things matter.
  start(&trctx.ctx, simple_tick_shim, &input, &output);

  ASSERT_TRUE(swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread));

  ASSERT_EQ(output, 1);

  free(stack);
}

extern "C" void add_tick_shim(TowerRuntimeContext *, void *, void *);
struct Thing {
  int a;
  int b;
};

TEST(IntegrationTests, Add) {
  // arguments to player code
  Thing input = {.a = 60, .b = 9};
  int output = -1;

  size_t stackSize = 8 * 1024;
  auto stack = (uint8_t *)calloc(stackSize, sizeof(char));
  CPPTowerRuntimeContext trctx(stack, stackSize, /*limit*/ 256);

  // Start uses pointers to make the ABI simpler, the function is
  // implemented in assembly so such things matter.
  start(&trctx.ctx, add_tick_shim, &input, &output);

  ASSERT_TRUE(swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread));

  ASSERT_EQ(output, 69);

  free(stack);
}

extern "C" void simple_cond_tick_shim(TowerRuntimeContext *, void *, void *);
extern "C" int a_func() {
  return 69;
}
extern "C" int b_func() {
  return 420;
}

TEST(IntegrationTests, SimpleCond ) {
  // arguments to player code
  int input = 9;
  int output = -1;

  size_t stackSize = 8 * 1024;
  auto stack = (uint8_t *)calloc(stackSize, sizeof(char));
  CPPTowerRuntimeContext trctx(stack, stackSize, /*limit*/ 256);

  // Start uses pointers to make the ABI simpler, the function is
  // implemented in assembly so such things matter.
  start(&trctx.ctx, simple_cond_tick_shim, &input, &output);

  ASSERT_TRUE(swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread));

  ASSERT_EQ(output, 69);

  free(stack);
}

extern "C" void fib_tick_shim(TowerRuntimeContext *, void *, void *);

TEST(IntegrationTests, Fib) {
  // arguments to player code
  int input = 16;
  int output = -1;

  size_t stackSize = 8 * 1024;
  auto stack = (uint8_t *)calloc(stackSize, sizeof(char));
  CPPTowerRuntimeContext trctx(stack, stackSize, /*limit*/ 256);

  // Start uses pointers to make the ABI simpler, the function is
  // implemented in assembly so such things matter.
  start(&trctx.ctx, fib_tick_shim, &input, &output);

  // fib(16) should take more than 256 instructions to complete
  ASSERT_FALSE(swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread));
  ASSERT_EQ(output, -1);

  // fib(16) should complete on second run
  ASSERT_TRUE(swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread));
  ASSERT_EQ(output, 987);

  free(stack);
}
} // namespace
