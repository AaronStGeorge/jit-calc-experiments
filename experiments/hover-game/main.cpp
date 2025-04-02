#import "TowerRuntime/TowerRuntime.h"

#include "raylib.h"

#include <cassert>
#include <cstdint>

extern "C" void tick_shim(TowerRuntimeContext *, void *, void *);

static const int screenWidth = 800;
static const int screenHeight = 450;

// map from player code colors to raylib colors
const Color colors[3] = {
    MAGENTA,
    RED,
    GREEN,
};

int main(int argc, char **argv) {
  // arguments to player code
  int input = -1;
  int completed = -1;
  int computing = -1;

  // Does player code need setup?
  bool needsSetup = true;

  // setup stack for player code
  size_t stackSize = 8 * 1024;
  auto stack = (uint8_t *)calloc(stackSize, sizeof(char));

  auto trctx = CPPTowerRuntimeContext(stack, stackSize, 256);

  InitWindow(screenWidth, screenHeight, "hover game");
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

      trctx = CPPTowerRuntimeContext(stack, stackSize, 256);

      input = hover;

      // Start uses pointers to make the ABI simpler, the function is
      // implemented in assembly so such things matter.
      start(&trctx.ctx, tick_shim, &input, &computing);
    }

    if (swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread)) {
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
