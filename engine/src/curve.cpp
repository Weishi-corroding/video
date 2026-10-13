#include "eng/curve.h"

namespace eng {

const std::vector<float>& Curve::lengths() const {
  if ((int)lengths_.size() == arcLengthDivisions + 1) return lengths_;
  lengths_.assign(1, 0.f);
  vec3 last = point(0);
  float sum = 0;
  for (int i = 1; i <= arcLengthDivisions; i++) {
    vec3 p = point((float)i / arcLengthDivisions);
    sum += distance(p, last);
    lengths_.push_back(sum);
    last = p;
  }
  return lengths_;
}

float Curve::length() const { return lengths().back(); }

float Curve::uToT(float u) const {
  const auto& L = lengths();
  int n = (int)L.size();
  float target = clamp(u) * L[n - 1];
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    float cmp = L[mid] - target;
    if (cmp < 0) lo = mid + 1;
    else if (cmp > 0) hi = mid - 1;
    else { hi = mid; break; }
  }
  int i = std::max(0, hi);
  if (L[i] == target) return (float)i / (n - 1);
  if (i >= n - 1) return 1.f;
  float seg = L[i + 1] - L[i];
  float f = seg > 0 ? (target - L[i]) / seg : 0;
  return (i + f) / (n - 1);
}

vec3 Curve::tangent(float t) const {
  const float d = 0.0001f;
  float t1 = std::max(0.f, t - d), t2 = std::min(1.f, t + d);
  return (point(t2) - point(t1)).normalized();
}

void Curve::frames(int segments, bool closed, std::vector<vec3>& T, std::vector<vec3>& N, std::vector<vec3>& B) const {
  T.resize(segments + 1);
  N.resize(segments + 1);
  B.resize(segments + 1);
  for (int i = 0; i <= segments; i++) T[i] = tangentAt((float)i / segments);
  // First normal: perpendicular to the tangent, built from the axis the tangent leans on least.
  vec3 axis{1, 0, 0};
  float ax = std::fabs(T[0].x), ay = std::fabs(T[0].y), az = std::fabs(T[0].z);
  float mn = ax;
  if (ay <= mn) { mn = ay; axis = {0, 1, 0}; }
  if (az <= mn) axis = {0, 0, 1};
  vec3 v = cross(T[0], axis).normalized();
  N[0] = cross(T[0], v);
  B[0] = cross(T[0], N[0]);
  for (int i = 1; i <= segments; i++) {
    N[i] = N[i - 1];
    vec3 c = cross(T[i - 1], T[i]);
    if (c.length() > 1e-6f) {
      float th = std::acos(clamp(dot(T[i - 1], T[i]), -1.f, 1.f));
      N[i] = quat::axisAngle(c.normalized(), th).rotate(N[i]);
    }
    B[i] = cross(T[i], N[i]);
  }
  if (closed) {
    float th = std::acos(clamp(dot(N[0], N[segments]), -1.f, 1.f)) / segments;
    if (dot(T[0], cross(N[0], N[segments])) > 0) th = -th;
    for (int i = 1; i <= segments; i++) {
      N[i] = quat::axisAngle(T[i], th * i).rotate(N[i]);
      B[i] = cross(T[i], N[i]);
    }
  }
}

vec3 CatmullRom::point(float t) const {
  int l = (int)pts.size();
  float p = (l - (closed ? 0 : 1)) * t;
  int ip = (int)std::floor(p);
  float w = p - ip;
  if (closed) {
    ip += ip > 0 ? 0 : (int)(std::floor(std::fabs(ip) / l) + 1) * l;
  } else if (w == 0 && ip == l - 1) {
    ip = l - 2;
    w = 1;
  }
  vec3 p0, p1, p2, p3;
  if (closed || ip > 0) p0 = pts[(ip - 1 + l) % l];
  else p0 = pts[0] * 2.f - pts[1];
  p1 = pts[ip % l];
  p2 = pts[(ip + 1) % l];
  if (closed || ip + 2 < l) p3 = pts[(ip + 2) % l];
  else p3 = pts[l - 1] * 2.f - pts[l - 2];
  // Cubic Hermite between p1 and p2 with Catmull-Rom tangents scaled by the tension.
  vec3 t0 = (p2 - p0) * tension, t1 = (p3 - p1) * tension;
  vec3 c0 = p1, c1 = t0, c2 = p1 * -3.f + p2 * 3.f - t0 * 2.f - t1, c3 = p1 * 2.f - p2 * 2.f + t0 + t1;
  return c0 + c1 * w + c2 * (w * w) + c3 * (w * w * w);
}

}  // namespace eng
