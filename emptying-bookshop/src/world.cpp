#include "world.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace film {

namespace {

// ---------------- lighting presets ----------------
struct Preset {
  uint32_t sun;
  float sunI, el, az;
  uint32_t hemiS, hemiG;
  float hemiI;
  uint32_t top, hor;
  float night, stars, exp;
};
const std::map<std::string, Preset> PRESETS = {
    {"day", {0xfff0dc, 2.7f, 40, 215, 0xd6e4ff, 0x6e5c48, 1.15f, 0x1d2a40, 0x51647a, 0, 0, 1.0f}},
    {"overcast", {0xe4e9f0, 1.25f, 55, 200, 0xdfe6ef, 0x5d5a55, 1.45f, 0x262d38, 0x5a6270, 0, 0, 0.98f}},
    {"golden", {0xffc48a, 2.6f, 20, 240, 0xf3d6c0, 0x5e4a3c, 0.95f, 0x22253f, 0x8a5e5a, 0.15f, 0, 1.0f}},
    {"dusk", {0xff9f70, 1.4f, 12, 250, 0x8a8fc0, 0x3a3038, 0.85f, 0x161b33, 0x5e4058, 0.55f, 0.2f, 1.02f}},
    {"evening", {0xa9bcff, 0.75f, 38, 150, 0x5d6fa0, 0x24222c, 0.75f, 0x0b1022, 0x2b3050, 1, 0.6f, 1.05f}},
    {"night", {0x9fb2ff, 0.55f, 50, 160, 0x4a5a8a, 0x1c1b24, 0.7f, 0x070a18, 0x1d2140, 1, 1, 1.05f}},
    {"analysis", {0xffe2c4, 1.7f, 30, 225, 0xb9c6e6, 0x3e3a40, 1.0f, 0x141b2c, 0x3a4458, 0.35f, 0.1f, 1.0f}},
};

// A preset with its colours in linear light, so blends happen there.
struct Light {
  Color sun, hemiS, hemiG, top, hor;
  float sunI, el, az, hemiI, night, stars, exp;
};
Light toLight(const Preset& p) {
  return {Color::hex(p.sun), Color::hex(p.hemiS), Color::hex(p.hemiG), Color::hex(p.top), Color::hex(p.hor),
          p.sunI, p.el, p.az, p.hemiI, p.night, p.stars, p.exp};
}
Light mixLight(const Light& a, const Light& b, float f) {
  return {a.sun.lerp(b.sun, f), a.hemiS.lerp(b.hemiS, f), a.hemiG.lerp(b.hemiG, f), a.top.lerp(b.top, f), a.hor.lerp(b.hor, f),
          lerp(a.sunI, b.sunI, f), lerp(a.el, b.el, f), lerp(a.az, b.az, f), lerp(a.hemiI, b.hemiI, f), lerp(a.night, b.night, f),
          lerp(a.stars, b.stars, f), lerp(a.exp, b.exp, f)};
}
const Preset& preset(const char* name) {
  auto it = PRESETS.find(name);
  if (it == PRESETS.end()) throw std::runtime_error(std::string("unknown light preset ") + name);
  return it->second;
}

Light presetAt(const std::vector<LightKey>& keys, float t) {
  // keys: arrive at preset p by time t, blending over d seconds.
  Light cur = toLight(preset(keys[0].p));
  for (size_t i = 1; i < keys.size(); i++) {
    const LightKey& k = keys[i];
    if (t >= k.t) { cur = toLight(preset(k.p)); continue; }
    if (t > k.t - k.d) return mixLight(cur, toLight(preset(k.p)), smooth((t - (k.t - k.d)) / k.d));
    break;
  }
  return cur;
}

void updateLight(World& W, float t) {
  Light L = presetAt(W.lightKeys, t);
  float el = radians(L.el), az = radians(L.az);
  Scene& s = W.scene;
  s.sun.position = s.sun.target + vec3{std::cos(el) * std::sin(az) * 90, std::sin(el) * 90, std::cos(el) * std::cos(az) * 90};
  s.sun.color = L.sun;
  s.sun.intensity = L.sunI;
  s.hemi.sky = L.hemiS;
  s.hemi.ground = L.hemiG;
  s.hemi.intensity = L.hemiI;
  s.sky.top = L.top;
  s.sky.horizon = L.hor;
  s.fog.color = L.hor;
  W.starMat->opacity = L.stars;
  s.exposure = L.exp;
  W.night = L.night;
  for (Material* m : W.nightMats) m->emissiveIntensity = m->user[1] * L.night;
  for (Material* m : W.lamps) m->emissiveIntensity = 2.4f * clamp((L.night - 0.25f) / 0.5f);
}

// ---------------- camera ----------------
float poseAt(const CamKey& k, float t, vec3& outP, vec3& outL) {
  outP = k.p;
  outL = k.l;
  // A slow push-in while holding a shot, capped so it never becomes a zoom.
  float hold = std::max(0.f, t - k.t);
  float push = std::min(k.push, hold * 0.004f);
  outP = lerp(outP, outL, push);
  return k.fov;
}

void updateCamera(World& W, float t) {
  const auto& keys = W.camKeys;
  size_t i = 0;
  while (i + 1 < keys.size() && t >= keys[i + 1].t - keys[i + 1].d) i++;
  const CamKey& k = keys[i];
  vec3 pA, lA, pB, lB;
  float fov = poseAt(k, t, pA, lA);
  // Moving toward key i (from key i-1) if we are inside its move window.
  if (i > 0 && t < k.t) {
    const CamKey& prev = keys[i - 1];
    float start = k.t - k.d;
    float f = smoother((t - start) / k.d);
    float fovA = poseAt(prev, start, pB, lB);
    float fovB = poseAt(k, k.t, pA, lA);
    pA = lerp(pB, pA, f);
    lA = lerp(lB, lA, f);
    if (k.arc != 0) pA.y += std::sin(f * PI) * k.arc;
    fov = lerp(fovA, fovB, f);
  }
  // Narrow screens: widen the view so the horizontal framing survives, then dolly back.
  const float designAspect = 16.f / 9;
  Camera& cam = W.camera;
  if (cam.aspect < designAspect) {
    float hTan = std::tan(radians(fov / 2)) * designAspect;
    float vTan = hTan / cam.aspect;
    float maxTan = std::tan(radians(33));
    float back = 1;
    if (vTan > maxTan) { back = vTan / maxTan; vTan = maxTan; }
    fov = degrees(std::atan(vTan)) * 2;
    pA = (pA - lA) * lerp(1, back, 0.85f) + lA;
  }
  cam.position = pA;
  cam.target = lA;
  cam.fov = fov;
  cam.update();
}

// ---------------- campus ----------------
Node* windowGrid(World& W, double w, double h, int rows, int cols, double y0, char face, double depth, uint32_t color = 0x5f7488,
                 uint32_t lit = 0xffd59a, float nightI = 1.6f, float ww = 0.9f, float wh = 1.1f) {
  // Windows on one facade: two instanced meshes, one that lights up at night and one that stays dark.
  Material* on = standard({.color = color, .roughness = 0.35f, .metalness = 0.1f, .emissive = lit, .emissiveIntensity = 0});
  on->user[1] = nightI;
  W.nightMats.push_back(on);
  Material* off = matS(color, 0.35f, 0.1f);
  static std::map<std::pair<float, float>, Geometry*> planes;
  Geometry*& g = planes[{ww, wh}];
  if (!g) g = geo::plane(ww, wh);
  Rng r((uint32_t)std::llround(w * 13 + h * 7 + rows + cols * 3));
  std::vector<mat4> lists[2];
  const double x0 = 0, z0 = 0;
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++) {
      double u = (j + 0.5) / cols - 0.5;
      double y = y0 + i * (h / rows);
      vec3 p;
      float ry = 0;
      if (face == 'S') { p = {(float)(x0 + u * w), (float)y, (float)(z0 + depth / 2 + 0.02)}; ry = 0; }
      if (face == 'N') { p = {(float)(x0 - u * w), (float)y, (float)(z0 - depth / 2 - 0.02)}; ry = PI; }
      if (face == 'E') { p = {(float)(x0 + depth / 2 + 0.02), (float)y, (float)(z0 - u * w)}; ry = PI / 2; }
      if (face == 'W') { p = {(float)(x0 - depth / 2 - 0.02), (float)y, (float)(z0 + u * w)}; ry = -PI / 2; }
      lists[r() < 0.72 ? 0 : 1].push_back(mat4::compose(p, quat::euler({0, ry, 0}), {1, 1, 1}));
    }
  Node* grp = group();
  Material* mats[2] = {on, off};
  for (int k = 0; k < 2; k++) {
    if (lists[k].empty()) continue;
    InstancedMesh* inst = instanced(g, mats[k], (int)lists[k].size());
    inst->matrices = lists[k];
    inst->receiveShadow = true;
    grp->add(inst);
  }
  return grp;
}

struct BuildingOpts {
  float x, z, w, d, h;
  uint32_t color;
  uint32_t roof = PAL::roofGrey;
  std::vector<std::pair<char, int>> faces;
  int floors = 2;
};

Node* building(World& W, const BuildingOpts& o) {
  Node* g = group();
  g->add(place(rbox(o.w, o.h, o.d, o.color, 0.12f), 0, o.h / 2, 0));
  g->add(place(rbox(o.w + 0.12f, 0.45f, o.d + 0.12f, (Color::hex(o.color) * 0.78f).toHex(), 0.06f), 0, 0.22f, 0));
  g->add(place(rbox(o.w + 0.3f, 0.22f, o.d + 0.3f, Color::hex(o.color).lerp(Color::hex(0xffffff), 0.35f).toHex(), 0.05f), 0, o.h - 0.05f, 0));
  g->add(place(rbox(o.w + 0.4f, 0.3f, o.d + 0.4f, o.roof, 0.08f), 0, o.h + 0.15f, 0));
  g->add(place(rbox(o.w * 0.18f, 0.5f, o.d * 0.22f, 0x8a929c, 0.05f), o.w * 0.22f, o.h + 0.55f, -o.d * 0.15f));
  for (auto [face, cols] : o.faces) {
    bool sn = face == 'S' || face == 'N';
    double span = sn ? o.w : o.d;
    g->add(windowGrid(W, span * 0.86, (double)o.h * 0.82, o.floors, cols, (double)o.h * 0.3, face, sn ? o.d : o.w));
  }
  g->position = {o.x, 0, o.z};
  return g;
}

Mesh* pathSeg(std::pair<float, float> a, std::pair<float, float> b, float w, Material* m) {
  float dx = b.first - a.first, dz = b.second - a.second;
  float len = std::hypot(dx, dz);
  Mesh* x = prop(geo::box(len + w * 0.02f, 0.04f, w), m, false);
  x->position = {(a.first + b.first) / 2, 0.02f, (a.second + b.second) / 2};
  x->rotation.y = -std::atan2(dz, dx);
  return x;
}

Mesh* joint(std::pair<float, float> p, float w, Material* m) {
  Mesh* x = prop(cylinderGeo(w / 2, w / 2, 0.04f, 20), m, false);
  x->position = {p.first, 0.02f, p.second};
  return x;
}

void buildCampus(World& W) {
  Node* campus = group();
  W.campus = campus;
  W.scene.add(campus);

  // Diorama base.
  campus->add(place(rbox(67, 3, 67, matS(PAL::plinth, 0.9f), 0.6f), 0, -1.6f, -2));
  campus->add(place(rbox(65.4f, 0.2f, 65.4f, matS(PAL::lawn, 0.95f), 0.1f), 0, -0.1f, -2));

  Material* pm = matS(PAL::path, 0.95f);
  struct PathDef { std::pair<float, float> a, b; float w; };
  const std::vector<PathDef> paths = {
      {{0, 19}, {0, -10.4f}, 2.4f},   {{-17.5f, 2}, {19.9f, 2}, 2.2f},     {{-9, 19}, {13, 19}, 1.8f},
      {{-17.5f, -0.2f}, {-17.5f, 8}, 1.6f}, {{-18.6f, 0}, {-17.5f, 0}, 1.4f}, {{-18.6f, 7}, {-17.5f, 7}, 1.4f},
      {{-7, 19}, {-7, 20.4f}, 1.4f},  {{11, 19}, {11, 20.4f}, 1.4f},
  };
  for (const auto& p : paths) campus->add(pathSeg(p.a, p.b, p.w, pm));
  for (auto p : std::vector<std::pair<float, float>>{{0, 19}, {0, 2}, {-17.5f, 2}, {-17.5f, 7}, {-17.5f, 0}}) campus->add(joint(p, 2.4f, pm));
  // Library plaza.
  campus->add(place(prop(geo::box(12, 0.04f, 3.2f), pm, false), 0, 0.02f, -10.4f));
  // The narrow side path to the shop.
  Material* sm = matS(0xd8cbb2, 0.95f);
  for (size_t i = 0; i + 1 < SIDE_PATH.size(); i++) campus->add(pathSeg(SIDE_PATH[i], SIDE_PATH[i + 1], 1.3f, sm));
  for (size_t i = 1; i + 1 < SIDE_PATH.size(); i++) campus->add(joint(SIDE_PATH[i], 1.3f, sm));
  // Old shop forecourt.
  campus->add(place(prop(geo::box(9, 0.04f, 2), pm, false), 9, 0.02f, 3.6f));

  // Library: wide, columns at the front.
  Node* lib = building(W, {.x = 0, .z = -16, .w = 18, .d = 8, .h = 7, .color = PAL::stone, .roof = 0x7f8a96, .faces = {{'S', 9}, {'E', 4}, {'W', 4}}, .floors = 2});
  for (int i = 0; i < 6; i++) lib->add(place(prop(cylinderGeo(0.28f, 0.32f, 5.6f, 14), matS(0xeee4d0, 0.7f)), -5.f + i * 2, 2.8f, 4.6f));
  lib->add(place(rbox(12, 0.5f, 2, PAL::stone, 0.06f), 0, 5.85f, 4.4f));
  lib->add(place(rbox(11, 0.3f, 1.6f, 0xd2c4aa, 0.04f), 0, 0.15f, 4.9f));
  W.libraryGroup = lib;
  campus->add(lib);

  campus->add(building(W, {.x = -22.5f, .z = -3, .w = 8, .d = 9, .h = 6, .color = PAL::brick, .faces = {{'E', 4}, {'S', 3}}, .floors = 3}));
  campus->add(building(W, {.x = -22.5f, .z = 9, .w = 8, .d = 8, .h = 5, .color = 0xc4a07c, .faces = {{'E', 3}, {'S', 3}, {'N', 3}}, .floors = 2}));
  campus->add(building(W, {.x = -7, .z = 23, .w = 12, .d = 5, .h = 5.2f, .color = 0xd9c3a5, .roof = PAL::roof, .faces = {{'N', 6}, {'E', 2}}, .floors = 3}));
  campus->add(building(W, {.x = 11, .z = 23, .w = 10, .d = 5, .h = 5.2f, .color = 0xcdb79a, .roof = PAL::roof, .faces = {{'N', 5}, {'W', 2}}, .floors = 3}));
  campus->add(building(W, {.x = 23.5f, .z = 3, .w = 7, .d = 9, .h = 3.6f, .color = 0xe2d2b4, .roof = 0x5f8a7a, .faces = {{'W', 4}, {'S', 3}}, .floors = 1}));

  // Student centre, whose left unit was the shop's old home (now empty).
  Node* sc = building(W, {.x = 9, .z = 6.5f, .w = 9, .d = 5, .h = 4.4f, .color = 0xd5c6ae, .faces = {{'E', 2}}, .floors = 2});
  sc->add(place(rbox(3.6f, 2.2f, 0.1f, mat(0x2c3440, {.rough = 0.3f}), 0.02f), -2.2f, 1.4f, -2.52f));
  sc->add(place(rbox(3.6f, 2.2f, 0.1f, mat(0x8fb4c8, {.rough = 0.3f, .emissive = 0xffd59a, .ei = 0}), 0.02f), 2.2f, 1.4f, -2.52f));
  Sign* toLet = sign({.w = 1.6f, .h = 0.45f, .text = "TO LET", .fg = 0x6b6152, .bg = 0xefe6d4});
  toLet->mesh->position = {-2.2f, 2.85f, -2.56f};
  toLet->mesh->rotation.y = PI;
  sc->add(toLet->mesh);
  Sign* cafeSign = sign({.w = 1.8f, .h = 0.45f, .text = "PRINT · COPY", .fg = 0x2f3a44, .bg = 0xefe6d4});
  cafeSign->mesh->position = {2.2f, 2.85f, -2.56f};
  cafeSign->mesh->rotation.y = PI;
  sc->add(cafeSign->mesh);
  campus->add(sc);

  // Trees and lamp posts, placed deterministically away from buildings and paths.
  Rng r(11);
  const std::vector<std::array<float, 4>> blocked = {
      {-10, 10, -21, -9.5f}, {-27, -18, -8, 14},       {-14, 18, 20, 26.5f}, {19.5f, 28, -2.5f, 8.5f}, {4, 14, 3, 9.5f},
      {12.5f, 22, -28.5f, -16}, {-1.6f, 1.6f, -11, 20}, {-19, 21, 0.6f, 3.4f}, {-10, 14, 17.8f, 20.2f}, {3.8f, 16, -11.5f, -9},
      {13.6f, 15.6f, -14, -21}, {-19, -16.5f, -1, 8.5f},
  };
  auto freeAt = [&](double x, double z) {
    for (const auto& b : blocked)
      if (x > b[0] - 1 && x < b[1] + 1 && z > b[2] - 1 && z < b[3] + 1) return false;
    return true;
  };
  int n = 0, guard = 0;
  while (n < 46 && guard++ < 2000) {
    double x = -31 + (31 - -31) * r();
    double z = -34 + (29 - -34) * r();
    if (!freeAt(x, z)) continue;
    float s = 0.8f + (float)r() * 0.5f;
    Node* t = tree(r, s);
    float ry = (float)r() * 6;
    campus->add(place(t, (float)x, 0, (float)z, ry));
    n++;
  }
  for (auto [x, z] : std::vector<std::pair<float, float>>{{1.6f, 14}, {1.6f, 7}, {-1.6f, -4}, {6, 0.6f}, {-8, 0.6f}, {13, 3.4f}, {1.6f, -8.8f}, {13.6f, -14.6f}, {10.2f, -12.2f}}) {
    LampPost lp = lampPost();
    lp.root->position = {x, 0, z};
    W.lamps.push_back(lp.bulb->material);
    campus->add(lp.root);
  }

  // Benches on the quad.
  for (auto [x, z, ry] : std::vector<std::tuple<float, float, float>>{{-6, -1, 0}, {-3.5f, 5, PI}, {6, -4, PI / 2}}) {
    Node* b = group();
    b->add(place(rbox(1.4f, 0.08f, 0.45f, PAL::wood, 0.02f), 0, 0.42f, 0));
    b->add(place(rbox(1.4f, 0.35f, 0.06f, PAL::wood, 0.02f), 0, 0.65f, -0.2f));
    b->add(place(rbox(0.08f, 0.42f, 0.4f, PAL::woodDark, 0.02f), -0.6f, 0.21f, 0));
    b->add(place(rbox(0.08f, 0.42f, 0.4f, PAL::woodDark, 0.02f), 0.6f, 0.21f, 0));
    campus->add(place(b, x, 0, z, ry));
  }
}

// ---------------- the shop ----------------
void buildShop(World& W) {
  Node* g = group();
  g->position = P::shop;
  Material* wall = matS(PAL::cream, 0.9f);
  g->add(place(rbox(8, 0.12f, 5.5f, matS(0xc8a97e, 0.8f), 0.03f), 0, 0.06f, 0));
  g->add(place(rbox(8, 3.2f, 0.25f, wall, 0.05f), 0, 1.6f, -2.62f));
  g->add(place(rbox(0.25f, 3.2f, 5.5f, wall, 0.05f), -3.88f, 1.6f, 0));
  g->add(place(rbox(0.25f, 3.2f, 5.5f, wall, 0.05f), 3.88f, 1.6f, 0));
  // Front: door on the left, a long window to its right.
  g->add(place(rbox(5.7f, 0.55f, 0.25f, wall, 0.04f), 1.05f, 0.28f, 2.62f));
  for (float x : {-3.3f, -1.8f, 3.88f}) g->add(place(rbox(0.3f, 3.2f, 0.3f, wall, 0.05f), x, 1.6f, 2.62f));
  g->add(place(rbox(8.2f, 0.7f, 0.32f, wall, 0.05f), 0, 2.85f, 2.62f));
  g->add(place(rbox(8.7f, 0.3f, 6.2f, matS(PAL::roof, 0.8f), 0.08f), 0, 3.35f, 0));
  Material* glass = standard({.color = PAL::glass, .roughness = 0.12f, .metalness = 0, .emissive = 0xffc77a, .emissiveIntensity = 0,
                              .opacity = 0.22f, .transparent = true, .depthWrite = false});
  Mesh* pane = eng::mesh(geo::plane(5.4f, 1.95f), glass);
  pane->position = {1.05f, 1.53f, 2.64f};
  g->add(pane);

  Sign* shopSign = sign({.w = 6.2f, .h = 0.55f, .text = "SECOND-HAND BOOKS", .fg = 0xa39a8a, .bg = 0xe3d8c3});
  shopSign->mesh->position = {0.3f, 2.85f, 2.8f};
  g->add(shopSign->mesh);

  Sign* doorSign = sign({.w = 0.62f, .h = 0.42f, .text = "OPEN\n9–5", .px = 160, .fg = 0x2f3a44, .bg = 0xfbf6ea, .size = 0.7f});
  doorSign->mesh->position = {-1.8f, 1.55f, 2.79f};
  g->add(doorSign->mesh);

  // Interior.
  g->add(place(shelf(2.2f, 2.5f, 0.5f, {.seed = 3, .rows = 5}), -2.4f, 0.12f, -2.2f));
  g->add(place(shelf(2.2f, 2.5f, 0.5f, {.seed = 4, .rows = 5}), 0.0f, 0.12f, -2.2f));
  g->add(place(shelf(2.2f, 2.5f, 0.5f, {.seed = 5, .rows = 5}), 2.4f, 0.12f, -2.2f));
  Node* desk = counter(2.2f);
  desk->position = {2.2f, 0.12f, 0.2f};
  g->add(desk);
  Mesh* till = rbox(0.4f, 0.26f, 0.3f, 0x3a4250, 0.04f);
  till->position = {2.7f, 1.2f, 0.2f};
  g->add(till);
  g->add(place(shelf(1.4f, 1.0f, 0.45f, {.seed = 6, .rows = 2}), -0.6f, 0.12f, 0.9f, 0));

  PointLight* lamp = pointLight(0xffd6a0, 0, 9, 1.6f);
  lamp->position = {0.5f, 2.7f, 0.2f};
  g->add(lamp);

  W.scene.add(g);
  W.shop = {g, shopSign, doorSign, glass, lamp};
}

void updateShop(World& W, float t) {
  std::string door = "OPEN\n9–5";
  float stSign = 0, stLit = 0.5f;
  float litFrom = 0.5f, litAt = -1, signFrom = 0, signAt = -1;
  for (const ShopKey& k : W.shopKeys) {
    if (k.t > t) break;
    if (k.door) door = *k.door;
    if (k.lit) { litFrom = stLit; stLit = *k.lit; litAt = k.t; }
    if (k.sign) { signFrom = stSign; stSign = *k.sign; signAt = k.t; }
  }
  float lit = litAt < 0 ? stLit : lerp(litFrom, stLit, smooth((t - litAt) / 0.8f));
  float sg = signAt < 0 ? stSign : lerp(signFrom, stSign, smooth((t - signAt) / 1.2f));
  auto& S = W.shop;
  S.doorSign->set(door);
  S.lamp->intensity = lit * 7;
  S.glass->emissiveIntensity = lit * (0.15f + 0.6f * W.night);
  float q = std::round(sg * 4) / 4;
  if (q < 0.5f) S.sign->set(std::nullopt, 0xa39a8a, 0xe3d8c3);
  else S.sign->set(std::nullopt, 0xfff6e0, q < 1 ? 0x3f7a64 : 0x2f6b55);
}

// ---------------- crowd ----------------
const std::vector<std::vector<std::pair<float, float>>> ROUTES = {
    {{-7, 20.4f}, {-7, 19}, {0, 19}, {0, 2}, {-17.5f, 2}, {-17.5f, 0}, {-18.6f, 0}},
    {{11, 20.4f}, {11, 19}, {0, 19}, {0, -10.6f}},
    {{-18.6f, 7}, {-17.5f, 7}, {-17.5f, 2}, {19.9f, 2}},
    {{0, -10.6f}, {0, 2}, {19.9f, 2}},
    {{-7, 20.4f}, {-7, 19}, {0, 19}, {0, 2}, {19.9f, 2}},
    {{-18.6f, 0}, {-17.5f, 0}, {-17.5f, 2}, {0, 2}, {0, -10.6f}},
};

void buildCrowd(World& W) {
  const int N = 52;
  Material* bodyMat = standard({.roughness = 0.8f});
  Material* headMat = standard({.roughness = 0.7f});
  auto& C = W.crowd;
  C.bodies = instanced(FIG::bodyGeo(), bodyMat, N);
  C.heads = instanced(FIG::headGeo(), headMat, N);
  C.bodies->castShadow = C.heads->castShadow = true;
  C.bodies->frustumCulled = C.heads->frustumCulled = false;
  for (const auto& r : ROUTES) C.curves.push_back(&polyPath(r));
  for (const Curve* c : C.curves) C.lens.push_back(c->length());
  Rng r(5);
  for (int i = 0; i < N; i++) {
    World::Walker w;
    w.route = i % (int)ROUTES.size();
    w.phase = (float)r();
    w.speed = 1.05f + (float)r() * 0.45f;
    w.side = ((float)r() - 0.5f) * 1.3f;
    w.gate = (float)r();
    w.bob = (float)r() * 10;
    C.walkers.push_back(w);
    C.bodies->setColor(i, Color::hex(pick(r, PAL::shirts)));
    C.heads->setColor(i, Color::hex(pick(r, PAL::skin)));
  }
  W.scene.add(C.bodies, C.heads);
}

float crowdClock(const World& W, float t) {
  float extra = 0;
  for (auto [a, b, k] : W.warps)
    if (t > a) extra += (std::min(t, b) - a) * (k - 1);
  return t + extra;
}

float levelAt(const std::vector<CrowdKey>& keys, float t) {
  if (keys.empty()) return 1;
  float cur = keys[0].v;
  for (size_t i = 1; i < keys.size(); i++) {
    const CrowdKey& k = keys[i];
    if (t >= k.t) { cur = k.v; continue; }
    if (t > k.t - k.d) return lerp(cur, k.v, smooth((t - (k.t - k.d)) / k.d));
    break;
  }
  return cur;
}

void updateCrowd(World& W, float t) {
  auto& C = W.crowd;
  float tau = crowdClock(W, t);
  float level = levelAt(W.crowdKeys, t);
  for (size_t i = 0; i < C.walkers.size(); i++) {
    const World::Walker& w = C.walkers[i];
    const Curve& curve = *C.curves[w.route];
    float u = tau * w.speed / C.lens[w.route] + w.phase;
    float k = std::floor(u), f = u - k;
    bool fwd = (long long)k % 2 == 0;
    float at = fwd ? f : 1 - f;
    vec3 tv = curve.pointAt(at);
    vec3 nv = curve.tangentAt(at);
    if (!fwd) nv = -nv;
    float yaw = std::atan2(nv.x, nv.z);
    // Keep to one side of the path, like people do.
    tv.x += nv.z * w.side * 0.5f;
    tv.z -= nv.x * w.side * 0.5f;
    float s = smooth(f / 0.025f) * smooth((1 - f) / 0.025f);
    s *= clamp((level - w.gate) * 8 + 0.5f);
    float bob = std::fabs(std::sin(tau * 7 + w.bob)) * 0.05f;
    float sc = std::max(s, 0.0001f);
    C.bodies->matrices[i] = mat4::compose({tv.x, FIG::bodyY + bob, tv.z}, quat::euler({0, yaw, std::sin(tau * 7 + w.bob) * 0.05f}), {sc, sc, sc});
    C.heads->matrices[i] = mat4::compose({tv.x, FIG::headY + bob, tv.z}, quat::euler({0, yaw, 0}), {sc, sc, sc});
  }
}

// ---------------- Jia ----------------
void buildJia(World& W) {
  W.jia = figure({.shirt = 0xe9e2d0, .skin = 0xe8c4a4, .apron = true, .notebook = true, .hair = 0x2a2220});
  W.scene.add(W.jia.root);
}

float angleLerp(float a, float b, float f) {
  float tau = PI * 2;
  float d = std::fmod(std::fmod(b - a + PI, tau) + tau, tau) - PI;
  return a + d * f;
}

void updateJia(World& W, float t) {
  const auto& keys = W.jiaKeys;
  Figure& jia = W.jia;
  if (keys.empty()) return;
  size_t i = 0;
  while (i + 1 < keys.size() && t >= keys[i + 1].t) i++;
  const JiaKey& k = keys[i];
  const JiaKey* n = i + 1 < keys.size() ? &keys[i + 1] : nullptr;
  if (k.hide) { jia->visible = false; return; }
  jia->visible = true;
  bool moving = n && !n->hide && (n->x != k.x || n->z != k.z) && n->walk;
  float x = k.x, z = k.z, y = k.y, yaw = k.ry, walking = 0, phase = 0;
  if (moving) {
    // Walk from k to n, starting so that we arrive exactly at n.t.
    float dist = std::hypot(n->x - k.x, n->z - k.z);
    float start = std::max(k.t, n->t - (n->dur >= 0 ? n->dur : std::max(0.8f, dist / 1.5f)));
    float dur = n->t - start;
    if (t > start) {
      float f = clamp((t - start) / dur);
      x = lerp(k.x, n->x, f);
      z = lerp(k.z, n->z, f);
      y = lerp(k.y, n->y, f);
      float moveYaw = std::atan2(n->x - k.x, n->z - k.z);
      yaw = angleLerp(k.ry, moveYaw, clamp((t - start) / 0.35f));
      walking = std::min({1.f, f * 8, (1 - f) * 8});
      phase = f * dist * 4.2f;
    }
  } else if (i > 0) {
    // Just arrived: turn toward this key's facing.
    const JiaKey& p = keys[i - 1];
    if (!p.hide && (p.x != k.x || p.z != k.z)) {
      float moveYaw = std::atan2(k.x - p.x, k.z - p.z);
      yaw = angleLerp(moveYaw, k.ry, clamp((t - k.t) / 0.5f));
    }
  }
  jia->position = {x, y, z};
  jia->rotation.y = yaw;
  pose(jia, {.walking = walking, .phase = phase, .look = k.look, .nod = k.nod});
}

}  // namespace

World* createWorld() {
  World* Wp = new World();
  World& W = *Wp;
  Scene& s = W.scene;
  s.fog.enabled = true;
  s.fog.color = Color::hex(0x51647a);
  s.fog.near = 110;
  s.fog.far = 300;
  s.sky.radius = 400;
  W.camera.fov = 40;
  W.camera.aspect = 16.f / 9;
  W.camera.near = 0.1f;
  W.camera.far = 600;

  // Stars for the night scenes (deterministic positions).
  {
    Rng r(77);
    std::vector<vec3> a;
    for (int i = 0; i < 700; i++) {
      double th = r() * PI * 2;
      double ph = std::acos(lerp(0.08f, 1.f, (float)r()));
      const double R = 380;
      a.push_back({(float)(R * std::sin(ph) * std::cos(th)), (float)(R * std::cos(ph)), (float)(R * std::sin(ph) * std::sin(th))});
    }
    W.starMat = basic({.color = 0xdfe6ff, .opacity = 0, .transparent = true, .depthWrite = false, .fog = false});
    W.starMat->pointSize = 1.6f;
    s.add(pointCloud(geo::points(a), W.starMat));
  }

  s.hemi = {Color::hex(0xd6e4ff), Color::hex(0x6e5c48), 1.1f};
  s.sun.color = Color::hex(0xfff0dc);
  s.sun.intensity = 2.6f;
  s.sun.castShadow = true;
  s.sun.shadow.mapSize = 2048;
  s.sun.shadow.left = -44; s.sun.shadow.right = 44; s.sun.shadow.top = 44; s.sun.shadow.bottom = -44;
  s.sun.shadow.near = 1; s.sun.shadow.far = 220;
  s.sun.shadow.bias = -0.0004f;
  s.sun.shadow.normalBias = 0.03f;
  s.sun.shadow.radius = 3;
  s.sun.target = {0, 0, -2};
  s.fill.color = Color::hex(0x9fb8ff);
  s.fill.intensity = 0.35f;
  s.fill.position = {40, 30, 60};
  s.fill.target = {0, 0, 0};

  // Bloom for lit windows and glowing threads, then the grade.
  W.post.bloomStrength = 0.32f;
  W.post.bloomRadius = 0.55f;
  W.post.bloomThreshold = 0.86f;
  W.post.vignette = 0.55f;

  buildCampus(W);
  buildShop(W);
  buildCrowd(W);
  buildJia(W);
  return Wp;
}

void World::finalize() {
  auto byT = [](const auto& a, const auto& b) { return a.t < b.t; };
  std::stable_sort(camKeys.begin(), camKeys.end(), byT);
  std::stable_sort(jiaKeys.begin(), jiaKeys.end(), byT);
  std::stable_sort(lightKeys.begin(), lightKeys.end(), byT);
  std::stable_sort(crowdKeys.begin(), crowdKeys.end(), byT);
  std::stable_sort(shopKeys.begin(), shopKeys.end(), byT);
}

void World::resize(int w, int h) { camera.aspect = (float)w / h; }

void World::update(float t) {
  fx = Fx{};
  updateLight(*this, t);
  updateCamera(*this, t);
  updateCrowd(*this, t);
  updateJia(*this, t);
  updateShop(*this, t);
  for (auto& f : updaters) f(t);
}

World::ScreenPos World::toScreen(const vec3& p) const {
  vec3 n = camera.project(p);
  return {(n.x + 1) / 2, (1 - n.y) / 2, n.z > 1};
}

void World::focus(const vec3& p, float r, float amt) {
  ScreenPos s = toScreen(p);
  post.focusCenter = {s.x, s.y};
  post.focusRadius = r;
  post.focusAmount = amt;
}

void World::applyFx() {
  post.saturation = fx.sat;
  post.fade = fx.fade;
  post.flash = fx.flash;
  if (fx.focus && fx.focusAmt > 0.001f) focus(*fx.focus, fx.focusR, fx.focusAmt);
  else post.focusAmount = 0;
}

}  // namespace film
