// Vectors, quaternions, matrices and colours. Matrices are column-major (m[col*4 + row]),
// the world is right-handed with +y up, and cameras look down their local -z axis.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace eng {

constexpr float PI = 3.14159265358979323846f;

inline float clamp(float x, float a = 0.f, float b = 1.f) { return std::min(b, std::max(a, x)); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float radians(float deg) { return deg * PI / 180.f; }
inline float degrees(float rad) { return rad * 180.f / PI; }

struct vec2 {
  float x = 0, y = 0;
  vec2() = default;
  constexpr vec2(float x_, float y_) : x(x_), y(y_) {}
  vec2 operator+(vec2 o) const { return {x + o.x, y + o.y}; }
  vec2 operator-(vec2 o) const { return {x - o.x, y - o.y}; }
  vec2 operator*(float s) const { return {x * s, y * s}; }
};

struct vec3 {
  float x = 0, y = 0, z = 0;
  vec3() = default;
  constexpr vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
  vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
  vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
  vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
  vec3 operator*(const vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
  vec3 operator-() const { return {-x, -y, -z}; }
  vec3& operator+=(const vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
  vec3& operator-=(const vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
  vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
  bool operator==(const vec3& o) const { return x == o.x && y == o.y && z == o.z; }
  bool operator!=(const vec3& o) const { return !(*this == o); }
  vec3& set(float x_, float y_, float z_) { x = x_; y = y_; z = z_; return *this; }
  vec3& setScalar(float s) { x = y = z = s; return *this; }
  float length() const { return std::sqrt(x * x + y * y + z * z); }
  float lengthSq() const { return x * x + y * y + z * z; }
  vec3 normalized() const { float l = length(); return l > 0 ? *this / l : vec3{}; }
  vec3 withY(float y_) const { return {x, y_, z}; }
};

inline vec3 operator*(float s, const vec3& v) { return v * s; }
inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(const vec3& a, const vec3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline vec3 normalize(const vec3& v) { return v.normalized(); }
inline float distance(const vec3& a, const vec3& b) { return (a - b).length(); }
inline vec3 lerp(const vec3& a, const vec3& b, float t) { return a + (b - a) * t; }

struct vec4 {
  float x = 0, y = 0, z = 0, w = 0;
  vec4() = default;
  constexpr vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
  vec4(const vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
  vec3 xyz() const { return {x, y, z}; }
};

struct mat4;

struct quat {
  float x = 0, y = 0, z = 0, w = 1;
  quat() = default;
  constexpr quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

  static quat axisAngle(const vec3& axis, float a) {
    float s = std::sin(a / 2);
    vec3 n = axis.normalized();
    return {n.x * s, n.y * s, n.z * s, std::cos(a / 2)};
  }
  // Rotation for Euler angles applied in XYZ order: the matrix is Rx * Ry * Rz.
  static quat euler(const vec3& e) {
    float c1 = std::cos(e.x / 2), c2 = std::cos(e.y / 2), c3 = std::cos(e.z / 2);
    float s1 = std::sin(e.x / 2), s2 = std::sin(e.y / 2), s3 = std::sin(e.z / 2);
    return {s1 * c2 * c3 + c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3, c1 * c2 * s3 + s1 * s2 * c3, c1 * c2 * c3 - s1 * s2 * s3};
  }
  // Shortest rotation taking unit vector a onto unit vector b.
  static quat fromTo(const vec3& a, const vec3& b) {
    float r = dot(a, b) + 1;
    quat q;
    if (r < 1e-6f) {
      r = 0;
      if (std::fabs(a.x) > std::fabs(a.z)) q = {-a.y, a.x, 0, r};
      else q = {0, -a.z, a.y, r};
    } else {
      vec3 c = cross(a, b);
      q = {c.x, c.y, c.z, r};
    }
    return q.normalized();
  }
  quat normalized() const {
    float l = std::sqrt(x * x + y * y + z * z + w * w);
    return l > 0 ? quat{x / l, y / l, z / l, w / l} : quat{};
  }
  quat operator*(const quat& b) const {
    return {x * b.w + w * b.x + y * b.z - z * b.y, y * b.w + w * b.y + z * b.x - x * b.z,
            z * b.w + w * b.z + x * b.y - y * b.x, w * b.w - x * b.x - y * b.y - z * b.z};
  }
  quat conjugate() const { return {-x, -y, -z, w}; }
  vec3 rotate(const vec3& v) const {
    vec3 u{x, y, z};
    vec3 t = cross(u, v) * 2.f;
    return v + t * w + cross(u, t);
  }
  // Euler angles (XYZ order) that produce this rotation.
  vec3 toEuler() const;
  // The rotation part of a matrix (its columns are normalised first).
  static quat fromMatrix(const mat4& m);
};

struct mat3 {
  float m[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  vec3 operator*(const vec3& v) const {
    return {m[0] * v.x + m[3] * v.y + m[6] * v.z, m[1] * v.x + m[4] * v.y + m[7] * v.z, m[2] * v.x + m[5] * v.y + m[8] * v.z};
  }
};

struct mat4 {
  float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  static mat4 identity() { return {}; }
  static mat4 compose(const vec3& p, const quat& q, const vec3& s) {
    mat4 r;
    float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2, yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
    float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    r.m[0] = (1 - (yy + zz)) * s.x; r.m[1] = (xy + wz) * s.x; r.m[2] = (xz - wy) * s.x; r.m[3] = 0;
    r.m[4] = (xy - wz) * s.y; r.m[5] = (1 - (xx + zz)) * s.y; r.m[6] = (yz + wx) * s.y; r.m[7] = 0;
    r.m[8] = (xz + wy) * s.z; r.m[9] = (yz - wx) * s.z; r.m[10] = (1 - (xx + yy)) * s.z; r.m[11] = 0;
    r.m[12] = p.x; r.m[13] = p.y; r.m[14] = p.z; r.m[15] = 1;
    return r;
  }
  static mat4 translation(const vec3& p) { return compose(p, {}, {1, 1, 1}); }
  // Perspective projection for Vulkan depth (0..1). y is up in clip space; the renderer flips the viewport.
  static mat4 perspective(float fovyRad, float aspect, float n, float f) {
    mat4 r;
    float t = 1.f / std::tan(fovyRad / 2);
    r.m[0] = t / aspect; r.m[5] = t; r.m[10] = f / (n - f); r.m[11] = -1; r.m[14] = n * f / (n - f); r.m[15] = 0;
    return r;
  }
  static mat4 ortho(float l, float rgt, float b, float t, float n, float f) {
    mat4 r;
    r.m[0] = 2 / (rgt - l); r.m[5] = 2 / (t - b); r.m[10] = -1 / (f - n);
    r.m[12] = -(rgt + l) / (rgt - l); r.m[13] = -(t + b) / (t - b); r.m[14] = -n / (f - n);
    return r;
  }
  // View matrix of an eye at `eye` looking at `target`.
  static mat4 lookAtView(const vec3& eye, const vec3& target, const vec3& up) {
    vec3 z = (eye - target).normalized();
    if (z.lengthSq() == 0) z = {0, 0, 1};
    vec3 x = cross(up, z);
    if (x.lengthSq() < 1e-12f) x = cross(std::fabs(up.z) == 1 ? vec3{1, 0, 0} : vec3{0, 0, 1}, z);
    x = x.normalized();
    vec3 y = cross(z, x);
    mat4 r;
    r.m[0] = x.x; r.m[4] = x.y; r.m[8] = x.z;
    r.m[1] = y.x; r.m[5] = y.y; r.m[9] = y.z;
    r.m[2] = z.x; r.m[6] = z.y; r.m[10] = z.z;
    r.m[12] = -dot(x, eye); r.m[13] = -dot(y, eye); r.m[14] = -dot(z, eye);
    return r;
  }

  mat4 operator*(const mat4& b) const {
    mat4 r;
    for (int c = 0; c < 4; c++)
      for (int rr = 0; rr < 4; rr++)
        r.m[c * 4 + rr] = m[rr] * b.m[c * 4] + m[4 + rr] * b.m[c * 4 + 1] + m[8 + rr] * b.m[c * 4 + 2] + m[12 + rr] * b.m[c * 4 + 3];
    return r;
  }
  vec4 operator*(const vec4& v) const {
    return {m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w, m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w, m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w};
  }
  vec3 transformPoint(const vec3& v) const {
    return {m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12], m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13], m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]};
  }
  vec3 transformDir(const vec3& v) const {
    return {m[0] * v.x + m[4] * v.y + m[8] * v.z, m[1] * v.x + m[5] * v.y + m[9] * v.z, m[2] * v.x + m[6] * v.y + m[10] * v.z};
  }
  vec3 position() const { return {m[12], m[13], m[14]}; }
  vec3 column(int c) const { return {m[c * 4], m[c * 4 + 1], m[c * 4 + 2]}; }
  float maxScale() const {
    return std::sqrt(std::max({column(0).lengthSq(), column(1).lengthSq(), column(2).lengthSq()}));
  }
  mat4 inverse() const;
  // Inverse-transpose of the upper 3x3, for transforming normals.
  mat3 normalMatrix() const;
};

inline quat quat::fromMatrix(const mat4& mm) {
  vec3 x = mm.column(0).normalized(), y = mm.column(1).normalized(), z = mm.column(2).normalized();
  float tr = x.x + y.y + z.z;
  quat q;
  if (tr > 0) {
    float s = 0.5f / std::sqrt(tr + 1);
    q = {(y.z - z.y) * s, (z.x - x.z) * s, (x.y - y.x) * s, 0.25f / s};
  } else if (x.x > y.y && x.x > z.z) {
    float s = 2 * std::sqrt(1 + x.x - y.y - z.z);
    q = {0.25f * s, (y.x + x.y) / s, (z.x + x.z) / s, (y.z - z.y) / s};
  } else if (y.y > z.z) {
    float s = 2 * std::sqrt(1 + y.y - x.x - z.z);
    q = {(y.x + x.y) / s, 0.25f * s, (z.y + y.z) / s, (z.x - x.z) / s};
  } else {
    float s = 2 * std::sqrt(1 + z.z - x.x - y.y);
    q = {(z.x + x.z) / s, (z.y + y.z) / s, 0.25f * s, (x.y - y.x) / s};
  }
  return q.normalized();
}

// Colours are stored linear. hex() reads an sRGB colour such as 0xffcf5a.
struct Color {
  float r = 1, g = 1, b = 1;
  Color() = default;
  constexpr Color(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
  static float toLinear(float c) { return c < 0.04045f ? c * 0.0773993808f : std::pow(c * 0.9478672986f + 0.0521327014f, 2.4f); }
  static float toSRGB(float c) { return c < 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 0.41666f) - 0.055f; }
  static Color hex(uint32_t h) {
    return {toLinear(((h >> 16) & 255) / 255.f), toLinear(((h >> 8) & 255) / 255.f), toLinear((h & 255) / 255.f)};
  }
  uint32_t toHex() const {
    auto q = [](float c) { return (uint32_t)std::lround(clamp(toSRGB(clamp(c)), 0.f, 1.f) * 255.f); };
    return (q(r) << 16) | (q(g) << 8) | q(b);
  }
  Color operator*(float s) const { return {r * s, g * s, b * s}; }
  Color operator*(const Color& o) const { return {r * o.r, g * o.g, b * o.b}; }
  Color operator+(const Color& o) const { return {r + o.r, g + o.g, b + o.b}; }
  Color lerp(const Color& o, float t) const { return {r + (o.r - r) * t, g + (o.g - g) * t, b + (o.b - b) * t}; }
};

}  // namespace eng
