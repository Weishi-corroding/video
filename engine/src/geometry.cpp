#include "eng/geometry.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>

#include "eng/curve.h"

namespace eng {

namespace {
std::vector<std::unique_ptr<Geometry>>& store() {
  static std::vector<std::unique_ptr<Geometry>> s;
  return s;
}

struct Builder {
  Geometry* g;
  explicit Builder(Topology t = Topology::Triangles) : g(newGeometry()) { g->topology = t; }
  uint32_t vertex(const vec3& p, const vec3& n, float u, float v) {
    g->positions.insert(g->positions.end(), {p.x, p.y, p.z});
    g->normals.insert(g->normals.end(), {n.x, n.y, n.z});
    g->uvs.insert(g->uvs.end(), {u, v});
    return (uint32_t)(g->positions.size() / 3 - 1);
  }
  void tri(uint32_t a, uint32_t b, uint32_t c) { g->indices.insert(g->indices.end(), {a, b, c}); }
  Geometry* done() {
    g->computeBounds();
    return g;
  }
};

// Grid coordinates across one face of a rounded box, from -half to +half. The outer `segments` cells
// on each side lie in the rounded band; their spacing gives equal angles once pushed onto the arc.
std::vector<float> roundedCoords(float half, float r, int segments) {
  std::vector<float> c;
  for (int k = segments; k >= 1; k--) c.push_back(-half + r - r * std::tan(PI / 4 * k / segments));
  c.push_back(-half + r);
  c.push_back(half - r);
  for (int k = 1; k <= segments; k++) c.push_back(half - r + r * std::tan(PI / 4 * k / segments));
  return c;
}

float extent(const vec3& a, float w, float h, float d) { return std::fabs(a.x) * w / 2 + std::fabs(a.y) * h / 2 + std::fabs(a.z) * d / 2; }

struct Face { vec3 n, u, v; };
const Face kFaces[6] = {
    {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}}, {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}}, {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
    {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}, {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},  {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
};
}  // namespace

Geometry* newGeometry() {
  store().push_back(std::make_unique<Geometry>());
  return store().back().get();
}

void Geometry::computeBounds() {
  size_t n = vertexCount();
  if (!n) { boundsCenter = {}; boundsRadius = 0; return; }
  vec3 mn{1e30f, 1e30f, 1e30f}, mx{-1e30f, -1e30f, -1e30f};
  for (size_t i = 0; i < n; i++) {
    const float* p = &positions[i * 3];
    mn = {std::min(mn.x, p[0]), std::min(mn.y, p[1]), std::min(mn.z, p[2])};
    mx = {std::max(mx.x, p[0]), std::max(mx.y, p[1]), std::max(mx.z, p[2])};
  }
  boundsCenter = (mn + mx) * 0.5f;
  float r2 = 0;
  for (size_t i = 0; i < n; i++) r2 = std::max(r2, (vec3{positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]} - boundsCenter).lengthSq());
  boundsRadius = std::sqrt(r2);
}

Geometry& Geometry::applyMatrix(const mat4& m) {
  mat3 nm = m.normalMatrix();
  for (size_t i = 0; i < vertexCount(); i++) {
    vec3 p = m.transformPoint({positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]});
    positions[i * 3] = p.x; positions[i * 3 + 1] = p.y; positions[i * 3 + 2] = p.z;
    if (normals.size() == positions.size()) {
      vec3 n = (nm * vec3{normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]}).normalized();
      normals[i * 3] = n.x; normals[i * 3 + 1] = n.y; normals[i * 3 + 2] = n.z;
    }
  }
  computeBounds();
  return *this;
}

namespace geo {

Geometry* box(float w, float h, float d) {
  Builder b;
  for (const Face& f : kFaces) {
    float hu = extent(f.u, w, h, d), hv = extent(f.v, w, h, d), hn = extent(f.n, w, h, d);
    uint32_t base = (uint32_t)b.g->vertexCount();
    for (int j = 0; j <= 1; j++)
      for (int i = 0; i <= 1; i++) {
        float x = i ? hu : -hu, y = j ? hv : -hv;
        b.vertex(f.n * hn + f.u * x + f.v * y, f.n, (float)i, (float)j);
      }
    b.tri(base, base + 1, base + 3);
    b.tri(base, base + 3, base + 2);
  }
  return b.done();
}

Geometry* roundedBox(float w, float h, float d, int segments, float radius) {
  Builder b;
  vec3 half{w / 2, h / 2, d / 2};
  vec3 innerHalf = half - vec3{radius, radius, radius};
  for (const Face& f : kFaces) {
    float hu = extent(f.u, w, h, d), hv = extent(f.v, w, h, d), hn = extent(f.n, w, h, d);
    std::vector<float> cu = roundedCoords(hu, radius, segments), cv = roundedCoords(hv, radius, segments);
    int nu = (int)cu.size(), nv = (int)cv.size();
    uint32_t base = (uint32_t)b.g->vertexCount();
    for (int j = 0; j < nv; j++)
      for (int i = 0; i < nu; i++) {
        vec3 p = f.n * hn + f.u * cu[i] + f.v * cv[j];
        vec3 inner{clamp(p.x, -innerHalf.x, innerHalf.x), clamp(p.y, -innerHalf.y, innerHalf.y), clamp(p.z, -innerHalf.z, innerHalf.z)};
        vec3 n = (p - inner).normalized();
        b.vertex(inner + n * radius, n, (cu[i] + hu) / (2 * hu), (cv[j] + hv) / (2 * hv));
      }
    for (int j = 0; j + 1 < nv; j++)
      for (int i = 0; i + 1 < nu; i++) {
        uint32_t a = base + j * nu + i, c = a + 1, e = a + nu, g2 = e + 1;
        b.tri(a, c, g2);
        b.tri(a, g2, e);
      }
  }
  return b.done();
}

Geometry* plane(float w, float h) {
  Builder b;
  b.vertex({-w / 2, -h / 2, 0}, {0, 0, 1}, 0, 0);
  b.vertex({w / 2, -h / 2, 0}, {0, 0, 1}, 1, 0);
  b.vertex({-w / 2, h / 2, 0}, {0, 0, 1}, 0, 1);
  b.vertex({w / 2, h / 2, 0}, {0, 0, 1}, 1, 1);
  b.tri(0, 1, 3);
  b.tri(0, 3, 2);
  return b.done();
}

Geometry* circle(float r, int segments) {
  Builder b;
  b.vertex({0, 0, 0}, {0, 0, 1}, 0.5f, 0.5f);
  for (int s = 0; s <= segments; s++) {
    float a = (float)s / segments * 2 * PI;
    float x = std::cos(a), y = std::sin(a);
    b.vertex({r * x, r * y, 0}, {0, 0, 1}, (x + 1) / 2, (y + 1) / 2);
  }
  for (int i = 1; i <= segments; i++) b.tri(i, i + 1, 0);
  return b.done();
}

Geometry* sphere(float r, int ws, int hs, float phiStart, float phiLength, float thetaStart, float thetaLength) {
  Builder b;
  float thetaEnd = std::min(thetaStart + thetaLength, PI);
  std::vector<std::vector<uint32_t>> grid(hs + 1, std::vector<uint32_t>(ws + 1));
  for (int iy = 0; iy <= hs; iy++) {
    float v = (float)iy / hs;
    float uOffset = 0;
    if (iy == 0 && thetaStart == 0) uOffset = 0.5f / ws;
    else if (iy == hs && thetaEnd == PI) uOffset = -0.5f / ws;
    for (int ix = 0; ix <= ws; ix++) {
      float u = (float)ix / ws;
      float ph = phiStart + u * phiLength, th = thetaStart + v * thetaLength;
      vec3 p{-r * std::cos(ph) * std::sin(th), r * std::cos(th), r * std::sin(ph) * std::sin(th)};
      grid[iy][ix] = b.vertex(p, p.normalized(), u + uOffset, 1 - v);
    }
  }
  for (int iy = 0; iy < hs; iy++)
    for (int ix = 0; ix < ws; ix++) {
      uint32_t a = grid[iy][ix + 1], bb = grid[iy][ix], c = grid[iy + 1][ix], d = grid[iy + 1][ix + 1];
      if (iy != 0 || thetaStart > 0) b.tri(a, bb, d);
      if (iy != hs - 1 || thetaEnd < PI) b.tri(bb, c, d);
    }
  return b.done();
}

Geometry* cylinder(float rTop, float rBottom, float h, int radial, int hs, bool openEnded) {
  Builder b;
  float halfH = h / 2, slope = (rBottom - rTop) / h;
  std::vector<std::vector<uint32_t>> grid(hs + 1, std::vector<uint32_t>(radial + 1));
  for (int y = 0; y <= hs; y++) {
    float v = (float)y / hs, rad = v * (rBottom - rTop) + rTop;
    for (int x = 0; x <= radial; x++) {
      float u = (float)x / radial, th = u * 2 * PI;
      float s = std::sin(th), c = std::cos(th);
      grid[y][x] = b.vertex({rad * s, -v * h + halfH, rad * c}, vec3{s, slope, c}.normalized(), u, 1 - v);
    }
  }
  for (int x = 0; x < radial; x++)
    for (int y = 0; y < hs; y++) {
      uint32_t a = grid[y][x], bb = grid[y + 1][x], c = grid[y + 1][x + 1], d = grid[y][x + 1];
      if (rTop > 0 || y != 0) b.tri(a, bb, d);
      if (rBottom > 0 || y != hs - 1) b.tri(bb, c, d);
    }
  if (!openEnded) {
    for (int top = 1; top >= 0; top--) {
      float rad = top ? rTop : rBottom;
      if (rad <= 0) continue;
      float sign = top ? 1.f : -1.f;
      uint32_t centerStart = (uint32_t)b.g->vertexCount();
      for (int x = 1; x <= radial; x++) b.vertex({0, halfH * sign, 0}, {0, sign, 0}, 0.5f, 0.5f);
      uint32_t rimStart = (uint32_t)b.g->vertexCount();
      for (int x = 0; x <= radial; x++) {
        float th = (float)x / radial * 2 * PI, s = std::sin(th), c = std::cos(th);
        b.vertex({rad * s, halfH * sign, rad * c}, {0, sign, 0}, c * 0.5f + 0.5f, s * 0.5f * sign + 0.5f);
      }
      for (int x = 0; x < radial; x++) {
        uint32_t c = centerStart + x, i = rimStart + x;
        if (top) b.tri(i, i + 1, c);
        else b.tri(i + 1, i, c);
      }
    }
  }
  return b.done();
}

Geometry* cone(float r, float h, int radial) { return cylinder(0, r, h, radial, 1, false); }

Geometry* capsule(float r, float length, int capSegments, int radialSegments) {
  // A lathe around y of a profile: bottom quarter arc, straight side, top quarter arc.
  struct P { float rad, y, nr, ny; };
  std::vector<P> prof;
  for (int i = 0; i <= capSegments; i++) {
    float a = -PI / 2 + (PI / 2) * i / capSegments;
    prof.push_back({r * std::cos(a), -length / 2 + r * std::sin(a), std::cos(a), std::sin(a)});
  }
  for (int i = 0; i <= capSegments; i++) {
    float a = (PI / 2) * i / capSegments;
    prof.push_back({r * std::cos(a), length / 2 + r * std::sin(a), std::cos(a), std::sin(a)});
  }
  Builder b;
  int rows = (int)prof.size();
  for (int k = 0; k < rows; k++)
    for (int x = 0; x <= radialSegments; x++) {
      float th = (float)x / radialSegments * 2 * PI, s = std::sin(th), c = std::cos(th);
      const P& p = prof[k];
      b.vertex({p.rad * s, p.y, p.rad * c}, vec3{p.nr * s, p.ny, p.nr * c}.normalized(), (float)x / radialSegments, (float)k / (rows - 1));
    }
  int stride = radialSegments + 1;
  for (int k = 0; k + 1 < rows; k++)
    for (int x = 0; x < radialSegments; x++) {
      uint32_t a = k * stride + x, c = a + 1, e = a + stride, g2 = e + 1;
      // Rows climb in y and columns turn counter-clockwise seen from above (towards +x from +z).
      b.tri(a, c, g2);
      b.tri(a, g2, e);
    }
  return b.done();
}

Geometry* torus(float R, float tube, int radialSegments, int tubularSegments) {
  Builder b;
  for (int j = 0; j <= radialSegments; j++)
    for (int i = 0; i <= tubularSegments; i++) {
      float u = (float)i / tubularSegments * 2 * PI, v = (float)j / radialSegments * 2 * PI;
      vec3 p{(R + tube * std::cos(v)) * std::cos(u), (R + tube * std::cos(v)) * std::sin(u), tube * std::sin(v)};
      vec3 c{R * std::cos(u), R * std::sin(u), 0};
      b.vertex(p, (p - c).normalized(), (float)i / tubularSegments, (float)j / radialSegments);
    }
  for (int j = 1; j <= radialSegments; j++)
    for (int i = 1; i <= tubularSegments; i++) {
      uint32_t a = (tubularSegments + 1) * j + i - 1, bb = (tubularSegments + 1) * (j - 1) + i - 1;
      uint32_t c = (tubularSegments + 1) * (j - 1) + i, d = (tubularSegments + 1) * j + i;
      b.tri(a, bb, d);
      b.tri(bb, c, d);
    }
  return b.done();
}

Geometry* icosahedron(float r, int detail) {
  const float t = (1 + std::sqrt(5.f)) / 2;
  const vec3 V[12] = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
                      {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
  const int F[60] = {0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                     3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9, 4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1};
  Builder b;
  int cols = detail + 1;
  auto emit = [&](vec3 a, vec3 bb, vec3 c) {
    a = a.normalized() * r; bb = bb.normalized() * r; c = c.normalized() * r;
    vec3 fn = cross(bb - a, c - a);
    if (dot(fn, a + bb + c) < 0) std::swap(bb, c);
    fn = cross(bb - a, c - a).normalized();
    uint32_t base = (uint32_t)b.g->vertexCount();
    for (const vec3& p : {a, bb, c}) b.vertex(p, detail ? p.normalized() : fn, 0, 0);
    b.tri(base, base + 1, base + 2);
  };
  for (int f = 0; f < 20; f++) {
    vec3 a = V[F[f * 3]], bb = V[F[f * 3 + 1]], c = V[F[f * 3 + 2]];
    std::vector<std::vector<vec3>> v(cols + 1);
    for (int i = 0; i <= cols; i++) {
      vec3 aj = lerp(a, c, (float)i / cols), bj = lerp(bb, c, (float)i / cols);
      int rows = cols - i;
      for (int j = 0; j <= rows; j++) v[i].push_back(j == 0 && i == cols ? aj : lerp(aj, bj, rows ? (float)j / rows : 0.f));
    }
    for (int i = 0; i < cols; i++)
      for (int j = 0; j < 2 * (cols - i) - 1; j++) {
        int k = j / 2;
        if (j % 2 == 0) emit(v[i][k + 1], v[i + 1][k], v[i][k]);
        else emit(v[i][k + 1], v[i + 1][k + 1], v[i + 1][k]);
      }
  }
  b.g->indices.clear();  // flat list of triangles; drawn without indices
  return b.done();
}

Geometry* tube(const Curve& curve, int segs, float radius, int radial, bool closed) {
  std::vector<vec3> T, N, B;
  curve.frames(segs, closed, T, N, B);
  Builder b;
  for (int i = 0; i <= segs; i++) {
    vec3 P = curve.pointAt((float)i / segs);
    for (int j = 0; j <= radial; j++) {
      float v = (float)j / radial * 2 * PI;
      float s = std::sin(v), c = -std::cos(v);
      vec3 n = (N[i] * c + B[i] * s).normalized();
      b.vertex(P + n * radius, n, (float)i / segs, (float)j / radial);
    }
  }
  for (int j = 1; j <= segs; j++)
    for (int i = 1; i <= radial; i++) {
      uint32_t a = (radial + 1) * (j - 1) + (i - 1), bb = (radial + 1) * j + (i - 1);
      uint32_t c = (radial + 1) * j + i, d = (radial + 1) * (j - 1) + i;
      b.tri(a, bb, d);
      b.tri(bb, c, d);
    }
  return b.done();
}

Geometry* edges(const Geometry& g, float thresholdDeg) {
  const float cosT = std::cos(radians(thresholdDeg));
  auto key = [](const vec3& p) {
    auto q = [](float v) { return (long long)std::llround(v * 10000.0); };
    return std::to_string(q(p.x)) + "," + std::to_string(q(p.y)) + "," + std::to_string(q(p.z));
  };
  struct E { vec3 a, b, n; bool open = true; };
  std::unordered_map<std::string, size_t> seen;
  std::vector<E> list;
  std::vector<vec3> out;
  size_t tris = g.elementCount() / 3;
  auto P = [&](size_t i) {
    uint32_t v = g.indices.empty() ? (uint32_t)i : g.indices[i];
    return vec3{g.positions[v * 3], g.positions[v * 3 + 1], g.positions[v * 3 + 2]};
  };
  for (size_t t = 0; t < tris; t++) {
    vec3 v[3] = {P(t * 3), P(t * 3 + 1), P(t * 3 + 2)};
    vec3 n = cross(v[1] - v[0], v[2] - v[0]);
    if (n.lengthSq() < 1e-20f) continue;
    n = n.normalized();
    std::string k[3] = {key(v[0]), key(v[1]), key(v[2])};
    for (int e = 0; e < 3; e++) {
      int a = e, bb = (e + 1) % 3;
      std::string h = k[a] + "_" + k[bb], rh = k[bb] + "_" + k[a];
      auto it = seen.find(rh);
      if (it != seen.end() && list[it->second].open) {
        E& other = list[it->second];
        if (dot(n, other.n) <= cosT) { out.push_back(other.a); out.push_back(other.b); }
        other.open = false;
      } else if (!seen.count(h)) {
        seen[h] = list.size();
        list.push_back({v[a], v[bb], n});
      }
    }
  }
  for (const E& e : list)
    if (e.open) { out.push_back(e.a); out.push_back(e.b); }
  return lineSegments(out);
}

Geometry* lineSegments(const std::vector<vec3>& pts) {
  Geometry* g = newGeometry();
  g->topology = Topology::Lines;
  for (const vec3& p : pts) g->positions.insert(g->positions.end(), {p.x, p.y, p.z});
  g->computeBounds();
  return g;
}

Geometry* points(const std::vector<vec3>& pts) {
  Geometry* g = lineSegments(pts);
  g->topology = Topology::Points;
  return g;
}


Geometry* flat(const Geometry& g) {
  Builder b;
  auto P = [&](uint32_t i) { return vec3{g.positions[i * 3], g.positions[i * 3 + 1], g.positions[i * 3 + 2]}; };
  bool hasUv = g.uvs.size() == g.vertexCount() * 2;
  size_t n = g.elementCount();
  for (size_t t = 0; t + 2 < n; t += 3) {
    uint32_t i[3];
    for (int k = 0; k < 3; k++) i[k] = g.indices.empty() ? (uint32_t)(t + k) : g.indices[t + k];
    vec3 a = P(i[0]), c1 = P(i[1]), c2 = P(i[2]);
    vec3 nrm = cross(c1 - a, c2 - a);
    if (nrm.lengthSq() < 1e-20f) continue;
    nrm = nrm.normalized();
    uint32_t base = (uint32_t)b.g->vertexCount();
    for (int k = 0; k < 3; k++) b.vertex(P(i[k]), nrm, hasUv ? g.uvs[i[k] * 2] : 0, hasUv ? g.uvs[i[k] * 2 + 1] : 0);
    b.tri(base, base + 1, base + 2);
  }
  return b.done();
}

namespace {
// Ear clipping for a simple counter-clockwise polygon; returns index triples.
std::vector<uint32_t> triangulate(const std::vector<vec2>& p) {
  std::vector<uint32_t> out, idx(p.size());
  for (size_t i = 0; i < p.size(); i++) idx[i] = (uint32_t)i;
  auto crossz = [](vec2 a, vec2 b, vec2 c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); };
  auto inside = [&](vec2 q, vec2 a, vec2 b, vec2 c) { return crossz(a, b, q) >= 0 && crossz(b, c, q) >= 0 && crossz(c, a, q) >= 0; };
  int guard = 0;
  while (idx.size() > 3 && guard++ < 100000) {
    bool clipped = false;
    for (size_t k = 0; k < idx.size(); k++) {
      uint32_t ia = idx[(k + idx.size() - 1) % idx.size()], ib = idx[k], ic = idx[(k + 1) % idx.size()];
      vec2 a = p[ia], b = p[ib], c = p[ic];
      if (crossz(a, b, c) <= 1e-9f) continue;  // reflex or flat corner
      bool ear = true;
      for (uint32_t j : idx)
        if (j != ia && j != ib && j != ic && inside(p[j], a, b, c)) { ear = false; break; }
      if (!ear) continue;
      out.insert(out.end(), {ia, ib, ic});
      idx.erase(idx.begin() + (long)k);
      clipped = true;
      break;
    }
    if (!clipped) break;  // degenerate polygon: give up on the rest
  }
  if (idx.size() == 3) out.insert(out.end(), {idx[0], idx[1], idx[2]});
  return out;
}
}  // namespace

Geometry* extrude(const std::vector<vec2>& outlineIn, float depth, Part part) {
  std::vector<vec2> pts = outlineIn;
  float area = 0;
  for (size_t i = 0; i < pts.size(); i++) {
    const vec2 &a = pts[i], &c = pts[(i + 1) % pts.size()];
    area += a.x * c.y - c.x * a.y;
  }
  if (area < 0) std::reverse(pts.begin(), pts.end());
  vec2 mn{1e30f, 1e30f}, mx{-1e30f, -1e30f};
  for (vec2 q : pts) {
    mn = {std::min(mn.x, q.x), std::min(mn.y, q.y)};
    mx = {std::max(mx.x, q.x), std::max(mx.y, q.y)};
  }
  float w = std::max(mx.x - mn.x, 1e-6f), h = std::max(mx.y - mn.y, 1e-6f);
  Builder b;
  float hz = depth / 2;
  if (part != Part::Edges) {
    std::vector<uint32_t> tris = triangulate(pts);
    for (int side = 0; side < 2; side++) {
      float z = side == 0 ? hz : -hz;
      vec3 nrm{0, 0, side == 0 ? 1.f : -1.f};
      uint32_t base = (uint32_t)b.g->vertexCount();
      for (vec2 q : pts) b.vertex({q.x, q.y, z}, nrm, (q.x - mn.x) / w, (q.y - mn.y) / h);
      for (size_t t = 0; t + 2 < tris.size(); t += 3) {
        if (side == 0) b.tri(base + tris[t], base + tris[t + 1], base + tris[t + 2]);
        else b.tri(base + tris[t], base + tris[t + 2], base + tris[t + 1]);
      }
    }
  }
  if (part != Part::Faces) {
    float perim = 0;
    for (size_t i = 0; i < pts.size(); i++) perim += (pts[(i + 1) % pts.size()] - pts[i]).length();
    float u = 0;
    for (size_t i = 0; i < pts.size(); i++) {
      vec2 a = pts[i], c = pts[(i + 1) % pts.size()];
      vec2 e = c - a;
      float len = e.length();
      if (len < 1e-7f) continue;
      vec3 nrm = vec3{e.y, -e.x, 0}.normalized();
      uint32_t base = (uint32_t)b.g->vertexCount();
      float u1 = u + len / perim;
      b.vertex({a.x, a.y, hz}, nrm, u, 1);
      b.vertex({c.x, c.y, hz}, nrm, u1, 1);
      b.vertex({a.x, a.y, -hz}, nrm, u, 0);
      b.vertex({c.x, c.y, -hz}, nrm, u1, 0);
      b.tri(base, base + 2, base + 1);
      b.tri(base + 1, base + 2, base + 3);
      u = u1;
    }
  }
  return b.done();
}

Geometry* ribbon(const Curve& curve, int segments, float width, vec3 up) {
  Builder b;
  for (int i = 0; i <= segments; i++) {
    float t = (float)i / segments;
    vec3 p = curve.pointAt(t), tg = curve.tangentAt(t);
    vec3 side = cross(tg, up);
    if (side.lengthSq() < 1e-10f) side = cross(tg, vec3{1, 0, 0});
    side = side.normalized();
    vec3 nrm = cross(side, tg).normalized();
    b.vertex(p - side * (width / 2), nrm, t, 0);
    b.vertex(p + side * (width / 2), nrm, t, 1);
  }
  for (int i = 0; i < segments; i++) {
    uint32_t a = (uint32_t)i * 2;
    b.tri(a, a + 1, a + 2);
    b.tri(a + 1, a + 3, a + 2);
  }
  return b.done();
}

Geometry* merge(const std::vector<std::pair<const Geometry*, mat4>>& parts) {
  Builder b;
  for (const auto& [g, m] : parts) {
    mat3 nm = m.normalMatrix();
    uint32_t base = (uint32_t)b.g->vertexCount();
    size_t nv = g->vertexCount();
    bool hasN = g->normals.size() == g->positions.size(), hasUv = g->uvs.size() == nv * 2;
    for (size_t i = 0; i < nv; i++) {
      vec3 p = m.transformPoint({g->positions[i * 3], g->positions[i * 3 + 1], g->positions[i * 3 + 2]});
      vec3 nrm = hasN ? (nm * vec3{g->normals[i * 3], g->normals[i * 3 + 1], g->normals[i * 3 + 2]}).normalized() : vec3{0, 1, 0};
      b.vertex(p, nrm, hasUv ? g->uvs[i * 2] : 0, hasUv ? g->uvs[i * 2 + 1] : 0);
    }
    if (g->indices.empty())
      for (size_t i = 0; i + 2 < nv; i += 3) b.tri(base + (uint32_t)i, base + (uint32_t)i + 1, base + (uint32_t)i + 2);
    else
      for (size_t i = 0; i + 2 < g->indices.size(); i += 3) b.tri(base + g->indices[i], base + g->indices[i + 1], base + g->indices[i + 2]);
  }
  return b.done();
}

}  // namespace geo
}  // namespace eng
