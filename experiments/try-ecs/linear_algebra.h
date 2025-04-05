#ifndef LINEAR_ALGEBRA_H
#define LINEAR_ALGEBRA_H

#include <ostream>

namespace linear_algebra {
struct Vector2 {
  float x;
  float y;

  float dot(const Vector2 &other) const;
  float magnitude() const;
  bool close(const Vector2 &other, float diff = 1e-3) const;
  Vector2 normalized() const;
  Vector2 perpendicular() const;
  Vector2 operator-(const Vector2 &other) const;
  Vector2 operator+(const Vector2 &other) const;
  Vector2 operator*(float scalar) const;
};

Vector2 operator*(float scalar, const Vector2 &vec);

std::ostream &operator<<(std::ostream &os, const Vector2 &vec);

} // namespace linear_algebra

#endif // LINEAR_ALGEBRA_H
