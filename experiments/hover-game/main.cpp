#import "TowerRuntime/TowerRuntime.h"

#include "raylib.h"

#include <cassert>

extern "C" void tick_shim(TowerRuntimeContext *, void *, void *);

static const int screenWidth = 800;
static const int screenHeight = 450;

// map from player code colors to raylib colors
const Color colors[3] = {
    MAGENTA,
    RED,
    GREEN,
};

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

int main(int argc, char **argv) {
  // contexts for game code and player code
  struct MachineState gameCtx;
  struct MachineState playerCodeCtx;

  // arguments to player code
  int input = -1;
  int completed = -1;
  int computing = -1;

  // Does player code need setup?
  bool needsSetup = true;

  // setup stack for player code
  size_t stackSize = 8 * 1024;
  char *stack = (char *)calloc(stackSize, sizeof(char));

  TowerRuntimeContext ctx{
      .main = &gameCtx,
      .thread = &playerCodeCtx,
      .count = 0,
      .limit = 256,
  };

  InitWindow(screenWidth, screenHeight, "classic game: asteroids");
  SetTargetFPS(60);

  Vector2 circleCenter = {.x = (float)screenWidth / 2,
                          .y = (float)screenHeight / 2};
  float circleRadius = 100.0;

  while (!WindowShouldClose()) { // Detect window close button or ESC key
    // update ==================================================================
    // update game state from player interation ============
    Vector2 mousePosition = GetMousePosition();
    int hover =
        CheckCollisionPointCircle(mousePosition, circleCenter, circleRadius);

    // update game state from player code interation =======
    if (needsSetup) {
      needsSetup = false;

      // set the link register to our setup/launch function. ARM's RET
      // instruction will return (aka jump) to whatever function we set here.
      // You can't set the link register like this from C, but the co-routine
      // start function is written in assembly.
      playerCodeCtx.lr = (uint64_t)launchpad;

      // set stack for thread context
      playerCodeCtx.sp = (uint64_t)&stack[stackSize];
      playerCodeCtx.fp = (uint64_t)&stack[stackSize];

      input = hover;

      // Start uses pointers to make the ABI simpler, the function is
      // implemented in assembly so such things matter.
      start(&ctx, tick_shim, &input, &computing);
    }

    if (swap(/*from*/ ctx.main, /*to*/ ctx.thread)) {
      completed = computing;
      needsSetup = true;
    }

    Color playerCodeColorChoice = colors[completed];

    // draw ====================================================================
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawCircleV(circleCenter, circleRadius, Fade(playerCodeColorChoice, 0.3f));
    EndDrawing();
  }
  free(stack);
  CloseWindow();
  return 0;
}
