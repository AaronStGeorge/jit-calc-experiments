#include "TowerRuntime/TowerRuntime.h"

#include "gtest/gtest.h"

extern "C" void add_tick_shim(TowerRuntimeContext *, void *, void *);
extern "C" void simple_tick_shim(TowerRuntimeContext *, void *, void *);

namespace {
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

struct Thing {
  int a;
  int b;
};

TEST(IntegrationTests, ADD) {
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
} // namespace
