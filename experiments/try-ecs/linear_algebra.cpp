#include "linear_algebra.h"

#include <cmath>

namespace linear_algebra {

float Vector2::dot(const Vector2 &other) const {
  return x * other.x + y * other.y;
}

float Vector2::magnitude() const { return std::sqrt(x * x + y * y); }

bool Vector2::close(const Vector2 &other, float diff) const {
  return (*this - other).magnitude() <= diff;
}

Vector2 Vector2::normalized() const {
  float m = magnitude();
  return (m != 0.f) ? Vector2{.x = x / m, .y = y / m} : Vector2{.x = x, .y = y};
}

Vector2 Vector2::perpendicular() const { return Vector2{.x = -y, .y = x}; }

Vector2 Vector2::operator-(const Vector2 &other) const {
  return {x - other.x, y - other.y};
}

Vector2 Vector2::operator+(const Vector2 &other) const {
  return {x + other.x, y + other.y};
}

Vector2 Vector2::operator*(float scalar) const {
  return {x * scalar, y * scalar};
}

Vector2 operator*(float scalar, const Vector2 &vec) { return vec * scalar; }

std::ostream &operator<<(std::ostream &os, const Vector2 &vec) {
  os << "(" << vec.x << ", " << vec.y << ")";
  return os;
}

} // namespace linear_algebra