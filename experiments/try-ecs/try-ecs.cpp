#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "ecs.h"
#include "linear_algebra.h"

namespace la = linear_algebra;

// ----------------------------------------------------------------------
//  Components
// ----------------------------------------------------------------------

// Component for position in 2D space
struct Position {
  float x, y;
};
STATIC_ASSERT_POD(Position)

la::Vector2 toVector2(const Position &pos) { return la::Vector2{pos.x, pos.y}; }
la::Vector2 toVector2(Position *pos) { return la::Vector2{pos->x, pos->y}; }
Position toPosition(const la::Vector2 &vec) { return Position{vec.x, vec.y}; }

struct Velocity {
  float vx, vy;
};
STATIC_ASSERT_POD(Velocity)

la::Vector2 toVector2(const Velocity &v) { return la::Vector2{v.vx, v.vy}; }
la::Vector2 toVector2(Velocity *v) { return la::Vector2{v->vx, v->vy}; }
Velocity toVelocity(const la::Vector2 &vec) { return Velocity{vec.x, vec.y}; }

// ----------------------------------------------------------------------
//  Utils
// ----------------------------------------------------------------------

#define PRINT_FP(x)                                                            \
  std::fixed << std::setprecision(2) << std::left << std::setw(6) << x
#define PRINT_POS(pos)                                                         \
  "Position* {.x=" << PRINT_FP(pos->x) << ", .y=" << PRINT_FP(pos->y) << "}"
#define PRINT_ENTITY(e) "Entity=" << std::left << std::setw(2) << e

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

// ----------------------------------------------------------------------
//  The business
// ----------------------------------------------------------------------

int main() {
  // Initialize raylib
  constexpr int screenWidth = 400;
  constexpr int screenHeight = 400;
  InitWindow(screenWidth, screenHeight, "Simple ECS + Raylib Example");
  SetTargetFPS(60);

  ECS ecs;

  // Create 20 random entities
  size_t n = 20;
  createRandomEntities(ecs, n, screenWidth, screenHeight);

  // Game loop
  while (!WindowShouldClose()) {
    static constexpr float radius = 10.0f;
    // ----------------------------------------------------------------------
    //  Movement
    // ----------------------------------------------------------------------
    for (auto &[ent, pos, vel] : ecs.query<Position, Velocity>()) {
      pos->x += vel->vx;
      pos->y += vel->vy;

      // Bounce off screen edges and correct position
      if (pos->x - radius < 0) {
        vel->vx *= -1.0f;
        pos->x = radius + 0.1f; // Move slightly inside boundary
      } else if (pos->x + radius > screenWidth) {
        vel->vx *= -1.0f;
        pos->x = screenWidth - radius - 0.1f; // Move slightly inside boundary
      }

      if (pos->y - radius < 0) {
        vel->vy *= -1.0f;
        pos->y = radius + 0.1f; // Move slightly inside boundary
      } else if (pos->y + radius > screenHeight) {
        vel->vy *= -1.0f;
        pos->y = screenHeight - radius - 0.1f; // Move slightly inside boundary
      }
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
      float x() const { return isStart ? pos->x - radius : pos->x + radius; }
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
      // Sort edges by x coordinate; if tie, start edges before end edges
      if (a.x() == b.x()) {
        // Put start edges before end edges if x is the same
        return (a.isStart && !b.isStart);
      }
      return a.x() < b.x();
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
              4.0 * radius * radius;
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
        assert(it != active.end() && "start edge not found");
        active.erase(it);
      }
    }

    // ----------------------------------------------------------------------
    // Collisions
    // ----------------------------------------------------------------------

    for (auto [a, b] : ecs.query<Position, Velocity>(overlaps)) {
      auto &[entity_a, pos_a, vel_a] = a;
      auto &[entity_b, pos_b, vel_b] = b;

      la::Vector2 v_a = toVector2(vel_a);
      la::Vector2 a_normal = (toVector2(pos_a) - toVector2(pos_b)).normalized();

      la::Vector2 v_a_normal = v_a.dot(a_normal) * a_normal;
      la::Vector2 v_a_tangential = v_a - v_a_normal;

      la::Vector2 v_b = toVector2(vel_b);
      la::Vector2 b_normal = (toVector2(pos_b) - toVector2(pos_a)).normalized();

      la::Vector2 v_b_normal = v_b.dot(b_normal) * b_normal;
      la::Vector2 v_b_tangential = v_b - v_b_normal;

      *vel_a = toVelocity(v_a_tangential + v_b_normal);
      *vel_b = toVelocity(v_b_tangential + v_a_normal);

      // Separate the balls to prevent sticking
      float distance = (toVector2(pos_a) - toVector2(pos_b)).magnitude();
      float overlap = (2 * radius) - distance;

      if (overlap > 0) {
        // Move balls apart along the collision normal
        float separation = overlap * 0.5f + 0.1f; // Add a small extra gap
        pos_a->x += a_normal.x * separation;
        pos_a->y += a_normal.y * separation;
        pos_b->x += b_normal.x * separation;
        pos_b->y += b_normal.y * separation;
      }
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
      DrawText(std::to_string(ent).c_str(), static_cast<int>(pos->x) - 2,
               static_cast<int>(pos->y) - 4, 6, BLACK);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
