// Chapter 4 · The Pull Inside the Problem
// The two sides of the shop's contradiction, the smaller problems that hang on it, and the outside condition.
#include "chapters.h"

namespace film {

namespace {

const vec3 UP{0, 1, 0};

/** A straight rod between two points, re-aimed every frame. */
struct Rod {
  Mesh* m;
  void span(const vec3& a, const vec3& b, float grow = 1) const {
    vec3 dir = b - a;
    float len = dir.length() * grow;
    m->visible = len > 0.01f;
    if (!m->visible) return;
    dir = dir.normalized();
    m->position = a + dir * (len / 2);
    m->setQuaternion(quat::fromTo(UP, dir));
    m->scale = {1, len, 1};
  }
};
Rod rod(float radius, Material* material) { return {eng::mesh(geo::cylinder(radius, radius, 1, 10, 1, true), material)}; }

struct Spark {
  Mesh* m;
  const Curve* curve;
  float t0;
};

// A short glow on the cables each time an outside arrow arrives.
float pulseAt(float t, const std::vector<Spark>& sparks) {
  float p = 0;
  for (const Spark& s : sparks) {
    float d = t - (s.t0 + 3.0f);
    if (d > 0 && d < 0.5f) p = std::max(p, 1 - d / 0.5f);
  }
  return p;
}

}  // namespace

Update buildC4(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[3];

  W.addLight({{T("4a", 1.5f), "analysis", 3}});
  W.addCrowd({{T("4a", 1), 0.3f, 2}});
  W.addShop({{.t = T("4a"), .door = "OPEN\n9–5", .lit = 0.5f}, {.t = T("4g", 0.6f), .sign = 1}});

  const CamKey main{.t = 0, .d = 3.0f, .p = {17, 8.8f, -3.0f}, .l = {17, 2.4f, -24}, .fov = 42};
  auto withT = [](CamKey k, float t, float d) { k.t = t; k.d = d; return k; };
  W.addCam({
      withT(main, T("4a", 0.8f), 3.0f),
      {.t = T("4f", 0.8f), .d = 2.2f, .p = {19.8f, 6.0f, -10.4f}, .l = {17.6f, 1.9f, -22.2f}, .fov = 41},
      withT(main, T("4h", 0.4f), 2.2f),
      {.t = T("4i", 0.8f), .d = 3.2f, .p = {36, 40, 30}, .l = {6, 0, -10}},
      {.t = T("4j", 4.6f), .d = 2.8f, .p = {26, 15, 3}, .l = {15, 1.2f, -20}},
  });

  W.addJia({
      {.t = T("4a", 0.2f), .x = 15.2f, .z = -19.6f, .y = 0, .ry = 0.35f},
      {.t = T("4a", 1.6f), .x = 16.9f, .z = -19.4f, .y = 0, .ry = PI},
  });

  // ---- the two sides ----
  const uint32_t BLUE = 0x7aa2ff, VIOLET = 0xd08cff, GOLD = 0xffcf5a;
  auto pillar = [&](uint32_t color) {
    Node* g = group();
    g->add(rbox(0.9f, 4.2f, 0.9f, mat(0x2c3446, {.rough = 0.5f}), 0.2f)->translateY(2.1f));
    Mesh* cap = eng::mesh(sphereGeo(0.62f, 24, 16), standard({.color = color, .roughness = 0.4f, .emissive = color, .emissiveIntensity = 0.55f}));
    cap->position.y = 4.55f;
    g->add(cap);
    W.scene.add(g);
    return g;
  };
  Node* left = pillar(BLUE);
  Node* right = pillar(VIOLET);
  auto cableMat = [](uint32_t color) { return standard({.color = color, .roughness = 0.4f, .emissive = color, .emissiveIntensity = 0.9f}); };
  Rod cableL = rod(0.1f, cableMat(BLUE)), cableR = rod(0.1f, cableMat(VIOLET));
  W.scene.add(cableL.m, cableR.m);
  const vec3 anchorL{12.9f, 1.9f, -24.2f}, anchorR{21.1f, 1.9f, -24.2f};

  // The arch: each side depends on the other.
  Node* archG = group();
  archG->position.x = 17;
  GrowTube archTube = growTube(arc({-8.4f, 4.6f, -25.5f}, {8.4f, 4.6f, -25.5f}, 4.4f), 0.07f, glow(0xf4efe6, 0.85f), 80, 8);
  archG->add(archTube.mesh);
  W.scene.add(archG);

  // ---- smaller problems ----
  Node* laptop = group();
  laptop->add(rbox(1.0f, 0.06f, 0.7f, mat(0x3a4250), 0.02f));
  Sign* screen = sign({.w = 0.95f, .h = 0.62f, .text = "www\n2009", .px = 140, .fg = 0x7d8794, .bg = 0xe8e4da});
  screen->mesh->position = {0, 0.34f, -0.33f};
  screen->mesh->rotation.x = -0.2f;
  laptop->add(screen->mesh);
  laptop->position = {22.6f, 2.3f, -20.4f};
  laptop->rotation.y = -0.35f;
  W.scene.add(laptop);
  struct Small { vec3 at; const char* en; const char* zh; char toCable; float f; Rod m; Mesh* dot; };
  std::vector<Small> small = {
      {{16.0f, 3.05f, -21.95f}, "Faded sign", "招牌褪色", 'L', 0.55f, {}, nullptr},
      {{22.6f, 2.75f, -20.4f}, "Old website", "网站过时", 'R', 0.6f, {}, nullptr},
      {{19.8f, 1.0f, -21.95f}, "Prices", "价格", 'R', 0.3f, {}, nullptr},
  };
  for (Small& s : small) {
    s.m = rod(0.035f, basic({.color = 0xff9b8c, .opacity = 0.9f, .transparent = true}));
    W.scene.add(s.m.m);
    s.dot = eng::mesh(sphereGeo(0.11f, 12, 8), glow(0xff7a6b));
    s.dot->position = s.at;
    W.scene.add(s.dot);
  }
  const vec3 shopHook{17, 2.2f, -23.0f};

  // People drawn by the brighter sign, who look in and leave.
  Rng r(52);
  struct Looker { Figure f; float t0, side; };
  std::vector<Looker> lookers;
  for (int i = 0; i < 3; i++) {
    Figure f = figure({.shirt = pick(r, PAL::shirts), .skin = pick(r, PAL::skin), .hair = pick(r, std::vector<uint32_t>{0x2a2220, 0x4a3426})});
    W.scene.add(f.root);
    lookers.push_back({f, T("4g", 1.2f + i * 0.9f), (i - 1) * 0.55f});
  }

  // ---- the outside condition ----
  Node* outer = group();
  Mesh* ringM = eng::mesh(geo::torus(41, 0.22f, 8, 160), glow(0xcfd6e0, 0.55f));
  ringM->rotation.x = -PI / 2;
  outer->add(ringM);
  struct Arrow { Mesh* m; float a; };
  std::vector<Arrow> arrows;
  Geometry* coneGeo = geo::cone(0.55f, 1.6f, 12);
  for (int i = 0; i < 18; i++) {
    float a = (i / 18.f) * PI * 2;
    Mesh* cone = eng::mesh(coneGeo, glow(0xe8edf4, 0.85f));
    outer->add(cone);
    arrows.push_back({cone, a});
  }
  outer->position = {0, 0.8f, -2};
  W.scene.add(outer);
  std::vector<Spark> sparks;
  for (int i = 0; i < 18; i++) {
    Mesh* s = eng::mesh(sphereGeo(0.28f, 12, 8), glow(0xe8edf4, 1));
    float a = (i / 18.f) * PI * 2 + 0.17f;
    vec3 from{std::cos(a) * 40, 1.6f, -2 + std::sin(a) * 40};
    W.scene.add(s);
    sparks.push_back({s, &arc(from, {17, 2.2f, -24}, 10), T("4j", 0.6f + (i % 6) * 0.45f)});
  }

  // ---- labels ----
  L.add({.kind = Kind::Card, .screen = vec2{0.5f, 0}, .anchor = vec2{0.5f, 0}, .en = "What pulls the shop two ways?", .zh = "是什么在把书店往两个方向拉？",
         .vis = [=](float t) { return win(t, T("4a", 2.2f), c("4b").s + 0.6f); }});
  L.add({.kind = Kind::Shop, .at = left, .lift = 5.4f, .en = "What the shop is: a daytime counter for buying and selling", .zh = "书店现在：白天营业、只管买卖的柜台",
         .kicker = Pair{"One side", "一方面"}, .vis = [=](float t) { return win(t, T("4b", 1.0f), c("4f").s); }});
  L.add({.kind = Kind::Need, .at = right, .lift = 5.4f, .en = "What students now need: cheap materials, evening hours, a place to work together",
         .zh = "学生现在需要：便宜的资料、晚上的时间、一起学习的地方", .kicker = Pair{"Other side", "另一方面"},
         .vis = [=](float t) { return win(t, T("4c", 1.0f), c("4f").s); }});
  L.add({.kind = Kind::Tag, .at = vec3{17, 9.2f, -25.5f}, .en = "Each side depends on the other", .zh = "双方相互依存",
         .vis = [=](float t) { return win(t, T("4d", 2.0f), c("4e").s + 0.4f); }});
  L.add({.kind = Kind::Principle, .at = vec3{17, 9.2f, -25.5f}, .en = "Contradiction: a unity of opposites", .zh = "矛盾：对立面的统一",
         .vis = [=](float t) { return win(t, T("4e", 2.4f), c("4f").s); }});
  for (size_t i = 0; i < small.size(); i++)
    L.add({.kind = Kind::Flaw, .cls = "small", .at = small[i].at, .lift = 0.25f, .en = small[i].en, .zh = small[i].zh,
           .vis = [=](float t) { return win(t, T("4f", 2.4f + i * 1.1f), c("4i").s); }});
  L.add({.kind = Kind::Flaw, .cls = "small", .at = vec3{14.4f, 1.6f, -18.8f}, .anchor = vec2{1, 1}, .offset = {-6, 0},
         .en = "They arrive, look in, and leave", .zh = "来了，看一眼，又走了", .vis = [=](float t) { return win(t, T("4g", 4.2f), c("4h").s + 0.4f); }});
  L.add({.kind = Kind::Principle, .at = vec3{17, 9.2f, -25.5f}, .en = "The principal contradiction: under these conditions, it shapes the others",
         .zh = "主要矛盾：在这些条件下，它制约着其他问题", .vis = [=](float t) { return win(t, T("4h", 2.2f), c("4i").s + 0.3f); }});
  L.add({.kind = Kind::Tag, .at = vec3{41, 1.2f, -2}, .en = "External condition: readings move online", .zh = "外部条件：资料转到网上",
         .kicker = Pair{"Outside", "外部"}, .vis = [=](float t) { return win(t, T("4i", 3.0f), ch.e); }});
  L.add({.kind = Kind::Card, .cls = "gold", .screen = vec2{0.5f, 0}, .anchor = vec2{0.5f, 0}, .en = "External causes work through internal causes",
         .zh = "外因通过内因起作用", .vis = [=](float t) { return win(t, T("4j", 0.8f), ch.e + 0.4f); }});
  L.add({.kind = Kind::Principle, .at = shopHook, .lift = 2.4f, .en = "Internal cause: how the shop meets its own contradiction",
         .zh = "内因：书店如何解决自身的矛盾", .vis = [=](float t) { return win(t, T("4j", 5.6f), ch.e + 0.4f); }});

  return [=](float t) {
    bool live = t > ch.s && t < ch.e + 1.2f;
    float out = 1 - ramp(t, ch.e - 0.8f, ch.e + 0.8f);

    // Pillars rise, then drift apart under tension.
    float apart = ramp(t, T("4e", 0.2f), T("4e", 3.0f)) * 1.6f;
    float riseL = settle(ramp(t, T("4b", 0.2f), T("4b", 1.2f), linear)) * out;
    float riseR = settle(ramp(t, T("4c", 0.2f), T("4c", 1.2f), linear)) * out;
    left->visible = live && riseL > 0.01f && out > 0.02f;
    right->visible = live && riseR > 0.01f && out > 0.02f;
    left->position = {8.6f - apart, 0, -25.5f};
    right->position = {25.4f + apart, 0, -25.5f};
    left->scale = {out, std::max(riseL, 0.01f), out};
    right->scale = {out, std::max(riseR, 0.01f), out};
    vec3 pL{left->position.x, 3.1f, -25.5f}, pR{right->position.x, 3.1f, -25.5f};

    // Cables: blue and violet, turning gold once named as the principal contradiction.
    float gold = ramp(t, T("4h", 0.6f), T("4h", 2.0f));
    float strain = 1 + std::sin(t * 9) * 0.12f * ramp(t, T("4e", 0.2f), T("4e", 1.2f)) * (1 - gold);
    struct C { const Rod* m; vec3 a, b; uint32_t col; float t0; };
    for (const C& cb : {C{&cableL, anchorL, pL, BLUE, T("4b", 0.8f)}, C{&cableR, anchorR, pR, VIOLET, T("4c", 0.8f)}}) {
      cb.m->span(cb.a, cb.b, ramp(t, cb.t0, cb.t0 + 1.0f) * out);
      if (!live) cb.m->m->visible = false;
      cb.m->m->scale.x = cb.m->m->scale.z = strain * (1 + gold * 0.5f);
      Material* mm = cb.m->m->material;
      mm->color = Color::hex(cb.col).lerp(Color::hex(GOLD), gold);
      mm->emissive = mm->color;
      mm->emissiveIntensity = 0.9f + gold * 0.8f + pulseAt(t, sparks) * 1.5f;
    }

    archG->scale.x = (8.4f + apart) / 8.4f;
    archTube.grow(live ? ramp(t, T("4d", 0.4f), T("4d", 2.0f)) * out : 0);

    // Small problems: threads to the shop, later re-hung on the main cables.
    float reroute = ramp(t, T("4h", 1.2f), T("4h", 3.0f));
    fade(laptop, win(t, T("4f", 2.6f), c("4i").s + 0.6f));
    laptop->position.y = 2.3f + std::sin(t * 1.4f) * 0.06f;
    for (size_t i = 0; i < small.size(); i++) {
      const Small& s = small[i];
      float on = win(t, T("4f", 2.2f + i * 1.1f), c("4i").s + 0.6f);
      s.m.m->material->opacity = 0.9f * on;
      s.dot->visible = on > 0.01f;
      if (!s.dot->visible) { s.m.m->visible = false; continue; }
      vec3 tmp = lerp(s.toCable == 'L' ? anchorL : anchorR, s.toCable == 'L' ? pL : pR, s.f);
      tmp = lerp(shopHook, tmp, reroute);
      s.m.span(s.at, tmp, ramp(t, T("4f", 2.2f + i * 1.1f), T("4f", 3.0f + i * 1.1f)));
    }

    for (const Looker& lk : lookers) {
      float go = ramp(t, lk.t0, lk.t0 + 3.2f, linear);
      float back = ramp(t, lk.t0 + 4.4f, lk.t0 + 7.2f, linear);
      float u = go - back;
      lk.f->visible = t > lk.t0 && t < lk.t0 + 7.2f;
      if (!lk.f->visible) continue;
      float z = lerp(-13.4f, -20.4f, smooth(u));
      lk.f->position = {14.6f + lk.side, 0, z};
      lk.f->rotation.y = t < lk.t0 + 4.4f ? PI : 0;
      float walking = (t < lk.t0 + 3.2f || t > lk.t0 + 4.4f) ? 1.f : 0.f;
      pose(lk.f, {.walking = walking, .phase = t * 7, .look = walking > 0 ? 0 : std::sin((t - lk.t0) * 3) * 0.5f});
    }

    // The outside condition and the arrows that travel in to the shop.
    float ringOn = win(t, T("4i", 1.2f), ch.e - 0.2f, 1.2f, 1.0f);
    outer->visible = ringOn > 0.01f;
    ringM->material->opacity = 0.55f * ringOn;
    for (size_t i = 0; i < arrows.size(); i++) {
      const Arrow& a = arrows[i];
      float pulse = (std::sin(t * 2 - i) + 1) * 0.5f;
      float rr = 41 - 2.6f - pulse * 1.2f;
      a.m->position = {std::cos(a.a) * rr, 0.4f, std::sin(a.a) * rr};
      a.m->rotation = {0, 0, 0};
      a.m->lookAt(outer->position);
      a.m->rotateX(PI / 2);
      a.m->material->opacity = 0.85f * ringOn;
      float k = backOut(ramp(t, T("4i", 1.4f + i * 0.05f), T("4i", 2.2f + i * 0.05f), linear));
      a.m->scale.setScalar(k != 0 ? k : 0.0001f);
    }
    for (const Spark& s : sparks) {
      float u = (t - s.t0) / 3.0f;
      s.m->visible = u > 0 && u < 1 && t < ch.e;
      if (!s.m->visible) continue;
      s.m->position = s.curve->point(smooth(u));
      scaleIn(s.m, std::min({1.f, u * 5, (1 - u) * 6}));
    }
  };
}

}  // namespace film
