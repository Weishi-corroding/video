// The scene graph: nodes with a transform, meshes, instanced meshes, lines, points, sprites, lights,
// the camera, and the scene's sky, fog, sun and ambient light.
#pragma once
#include <vector>
#include "geometry.h"
#include "material.h"
#include "math.h"

namespace eng {

enum class NodeKind { Group, Mesh, Instanced, Lines, Points, Sprite, PointLight };

struct Node {
  NodeKind kind = NodeKind::Group;
  vec3 position{0, 0, 0};
  vec3 rotation{0, 0, 0};  // Euler angles in radians, applied in XYZ order
  vec3 scale{1, 1, 1};
  bool visible = true;
  bool castShadow = false, receiveShadow = false;
  bool frustumCulled = true;
  int renderOrder = 0;
  Node* parent = nullptr;
  std::vector<Node*> children;
  mat4 world;  // updated by Scene::updateWorld()

  explicit Node(NodeKind k = NodeKind::Group) : kind(k) {}
  virtual ~Node() = default;

  template <class... T>
  Node* add(T*... c) {
    (attach(c), ...);
    return this;
  }
  void attach(Node* c);
  void remove(Node* c);

  quat quaternion() const { return quat::euler(rotation); }
  void setQuaternion(const quat& q) { rotation = q.toEuler(); }
  void rotateX(float a) { setQuaternion(quaternion() * quat::axisAngle({1, 0, 0}, a)); }
  Node* translateY(float d) {
    position += quaternion().rotate({0, d, 0});
    return this;
  }
  // Turn so the local +z axis points at a world-space target.
  void lookAt(const vec3& target);
  mat4 localMatrix() const { return mat4::compose(position, quaternion(), scale); }
  // World matrix from the ancestors' current transforms (no caching).
  mat4 computeWorld() const;
  vec3 worldPosition() const { return computeWorld().position(); }
  void updateWorld(const mat4& parentWorld);

  template <class F>
  void traverse(F&& f) {
    f(this);
    for (Node* c : children) c->traverse(f);
  }
};

struct Mesh : Node {
  Geometry* geometry = nullptr;
  Material* material = nullptr;
  Mesh(Geometry* g, Material* m, NodeKind k = NodeKind::Mesh) : Node(k), geometry(g), material(m) {}
};

// Many copies of one mesh, each with its own matrix (relative to the node) and optional colour.
struct InstancedMesh : Mesh {
  std::vector<mat4> matrices;
  std::vector<Color> colors;  // empty, or one per instance (multiplies the material colour)
  InstancedMesh(Geometry* g, Material* m, int count) : Mesh(g, m, NodeKind::Instanced), matrices(count) {}
  int count() const { return (int)matrices.size(); }
  void setColor(int i, Color c) {
    if (colors.empty()) colors.assign(matrices.size(), Color{1, 1, 1});
    colors[i] = c;
  }
};

struct PointLight : Node {
  Color color{1, 1, 1};
  float intensity = 1, distance = 0, decay = 2;
  PointLight() : Node(NodeKind::PointLight) {}
};

struct DirectionalLight {
  Color color{1, 1, 1};
  float intensity = 1;
  vec3 position{0, 1, 0}, target{0, 0, 0};
  bool castShadow = false;
  struct {
    int mapSize = 2048;
    float left = -10, right = 10, bottom = -10, top = 10, near = 0.5f, far = 500;
    float bias = 0, normalBias = 0, radius = 1;
  } shadow;
};

struct HemisphereLight {
  Color sky{1, 1, 1}, ground{0, 0, 0};
  float intensity = 1;
};

struct Camera {
  vec3 position{0, 0, 10}, target{0, 0, 0}, up{0, 1, 0};
  float fov = 50;  // vertical, degrees
  float aspect = 16.f / 9, near = 0.1f, far = 1000;
  mat4 view, proj;
  void update() {
    view = mat4::lookAtView(position, target, up);
    proj = mat4::perspective(radians(fov), aspect, near, far);
  }
  // Normalised device coordinates of a world point: x, y in -1..1 with y up; z > 1 means behind the camera.
  vec3 project(const vec3& p) const {
    vec4 c = proj * (view * vec4(p, 1));
    // proj maps depth to 0..1; re-express it in the -1..1 range so that z > 1 marks points behind.
    float z = c.z / c.w * 2 - 1;
    return {c.x / c.w, c.y / c.w, z};
  }
};

struct Scene : Node {
  struct {
    bool enabled = false;
    Color color{1, 1, 1};
    float near = 1, far = 1000;
  } fog;
  // A vertical gradient on a sphere around the origin.
  struct {
    Color top{0, 0, 0}, horizon{0, 0, 0};
    float radius = 400;
  } sky;
  HemisphereLight hemi;
  DirectionalLight sun;   // the only light that casts shadows
  DirectionalLight fill;  // a second, shadowless directional light (intensity 0 = off)
  float exposure = 1;
  // The paper look for materials with `paper` > 0: a tileable sheet of fibres mapped in object space.
  struct {
    float tile = 3;        // world units covered by one tile of the fibre pattern
    float relief = 0.6f;   // height of the fibre relief (hundredths of a world unit)
    float mottle = 0.14f;  // how much the fibres vary the colour
    float wrap = 0.3f;     // diffuse light wrapping past the terminator, as in thin paper
    float sky = 0.06f;     // fibre texture on the sky gradient, as on a painted backdrop
  } paper;
  Scene() : Node(NodeKind::Group) { fill.intensity = 0; }
  void updateWorld() { Node::updateWorld(mat4::identity()); }
};

// Nodes are owned by the engine and live for the whole program.
Node* group();
Mesh* mesh(Geometry* g, Material* m);
Mesh* lines(Geometry* g, Material* m);
Mesh* pointCloud(Geometry* g, Material* m);
// A camera-facing unit quad, scaled by the node's scale.
Mesh* sprite(Material* m);
InstancedMesh* instanced(Geometry* g, Material* m, int count);
PointLight* pointLight(uint32_t color, float intensity, float distance, float decay);

}  // namespace eng
