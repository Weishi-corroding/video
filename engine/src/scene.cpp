#include <algorithm>
#include <memory>

#include "eng/scene.h"

namespace eng {

namespace {
template <class T>
std::vector<std::unique_ptr<T>>& pool() {
  static std::vector<std::unique_ptr<T>> p;
  return p;
}
template <class T, class... A>
T* make(A&&... a) {
  pool<Node>().push_back(std::make_unique<T>(std::forward<A>(a)...));
  return static_cast<T*>(pool<Node>().back().get());
}
}  // namespace

void Node::attach(Node* c) {
  if (!c) return;
  if (c->parent) c->parent->remove(c);
  c->parent = this;
  children.push_back(c);
}

void Node::remove(Node* c) {
  auto it = std::find(children.begin(), children.end(), c);
  if (it != children.end()) {
    children.erase(it);
    c->parent = nullptr;
  }
}

mat4 Node::computeWorld() const {
  mat4 m = localMatrix();
  for (const Node* p = parent; p; p = p->parent) m = p->localMatrix() * m;
  return m;
}

void Node::updateWorld(const mat4& parentWorld) {
  world = parentWorld * localMatrix();
  for (Node* c : children) c->updateWorld(world);
}

void Node::lookAt(const vec3& target) {
  vec3 pos = computeWorld().position();
  // The inverse of a view matrix from the target back to this node has its +z axis pointing at the target.
  quat q = quat::fromMatrix(mat4::lookAtView(target, pos, {0, 1, 0}).inverse());
  if (parent) q = quat::fromMatrix(parent->computeWorld()).conjugate() * q;
  setQuaternion(q.normalized());
}

Node* group() { return make<Node>(NodeKind::Group); }
Mesh* mesh(Geometry* g, Material* m) { return make<Mesh>(g, m, NodeKind::Mesh); }
Mesh* lines(Geometry* g, Material* m) { return make<Mesh>(g, m, NodeKind::Lines); }
Mesh* pointCloud(Geometry* g, Material* m) { return make<Mesh>(g, m, NodeKind::Points); }
Mesh* sprite(Material* m) {
  static Geometry* quad = geo::plane(1, 1);
  return make<Mesh>(quad, m, NodeKind::Sprite);
}
InstancedMesh* instanced(Geometry* g, Material* m, int count) { return make<InstancedMesh>(g, m, count); }
PointLight* pointLight(uint32_t color, float intensity, float distance, float decay) {
  PointLight* l = make<PointLight>();
  l->color = Color::hex(color);
  l->intensity = intensity;
  l->distance = distance;
  l->decay = decay;
  return l;
}

// ---- materials and textures ----
namespace {
std::vector<std::unique_ptr<Material>>& materials() {
  static std::vector<std::unique_ptr<Material>> m;
  return m;
}
std::vector<std::unique_ptr<Texture>>& textures() {
  static std::vector<std::unique_ptr<Texture>> t;
  return t;
}
}  // namespace

Material* newMaterial() {
  materials().push_back(std::make_unique<Material>());
  return materials().back().get();
}

Material* Material::clone() const {
  Material* m = newMaterial();
  *m = *this;
  return m;
}

Material* standard(const StandardParams& p) {
  Material* m = newMaterial();
  m->shading = Shading::Standard;
  m->color = Color::hex(p.color);
  m->roughness = p.roughness;
  m->metalness = p.metalness;
  m->emissive = Color::hex(p.emissive);
  m->emissiveIntensity = p.emissiveIntensity;
  m->opacity = p.opacity;
  m->transparent = p.transparent;
  m->depthWrite = p.depthWrite;
  m->side = p.side;
  m->map = p.map;
  m->paper = p.paper;
  return m;
}

Material* basic(const BasicParams& p) {
  Material* m = newMaterial();
  m->shading = Shading::Basic;
  m->color = Color::hex(p.color);
  m->opacity = p.opacity;
  m->transparent = p.transparent;
  m->depthWrite = p.depthWrite;
  m->depthTest = p.depthTest;
  m->fog = p.fog;
  m->side = p.side;
  m->map = p.map;
  return m;
}

Texture* newTexture(int width, int height) {
  textures().push_back(std::make_unique<Texture>());
  Texture* t = textures().back().get();
  t->width = width;
  t->height = height;
  t->pixels.assign((size_t)width * height * 4, 0);
  return t;
}

}  // namespace eng
