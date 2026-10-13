// Parametric 3D curves with arc-length sampling.
#pragma once
#include <functional>
#include <vector>
#include "math.h"

namespace eng {

struct Curve {
  int arcLengthDivisions = 200;
  virtual ~Curve() = default;

  // Point at parameter t in [0, 1].
  virtual vec3 point(float t) const = 0;
  // Point at fraction u of the length.
  vec3 pointAt(float u) const { return point(uToT(u)); }
  // Unit tangent at parameter t, and at fraction u of the length.
  vec3 tangent(float t) const;
  vec3 tangentAt(float u) const { return tangent(uToT(u)); }
  float length() const;
  float uToT(float u) const;
  // Tangents, normals and binormals by parallel transport, for segments + 1 samples.
  void frames(int segments, bool closed, std::vector<vec3>& T, std::vector<vec3>& N, std::vector<vec3>& B) const;

 private:
  const std::vector<float>& lengths() const;
  mutable std::vector<float> lengths_;
};

struct QuadraticBezier : Curve {
  vec3 a, b, c;
  QuadraticBezier(vec3 a_, vec3 b_, vec3 c_) : a(a_), b(b_), c(c_) {}
  vec3 point(float t) const override {
    float k = 1 - t;
    return a * (k * k) + b * (2 * k * t) + c * (t * t);
  }
};

// Catmull-Rom spline through the points. With a small tension the curve stays close to the
// straight segments and only rounds the corners slightly.
struct CatmullRom : Curve {
  std::vector<vec3> pts;
  bool closed = false;
  float tension = 0.5f;
  CatmullRom(std::vector<vec3> p, bool closed_ = false, float tension_ = 0.5f) : pts(std::move(p)), closed(closed_), tension(tension_) {}
  vec3 point(float t) const override;
};

// A curve from a function, for one-off shapes.
struct FnCurve : Curve {
  std::function<vec3(float)> fn;
  explicit FnCurve(std::function<vec3(float)> f) : fn(std::move(f)) {}
  vec3 point(float t) const override { return fn(t); }
};

}  // namespace eng
