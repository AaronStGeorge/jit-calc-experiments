#include <cstddef>
#include <iomanip>
#include <ios>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include <raylib.h>

#include "ecs.h"

// Component for position in 2D space
struct Position {
  float x, y;
};
STATIC_ASSERT_POD(Position)

struct Velocity {
  float vx, vy;
};
STATIC_ASSERT_POD(Velocity)

// Create N random entities.
void createRandomEntities(ECS &ecs, size_t n, int screenWidth,
                          int screenHeight) {
  static std::random_device rd; // Obtain a random seed
  static std::mt19937 generator(
      rd()); // Mersenne Twister engine seeded with rd()
  std::uniform_real_distribution<float> x(20, screenWidth - 20);
  std::uniform_real_distribution<float> y(20, screenHeight - 20);
  std::uniform_real_distribution<float> v(-5.0, 5.0);

  for (size_t i = 0; i < n; i++) {
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

    static constexpr float radius = 10.0f;
    // ----------------------------------------------------------------------
    //  Movement
    // ----------------------------------------------------------------------
    for (auto &[ent, pos, vel] : ecs.query<Position, Velocity>()) {
      pos->x += vel->vx;
      pos->y += vel->vy;

      // Bounce off screen edges
      if (pos->x - radius < 0 || pos->x + radius > screenWidth)
        vel->vx *= -1.0f;
      if (pos->y - radius < 0 || pos->y + radius > screenHeight)
        vel->vy *= -1.0f;
    }

    // ----------------------------------------------------------------------
    // Sweep and Prune on the x-axis to detect overlapping entities
    // ----------------------------------------------------------------------

    // Edge structure for the x-axis
    struct Edge {
      Position *pos;
      Entity entity;
      bool isStart; // true if left edge, false if right edge

      // Get the x-coordinate of the edge
      float value() const {
        return isStart ? pos->x - radius : pos->x + radius;
      }
    };

    // Build a list of Edge structures for x-axis bounding of each circle.
    std::vector<Edge> edges;
    // To handle all positions with a query
    {
      auto posQuery = ecs.query<Position>();
      edges.reserve(posQuery.begin() != posQuery.end()
                        ? (posQuery.end() - posQuery.begin()) * 2
                        : 0);

      for (auto &[ent, pos] : posQuery) {
        edges.push_back({pos, ent, true});
        edges.push_back({pos, ent, false});
      }
    }

    // Sort edges and sweep to find x-overlapping entity pairs.
    std::sort(edges.begin(), edges.end(), [](auto &a, auto &b) {
      // Sort edges by coordinate; if tie, start edges before end edges
      if (a.value() == b.value()) {
        // Put start edges before end edges if x is the same
        return (a.isStart && !b.isStart);
      }
      return a.value() < b.value();
    });

    // Sweep to find all x-overlapping pairs
    std::vector<std::pair<Entity, Entity>> overlaps;
    overlaps.reserve(edges.size()); // Overallocate in case

    // The set of intervals that have started but not ended
    std::vector<Edge> active;

    for (auto &edge : edges) {
      if (edge.isStart) {
        // Check for overlaps with all active intervals
        for (auto &other : active) {
          // standard pythagorean theorem to check if two circles overlap
          bool overlap =
              (other.pos->x - edge.pos->x) * (other.pos->x - edge.pos->x) +
                  (other.pos->y - edge.pos->y) * (other.pos->y - edge.pos->y) <=
              radius * radius;
          if (overlap) {
            overlaps.push_back({edge.entity, other.entity});
          }
        }
        active.push_back(edge);
      } else {
        // interval ends -> remove from active
        auto it =
            std::find_if(active.begin(), active.end(), [&edge](const Edge &e) {
              return e.entity == edge.entity;
            });
        if (it != active.end()) {
          active.erase(it);
        }
      }
    }

    // ----------------------------------------------------------------------
    // Collisions
    // ----------------------------------------------------------------------

    for (auto [a, b] : ecs.query<Position, Velocity>(overlaps)) {
      auto &[e1, pos1, vel1] = a;
      auto &[e2, pos2, vel2] = b;

#define PRINT_FP(x)                                                            \
  std::fixed << std::setprecision(2) << std::left << std::setw(6) << x
#define PRINT_POS(pos)                                                         \
  "Position* {.x=" << PRINT_FP(pos->x) << ", .y=" << PRINT_FP(pos->y) << ")"
#define PRINT_ENTITY(e) "Entity=" << std::left << std::setw(2) << e

      std::cout << PRINT_ENTITY(e1) << " " << PRINT_POS(pos1);
      std::cout << " <OVERLAP> ";
      std::cout << PRINT_ENTITY(e2) << " " << PRINT_POS(pos2);
      std::cout << std::endl;

#undef PRINT_ENTITY
#undef PRINT_POS
#undef PRINT_FP
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
