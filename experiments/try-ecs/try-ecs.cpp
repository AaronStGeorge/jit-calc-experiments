#include <cstddef>
#include <random>

#include <raylib.h>
#include <vector>

#include "ecs.h"

// Component for position in 2D space
struct Position {
  float x, y;
};

// (Optional) A velocity component if you want to see motion
struct Velocity {
  float vx, vy;
};

// Create N random entities.
void createRandomEntities(ECS &ecs, size_t n, int screenWidth,
                          int screenHeight) {
  static std::random_device rd; // Obtain a random seed
  static std::mt19937 generator(
      rd()); // Mersenne Twister engine seeded with rd()
  std::uniform_real_distribution<float> x(20, screenWidth - 20);
  std::uniform_real_distribution<float> y(20, screenHeight - 20);
  std::uniform_real_distribution<float> v(-5.0, 5.0);

  for (int i = 0; i < n; i++) {
    ecs.createEntity(Position{.x = x(generator), .y = y(generator)},
                     Velocity{.vx = v(generator), .vy = v(generator)});
  }
}

int main() {
  // Initialize raylib
  constexpr int screenWidth = 800;
  constexpr int screenHeight = 450;
  InitWindow(screenWidth, screenHeight, "Simple ECS + Raylib Example");
  SetTargetFPS(60);

  ECS ecs;

  // Create 10 random entities
  size_t n = 10;
  createRandomEntities(ecs, n, screenWidth, screenHeight);

  // Main loop
  while (!WindowShouldClose()) {
    // ----------------------------------------------------------------------
    //  Movement
    // ----------------------------------------------------------------------
    for (auto &[ent, pos, vel] : ecs.query<Position, Velocity>()) {
      pos->x += vel->vx;
      pos->y += vel->vy;

      // Bounce off screen edges
      if (pos->x - 10 < 0 || pos->x + 10 > screenWidth)
        vel->vx *= -1.0f;
      if (pos->y - 10 < 0 || pos->y + 10 > screenHeight)
        vel->vy *= -1.0f;
    }

    // ----------------------------------------------------------------------
    //  Rendering
    // ----------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(BLACK);

    // Draw all entities with a Position
    for (auto &[ent, pos] : ecs.query<Position>()) {
      // A simple white circle at (pos->x, pos->y)
      DrawCircle(static_cast<int>(pos->x), static_cast<int>(pos->y), 10, WHITE);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
