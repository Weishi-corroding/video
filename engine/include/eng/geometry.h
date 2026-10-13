// Triangle, line and point geometry, and builders for the shapes the films use.
#pragma once
#include <cstdint>
#include <vector>
#include "math.h"

namespace eng {

struct Curve;

enum class Topology { Triangles, Lines, Points };

struct Geometry {
  Topology topology = Topology::Triangles;
  std::vector<float> positions;   // x y z per vertex
  std::vector<float> normals;     // x y z per vertex (may be empty for lines and points)
  std::vector<float> uvs;         // u v per vertex (may be empty)
  std::vector<uint32_t> indices;  // empty for non-indexed geometry
  // Only part of the geometry is drawn: [drawStart, drawStart + drawCount) of the indices (or vertices).
  uint32_t drawStart = 0, drawCount = UINT32_MAX;

  vec3 boundsCenter;
  float boundsRadius = 0;

  // Renderer bookkeeping.
  int gpuSlot = -1;

  size_t vertexCount() const { return positions.size() / 3; }
  size_t elementCount() const { return indices.empty() ? vertexCount() : indices.size(); }
  void setDrawRange(uint32_t start, uint32_t count) { drawStart = start; drawCount = count; }
  void computeBounds();
  Geometry& applyMatrix(const mat4& m);
  Geometry& rotateX(float a) { return applyMatrix(mat4::compose({}, quat::axisAngle({1, 0, 0}, a), {1, 1, 1})); }
  Geometry& rotateY(float a) { return applyMatrix(mat4::compose({}, quat::axisAngle({0, 1, 0}, a), {1, 1, 1})); }
  Geometry& translate(float x, float y, float z) { return applyMatrix(mat4::translation({x, y, z})); }
};

// All geometry lives for the life of the program; builders return pointers owned by the engine.
Geometry* newGeometry();

namespace geo {
Geometry* box(float w, float h, float d);
// A box with rounded edges and corners; `segments` steps per quarter arc on each face.
Geometry* roundedBox(float w, float h, float d, int segments, float radius);
Geometry* plane(float w, float h);
Geometry* circle(float r, int segments);
Geometry* sphere(float r, int widthSegments = 32, int heightSegments = 16, float phiStart = 0, float phiLength = 2 * PI,
                 float thetaStart = 0, float thetaLength = PI);
Geometry* cylinder(float rTop, float rBottom, float h, int radial = 32, int heightSegments = 1, bool openEnded = false);
Geometry* cone(float r, float h, int radial = 32);
// A capsule along y: hemispheres of `r` on a straight section of `length`.
Geometry* capsule(float r, float length, int capSegments = 4, int radialSegments = 8);
// A torus around the z axis.
Geometry* torus(float r, float tube, int radialSegments, int tubularSegments);
Geometry* icosahedron(float r, int detail);
// A tube along a curve. Indices are ordered along the curve, so a draw range of
// k * radialSegments * 6 indices draws the first k segments.
Geometry* tube(const Curve& curve, int tubularSegments, float radius, int radialSegments, bool closed = false);
// Line segments along the edges of a triangle mesh where neighbouring faces meet at more than thresholdDeg.
Geometry* edges(const Geometry& g, float thresholdDeg = 1);
Geometry* lineSegments(const std::vector<vec3>& points);
Geometry* points(const std::vector<vec3>& points);

// A copy where every triangle has its own vertices and its face normal: crisp folds, as in papercraft.
Geometry* flat(const Geometry& g);
// A shape cut from card: a simple polygon in x and y, extruded `depth` along z and centred on z = 0.
// `part` picks the broad faces, the cut edge, or both (so the edge can take its own material).
enum class Part { All, Faces, Edges };
Geometry* extrude(const std::vector<vec2>& outline, float depth, Part part = Part::All);
// A thin strip along a curve, `width` wide, lying across the curve with its normal toward `up` (use
// Side::Double to see both sides). Indices run along the curve: a draw range of k * 6 indices draws
// the first k segments.
Geometry* ribbon(const Curve& curve, int segments, float width, vec3 up = {0, 1, 0});
// Several geometries combined into one, each placed by a matrix.
Geometry* merge(const std::vector<std::pair<const Geometry*, mat4>>& parts);
}  // namespace geo

}  // namespace eng
