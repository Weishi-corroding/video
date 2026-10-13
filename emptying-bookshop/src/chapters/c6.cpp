// Chapter 6 · Back, but Higher
// The shop's history as a helix: swap shelf → shop → swap-and-study room. From above a circle, from the side a climb.
#include "chapters.h"

namespace film {

Update buildC6(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[5];

  W.addLight({{T("6a", 1.0f), "night", 3}});
  W.addCrowd({{T("6a", 1.0f), 0, 2}});

  // Helix geometry: one full turn from stage 1 to stage 3, continuing faintly beyond.
  const vec3 C{17, 0, -25};
  const float R = 5.2f, Y0 = 6.5f, Y1 = 18.5f, TH = 0.6f;
  auto at = [=](float s) {
    return vec3{C.x + std::sin(TH + s * PI * 2) * R, Y0 + (Y1 - Y0) * s, C.z + std::cos(TH + s * PI * 2) * R};
  };
  auto helixCurve = [&](float s0, float s1) -> const Curve& {
    return keepCurve(new FnCurve([=](float u) { return at(lerp(s0, s1, u)); }));
  };
  Material* tubeMat = standard({.color = 0xf3d58a, .roughness = 0.9f, .emissive = 0xffd98a, .emissiveIntensity = 0.55f,
                                 .opacity = 0.97f, .transparent = true, .side = Side::Double, .paper = 1});
  Geometry* tubeGeo = geo::ribbon(helixCurve(0, 1), 240, 0.62f);
  Mesh* tube = eng::mesh(tubeGeo, tubeMat);
  Mesh* ghostTube = eng::mesh(geo::tube(helixCurve(0, 1.22f), 260, 0.035f, 5, false),
                              basic({.color = 0xdfe2f0, .opacity = 0.25f, .transparent = true, .depthWrite = false}));
  Mesh* beyond = eng::mesh(geo::ribbon(helixCurve(1, 1.22f), 60, 0.5f),
                           standard({.color = 0xf3d58a, .roughness = 0.9f, .emissive = 0xffd98a, .emissiveIntensity = 0.4f, .opacity = 0.4f,
                                     .transparent = true, .depthWrite = false, .side = Side::Double, .paper = 1}));
  Node* helix = group();
  helix->add(tube, ghostTube, beyond);
  W.scene.add(helix);
  const int perSeg = 6;
  auto growTo = [=](float s) { tubeGeo->setDrawRange(0, (uint32_t)std::ceil(240 * clamp(s)) * perSeg); };

  // A column of dots from stage 1 to stage 3: same direction, different height.
  Node* column = group();
  std::vector<Mesh*> columnDots;
  for (int i = 1; i < 16; i++) {
    Mesh* d = eng::mesh(sphereGeo(0.09f, 10, 8), glow(0xffffff, 0.9f));
    d->position = at(0).withY(lerp(Y0, Y1, i / 16.f));
    column->add(d);
    columnDots.push_back(d);
  }
  W.scene.add(column);

  // ---- three stages ----
  Rng r(71);
  auto person = [&](bool s = false, FigOpts o = {}) {
    o.shirt = pick(r, PAL::shirts);
    o.skin = pick(r, PAL::skin);
    if (r() < 0.7) o.hair = pick(r, std::vector<uint32_t>{0x2a2220, 0x4a3426, 0x1d1d22});
    return (s ? seated(o) : figure(o)).root;
  };
  auto stage = [&](float s, const std::function<void(Node*)>& build) {
    Node* g = group();
    g->add(place(eng::mesh(cylinderGeo(2.4f, 2.3f, 0.3f, 48), matS(0xefe6d4, 0.8f)), 0, -0.15f, 0));
    Node* inner = group();
    build(inner);
    g->add(inner);
    g->position = at(s);
    g->rotation.y = TH;  // every stage faces the same way, toward the viewer's side
    W.scene.add(g);
    return g;
  };
  // Stage 1: a lounge, a messy swap shelf, books on the floor.
  std::vector<std::pair<Mesh*, vec3>> lost;
  Node* s1 = stage(0, [&](Node* g) {
    g->add(place(eng::mesh(cylinderGeo(1.5f, 1.5f, 0.02f, 40), matS(0xb5694f, 0.95f)), 0, 0.01f, 0.2f));
    g->add(place(sofa(0x7c8fa6), -0.5f, 0, 0.3f, 0));
    g->add(place(shelf(1.1f, 1.3f, 0.35f, {.seed = 19, .rows = 3, .messy = true}), 1.1f, 0, -0.9f, -0.5f));
    for (auto [x, z] : std::vector<std::pair<float, float>>{{-0.95f, 0.35f}, {-0.1f, 0.35f}}) g->add(place(person(true), x, 0, z));
    for (int i = 0; i < 5; i++) {
      Mesh* b = rbox(0.28f, 0.05f, 0.2f, matS(pick(r, std::vector<uint32_t>{0xb85c4a, 0x4f6f9a, 0xd9b45a, 0x6f9a7a}), 0.9f), 0.01f);
      float bx = 0.3f + (float)r() * 1.1f;
      float bz = 0.4f + (float)r() * 0.9f;
      b->position = {bx, 0.03f, bz};
      b->rotation.y = (float)r() * 3;
      g->add(b);
    }
    for (int i = 0; i < 3; i++) {
      Mesh* b = rbox(0.24f, 0.32f, 0.06f, matS(pick(r, std::vector<uint32_t>{0xb85c4a, 0x4f6f9a, 0xd9b45a}), 0.9f), 0.01f);
      b->material = b->material->clone();
      g->add(b);
      lost.push_back({b, {1.0f + i * 0.12f, 0.9f, -0.8f}});
    }
  });
  // Stage 2: a counter and neat shelves; one person serves, one buys.
  Node* s2 = stage(0.5f, [&](Node* g) {
    g->add(place(shelf(1.3f, 1.7f, 0.4f, {.seed = 23, .rows = 4}), -0.75f, 0, -1.2f));
    g->add(place(shelf(1.3f, 1.7f, 0.4f, {.seed = 24, .rows = 4}), 0.75f, 0, -1.2f));
    g->add(place(counter(1.6f), 0, 0, 0.1f));
    g->add(place(rbox(0.3f, 0.2f, 0.24f, 0x3a4250, 0.03f), 0.45f, 1.05f, 0.1f));
    g->add(place(figure({.shirt = 0xe9e2d0, .skin = 0xe8c4a4, .apron = true, .hair = 0x2a2220}).root, 0, 0, -0.55f));
    g->add(place(person(), 0.25f, 0, 1.0f, PI));
    Sign* tag = sign({.w = 0.9f, .h = 0.26f, .text = script::tr("OPEN 9–5", "营业 9–5"), .px = 100, .fg = 0x2f3a44, .bg = 0xfbf6ea});
    tag->mesh->position = {0, 1.6f, 0.46f};
    g->add(tag->mesh);
  });
  // Stage 3: swap shelf, neat shelf, a study table with people, and the counter kept small.
  Node* s3 = stage(1, [&](Node* g) {
    g->add(place(shelf(1.2f, 1.6f, 0.4f, {.seed = 29, .rows = 4}), -0.9f, 0, -1.25f));
    Node* sw = shelf(1.0f, 1.1f, 0.35f, {.seed = 31, .rows = 3});
    g->add(place(sw, 0.85f, 0, -1.25f));
    Sign* swS = sign({.w = 1.0f, .h = 0.24f, .text = script::tr("SWAP · STUDY", "换书 · 自习"), .px = 100, .fg = 0xfff7e6, .bg = 0x2f6b55});
    swS->mesh->position = {0.85f, 1.32f, -1.1f};
    g->add(swS->mesh);
    g->add(place(table(1.3f, 0.8f), 0, 0, 0.35f));
    for (auto [x, z, ry] : std::vector<std::tuple<float, float, float>>{{-0.45f, -0.25f, 0}, {0.45f, -0.25f, 0}, {-0.45f, 0.95f, PI}, {0.45f, 0.95f, PI}})
      g->add(place(person(true), x, 0, z, ry));
    g->add(place(rbox(0.8f, 0.7f, 0.45f, 0x9c7452, 0.04f), -1.55f, 0.35f, 0.45f));
    Mesh* lamp = eng::mesh(sphereGeo(0.16f, 12, 8), standard({.color = 0xfff1d6, .emissive = 0xffc77a, .emissiveIntensity = 2.5f}));
    lamp->position = {0, 1.7f, 0.35f};
    g->add(lamp);
  });
  const std::vector<Node*> stages{s1, s2, s3};

  W.addCam({
      {.t = T("6a", 0.8f), .d = 3.4f, .p = {42, 17, 8}, .l = {17, 11, -24}},
      {.t = T("6b", 0.6f), .d = 2.6f, .p = {30, 10.6f, -6.5f}, .l = {19.9f, 6.9f, -20.7f}},
      {.t = T("6c", 0.9f), .d = 2.8f, .p = {28.5f, 16.8f, -11}, .l = {14.1f, 12.8f, -29.3f}, .arc = 1.5f},
      {.t = T("6d", 0.7f), .d = 2.6f, .p = {31, 22.6f, -6.5f}, .l = {19.9f, 18.9f, -20.7f}},
      {.t = T("6e", 1.0f), .d = 2.6f, .p = {17.4f, 46, -24.4f}, .l = {17, 10, -25}, .fov = 42},
      {.t = T("6e", 7.4f), .d = 2.6f, .p = {43.4f, 12.6f, -43.1f}, .l = {17, 12.4f, -25}, .fov = 40},
      {.t = T("6f", 1.0f), .d = 3.0f, .p = {50, 17, 8}, .l = {17, 14.5f, -24}, .fov = 44},
  });

  // ---- labels ----
  auto lbl = [&](Node* g, Pair kicker, const char* en, const char* zh, float a, float b) {
    L.add({.kind = Kind::Tag, .at = g, .lift = 2.3f, .en = en, .zh = zh, .kicker = kicker, .vis = [=](float t) { return win(t, a, b); }});
  };
  lbl(s1, {"Stage 1", "阶段一"}, "Swap shelf in a dorm lounge: shared and friendly, but disorganised", "宿舍换书架：共享、友好，但杂乱无序", T("6b", 1.2f), c("6c").s + 0.6f);
  lbl(s2, {"Stage 2", "阶段二"}, "A proper shop: reliable and catalogued, but the gathering is lost", "正式书店：可靠、有目录，却失去了相聚", T("6c", 2.6f), c("6d").s + 0.6f);
  lbl(s3, {"Stage 3", "阶段三"}, "Swap-and-study room: sharing and gathering return, organisation stays", "换书自习空间：共享与相聚回来了，条理也保留了", T("6d", 1.6f), c("6e").s + 0.4f);
  L.add({.kind = Kind::Tag, .at = s3, .lift = 1.2f, .en = "From above: it looks like a return", .zh = "从上面看：像是回到了起点",
         .vis = [=](float t) { return win(t, T("6e", 2.6f), T("6e", 6.6f)); }});
  L.add({.kind = Kind::Tag, .at = at(0.75f), .lift = 1.0f, .en = "From the side: it climbs", .zh = "从侧面看：它在上升",
         .vis = [=](float t) { return win(t, T("6e", 8.0f), c("6f").s + 0.6f); }});
  for (auto [g, en, zh] : std::vector<std::tuple<Node*, const char*, const char*>>{
           {s1, "1 · shared, disorganised", "1 · 共享，但无序"}, {s2, "2 · organised, no gathering", "2 · 有序，却无相聚"}, {s3, "3 · shared and organised", "3 · 既共享，又有序"}})
    L.add({.kind = Kind::Tag, .cls = "small plain", .at = g, .lift = 1.9f, .en = en, .zh = zh, .vis = [=](float t) { return win(t, T("6f", 2.4f), ch.e + 0.4f); }});
  L.add({.kind = Kind::Card, .cls = "gold", .screen = vec2{0.5f, 0}, .anchor = vec2{0.5f, 0},
         .en = "Negation of the negation: each stage overcomes the last, and keeps what was valuable in it",
         .zh = "否定之否定：每个阶段克服前一阶段，又保留其中有价值的东西",
         .attr = Pair{"A way to read development, not a formula that predicts it.", "这是理解发展的一种方式，不是预测发展的公式。"},
         .vis = [=](float t) { return win(t, T("6f", 0.6f), ch.e + 0.6f); }});

  return [=](float t) {
    // The helix leaves at the start of chapter 7, sinking back into the shop.
    float gone = ramp(t, c("7a").s - 1.2f, c("7a").s + 1.2f);
    bool on = t > T("6a") - 0.2f && gone < 1;
    helix->visible = on;
    ghostTube->material->opacity = 0.25f * ramp(t, T("6a", 0.4f), T("6a", 2.0f)) * (1 - gone);
    tubeMat->opacity = 0.95f * (1 - gone);
    beyond->material->opacity = 0.45f * ramp(t, T("6e", 0.2f), T("6e", 1.4f)) * (1 - gone);
    float s = 0.02f + 0.48f * ramp(t, T("6c", 0.0f), T("6c", 2.2f)) + 0.5f * ramp(t, T("6d", 0.0f), T("6d", 2.0f));
    growTo(t > T("6b", 0.2f) ? s : 0);

    const char* cues[3] = {"6b", "6c", "6d"};
    for (int i = 0; i < 3; i++) {
      float k = backOut(ramp(t, T(cues[i], 0.2f), T(cues[i], 1.1f), linear));
      scaleIn(stages[i], on ? k * (1 - gone) : 0);
    }
    // Books go missing from the messy shelf.
    for (size_t i = 0; i < lost.size(); i++) {
      auto [b, home] = lost[i];
      float u = ramp(t, T("6b", 3.2f + i * 0.7f), T("6b", 4.6f + i * 0.7f));
      b->position = home + vec3{0.4f * u, 1.4f * u, 0.6f * u};
      b->rotation = {u * 2, u * 3, 0};
      fade(b, 1 - u);
    }

    float col = win(t, T("6e", 1.6f), ch.e + 0.4f, 1.0f, 0.8f) * (1 - gone);
    column->visible = col > 0.01f;
    for (size_t i = 0; i < columnDots.size(); i++) columnDots[i]->material->opacity = 0.9f * col * (0.5f + 0.5f * std::sin(t * 3 - i * 0.5f));
  };
}

}  // namespace film
