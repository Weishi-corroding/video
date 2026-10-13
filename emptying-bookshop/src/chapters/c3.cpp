// Chapter 3 · Go and Find Out
// Investigation: four findings (place, time, courses, study space), each tied back to the shop by a thread.
#include "chapters.h"

namespace film {

Update buildC3(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[2];

  W.addLight({{T("3d", 3.2f), "golden", 2.5f}, {T("3f", 1.8f), "dusk", 2.6f}});
  W.addCrowd({{T("3a"), 1.0f, 2}, {T("3f", 1), 0.45f, 2}});
  W.addShop({
      {.t = T("3b", 2.5f), .door = script::tr("OUT\nASKING", "外出\n调查中"), .lit = 0},
      {.t = T("3d", 2.0f), .lit = 0.6f},
      {.t = T("3d", 3.2f), .door = script::tr("CLOSED", "已打烊"), .lit = 0},
      {.t = T("3i"), .door = script::tr("OPEN\n9–5", "营业\n9–5")},
  });

  W.addCam({
      {.t = T("3a", 0.6f), .d = 3.0f, .p = {30, 30, 28}, .l = {5, 0, -9}},
      {.t = T("3b", 0.8f), .d = 2.6f, .p = {17, 9, 1}, .l = {6, 0.8f, -11.5f}},
      {.t = T("3b", 5.4f), .d = 3.2f, .p = {-1.2f, 6.8f, 4.2f}, .l = {-5.0f, 0.9f, -6.4f}},
      {.t = T("3c", 1.0f), .d = 2.8f, .p = {32, 30, -6}, .l = {9, 0, -10}, .fov = 44},
      {.t = T("3d", 0.8f), .d = 2.6f, .p = {6, 23, 31}, .l = {-1, 2, -7}},
      {.t = T("3e", 0.8f), .d = 2.6f, .p = {16, 15, 22}, .l = {-10, 2, 9}},
      {.t = T("3f", 0.6f), .d = 2.6f, .p = {8, 6.4f, 1.4f}, .l = {0.5f, 2.2f, -11.2f}},
      {.t = T("3g", 0.6f), .d = 3.0f, .p = {36, 36, 26}, .l = {3, 0, -7}},
      {.t = T("3i", 0.4f), .d = 3.0f, .p = {22.5f, 6.2f, -9.5f}, .l = {13.5f, 1.8f, -20.5f}},
  });

  // Jia: down the side path to the quad, then to the library plaza, then back to the shop.
  W.addJia({
      {.t = 115.0f, .x = 14.7f, .z = -20.2f, .y = 0, .ry = 0},
      {.t = T("3a", 1.2f), .x = 14.7f, .z = -20.2f, .y = 0, .ry = 0},
      {.t = T("3a", 4.8f), .x = 14.6f, .z = -13.8f},
      {.t = T("3b", 1.2f), .x = 11, .z = -10.4f},
      {.t = T("3b", 3.8f), .x = 4.8f, .z = -10.4f},
      {.t = T("3b", 5.6f), .x = 0.9f, .z = -8.2f},
      {.t = T("3b", 7.8f), .x = -4.1f, .z = -6.4f, .ry = -1.57f},
      {.t = T("3e", 3.0f), .x = -4.1f, .z = -6.4f, .ry = -1.57f},
      {.t = T("3f", 0.6f), .x = 2.2f, .z = -7.6f, .ry = -0.6f},
      {.t = T("3g", 0.6f), .x = 2.2f, .z = -7.6f, .ry = -0.6f},
      {.t = T("3g", 3.0f), .x = 4.8f, .z = -10.4f},
      {.t = T("3g", 6.8f), .x = 11, .z = -10.4f},
      {.t = T("3h", 2.6f), .x = 14.6f, .z = -13.6f},
      {.t = T("3i", 0.4f), .x = 15.2f, .z = -19.6f, .ry = 0.35f},
  });

  auto r = std::make_shared<Rng>(41);
  auto person = [r](FigOpts o = {}) {
    o.shirt = pick(*r, PAL::shirts);
    o.skin = pick(*r, PAL::skin);
    if ((*r)() < 0.7) o.hair = pick(*r, std::vector<uint32_t>{0x2a2220, 0x4a3426, 0x1d1d22, 0x6b4a2e});
    return figure(o);
  };

  // ---- the students Jia talks to on the quad ----
  std::vector<Figure> grp;
  for (auto [x, z, ry] : std::vector<std::tuple<float, float, float>>{{-5.8f, -5.8f, 1.4f}, {-5.7f, -7.2f, 1.7f}, {-6.7f, -6.5f, 1.57f}}) {
    bool nb = (*r)() < 0.5;
    Figure f = person({.notebook = nb});
    place(f.root, x, 0, z, ry);
    W.scene.add(f.root);
    grp.push_back(f);
  }

  // ---- threads: each finding tied back to the shop ----
  auto shopPt = [](float dx) { return vec3{16.6f + dx, 3.8f, -22.8f}; };
  // Teal yarn: lit like paper, with a little glow of its own so it reads at dusk.
  auto teal = [](float o) { return standard({.color = 0x37b3a5, .roughness = 0.9f, .emissive = 0x37b3a5, .emissiveIntensity = 0.45f,
                                             .opacity = o, .transparent = true, .depthWrite = false, .paper = 0.6f}); };
  struct ThreadDef { const char* key; vec3 from, to; float lift, r; const char* cue; float at; };
  const std::vector<ThreadDef> defs = {
      {"place", {6.8f, 2.9f, 3.9f}, shopPt(-0.9f), 7, 0.12f, "3c", 3.2f},
      {"time", {-12, 7.2f, -3}, shopPt(-0.3f), 7, 0.18f, "3d", 4.2f},
      {"courses", {-7, 5.9f, 21.5f}, shopPt(0.3f), 9, 0.075f, "3e", 3.6f},
      {"study", {0.6f, 2.6f, -7.8f}, shopPt(0.9f), 6, 0.18f, "3f", 3.8f},
  };
  struct Thread { ThreadDef d; GrowTube tube; Mesh* knot; Material* m; };
  std::vector<Thread> threads;
  for (const ThreadDef& d : defs) {
    Material* m = teal(0.85f);
    GrowTube tube = growTube(arc(d.from, d.to, d.lift), d.r, m, 90, 8);
    Mesh* knot = eng::mesh(facetedBall(d.r * 1.0f + 0.1f, 1), teal(0.95f));
    knot->position = d.from;
    W.scene.add(tube.mesh, knot);
    threads.push_back({d, tube, knot, m});
  }

  // ---- finding 1, place: the empty unit where the shop used to be ----
  Node* oldBox = group();
  Geometry* ob = geo::box(3.9f, 2.7f, 0.7f);
  oldBox->add(eng::mesh(ob, teal(0.16f)));
  Mesh* oldEdges = lines(geo::edges(*ob), basic({.color = 0x1f8a80, .opacity = 1, .transparent = true}));
  oldBox->add(oldEdges);
  oldBox->position = {6.8f, 1.4f, 3.9f};
  W.scene.add(oldBox);

  // ---- finding 2, time: a clock that reaches five ----
  Node* clock = group();
  clock->position = {-12, 8.4f, -3};
  Geometry* faceGeo = geo::cylinder(1.5f, 1.5f, 0.16f, 48);
  faceGeo->rotateX(PI / 2);
  Mesh* face = eng::mesh(faceGeo, mat(0xf7f1e4, {.rough = 0.6f}));
  Mesh* rim = eng::mesh(geo::torus(1.5f, 0.09f, 10, 48), mat(0x2f3a44, {.rough = 0.5f}));
  clock->add(face, rim);
  for (int i = 0; i < 12; i++) {
    Mesh* tick = rbox(0.07f, i % 3 == 0 ? 0.3f : 0.18f, 0.04f, mat(0x2f3a44), 0.01f);
    float a = (i / 12.f) * PI * 2;
    tick->position = {std::sin(a) * 1.25f, std::cos(a) * 1.25f, 0.1f};
    tick->rotation.z = -a;
    clock->add(tick);
  }
  auto hand = [&](float len, float w) {
    Node* g = group();
    Mesh* b = rbox(w, len, 0.05f, mat(0x2f3a44), 0.02f);
    b->position.y = len / 2 - 0.1f;
    g->add(b);
    g->position.z = 0.13f;
    clock->add(g);
    return g;
  };
  Node* hourH = hand(0.75f, 0.12f);
  Node* minH = hand(1.15f, 0.08f);
  clock->add(place(eng::mesh(sphereGeo(0.09f, 12, 8), mat(0xd8604f)), 0, 0, 0.18f));
  clock->lookAt({6, 23, 31});
  W.scene.add(clock);

  // Students pouring out of the teaching buildings at five.
  struct Walker { Figure f; const Curve* curve; float len, t0, v, side; };
  std::vector<Walker> burst;
  for (int i = 0; i < 16; i++) {
    bool fromT2 = i % 2 == 1;
    std::vector<vec3> pts = {{-18.6f, 0, fromT2 ? 7.f : 0.f}, {-17.5f, 0, fromT2 ? 7.f : 0.f}, {-17.5f, 0, 2},
                             {i % 3 == 0 ? 0.f : 6 + (i % 5) * 2.6f, 0, 2}};
    if (i % 3 == 0) pts.push_back({0, 0, 12.f + (i % 4)});
    const Curve* curve = &keepCurve(new CatmullRom(pts, false, 0.05f));
    Figure f = person();
    W.scene.add(f.root);
    float v = 1.5f + (float)(*r)() * 0.5f;
    float side = ((float)(*r)() - 0.5f) * 1.4f;
    burst.push_back({f, curve, curve->length(), T("3d", 3.3f) + i * 0.32f, v, side});
  }

  // ---- finding 3, courses: readings fly from the teaching buildings to the dorms ----
  struct Doc { Node* g; const Curve* curve; float t0; };
  std::vector<Doc> docs;
  for (int i = 0; i < 10; i++) {
    Node* g = group();
    Mesh* page = eng::mesh(geo::plane(0.62f, 0.8f), standard({.color = 0xfbf7ee, .roughness = 0.9f, .emissive = 0xfbf7ee, .emissiveIntensity = 0.3f,
                                                               .side = Side::Double, .paper = 1}));
    g->add(page);
    for (int k = 0; k < 4; k++) {
      Mesh* line = eng::mesh(geo::plane(0.42f, 0.05f), basic({.color = 0x1f8a80, .side = Side::Double}));
      line->position = {0, 0.22f - k * 0.14f, 0.005f};
      g->add(line);
    }
    vec3 from = i % 2 ? vec3{-22.5f, 5.8f, 9} : vec3{-22.5f, 6.8f, -3};
    vec3 to = i % 3 == 0 ? vec3{11, 4.2f, 20.6f} : vec3{-7 + (i % 4 - 1.5f) * 2, 4.2f, 20.6f};
    W.scene.add(g);
    docs.push_back({g, &arc(from, to, 6.f + (i % 3)), T("3e", 0.6f) + i * 0.42f});
  }

  // ---- finding 4, study space: groups with nowhere to sit outside a full library ----
  struct Waiting { Figure f; int k, gi; };
  std::vector<Waiting> waiting;
  int gi = 0;
  for (auto [cx, cz] : std::vector<std::pair<float, float>>{{-4.6f, -8.4f}, {0.6f, -7.6f}, {5.6f, -8.6f}}) {
    for (int k = 0; k < 3; k++) {
      float a = (k / 3.f) * PI * 2 + gi;
      Figure f = person({.notebook = k != 1});
      place(f.root, cx + std::cos(a) * 0.65f, 0, cz + std::sin(a) * 0.65f, std::atan2(-std::cos(a), -std::sin(a)));
      W.scene.add(f.root);
      waiting.push_back({f, k, gi});
    }
    gi++;
  }

  // ---- labels ----
  L.add({.kind = Kind::Card, .cls = "quote", .screen = vec2{0.5f, 0.3f}, .en = "No investigation, no right to speak.", .zh = "没有调查，没有发言权。",
         .attr = Pair{"Mao Zedong, “Oppose Book Worship” (1930)", "毛泽东《反对本本主义》（1930年）"},
         .vis = [=](float t) { return win(t, T("3a", 0.4f), c("3b").s + 1.0f, 0.7f, 0.7f); }});
  L.add({.kind = Kind::Tag, .cls = "plain", .at = W.jia, .lift = 1.55f, .en = "“What do you actually do?”", .zh = "“你们实际上是怎么做的？”",
         .vis = [=](float t) { return win(t, T("3b", 7.6f), c("3c").s + 0.6f); }});
  L.add({.kind = Kind::Fact, .at = oldBox, .lift = 1.6f, .en = "It used to be here, on the main path", .zh = "书店原来在这里，就在主路旁",
         .vis = [=](float t) { return win(t, T("3c", 2.6f), c("3d").s + 0.3f); }});
  L.add({.kind = Kind::Fact, .at = shopPt(0), .lift = 0.4f, .en = "Since last year: behind the library, off the main path", .zh = "去年起：在图书馆后面，离开了主路",
         .vis = [=](float t) { return win(t, T("3c", 4.4f), c("3d").s + 0.3f); }});
  L.add({.kind = Kind::Fact, .at = clock, .lift = 1.8f, .en = "Most classes end at 5", .zh = "大多数课五点结束",
         .vis = [=](float t) { return win(t, T("3d", 1.6f), c("3e").s + 0.3f); }});
  L.add({.kind = Kind::Fact, .at = shopPt(0), .lift = 0.4f, .en = "The shop closes at 5", .zh = "书店五点关门",
         .vis = [=](float t) { return win(t, T("3d", 3.6f), c("3e").s + 0.3f); }});
  L.add({.kind = Kind::Fact, .at = vec3{-22.5f, 6.6f, 3}, .en = "Readings posted online", .zh = "阅读资料放到了网上",
         .vis = [=](float t) { return win(t, T("3e", 1.2f), c("3f").s + 0.3f); }});
  L.add({.kind = Kind::Fact, .at = vec3{-7, 5.6f, 20.5f}, .en = "Fewer printed books change hands", .zh = "纸质书的买卖少了",
         .vis = [=](float t) { return win(t, T("3e", 3.4f), c("3f").s + 0.3f); }});
  L.add({.kind = Kind::Fact, .at = vec3{0.6f, 1.6f, -7.6f}, .en = "Nowhere to study together in the evening", .zh = "晚上没有一起学习的地方",
         .vis = [=](float t) { return win(t, T("3f", 2.8f), c("3g").s + 0.4f); }});
  L.add({.kind = Kind::Note, .at = vec3{0, 6.4f, -12}, .stem = false, .en = "The library is full, and it has to stay quiet", .zh = "图书馆满座，而且必须保持安静",
         .vis = [=](float t) { return win(t, T("3f", 4.4f), c("3g").s + 0.4f); }});
  for (auto [key, en, zh] : std::vector<std::tuple<std::string, std::string, std::string>>{
           {"place", "Place", "位置"}, {"time", "Time", "时间"}, {"courses", "Courses", "课程"}, {"study", "Study space", "学习空间"}}) {
    vec3 from;
    for (const Thread& th : threads)
      if (th.d.key == key) from = th.d.from;
    L.add({.kind = Kind::Fact, .cls = "small", .at = from, .lift = 0.5f, .en = en, .zh = zh, .noKicker = true,
           .vis = [=](float t) { return win(t, T("3g", 1.6f), c("3i").s); }});
  }
  L.add({.kind = Kind::Principle, .at = shopPt(0), .lift = 1.6f, .en = "Universal connection: but some connections weigh more", .zh = "普遍联系：但联系有强有弱、有主有次",
         .vis = [=](float t) { return win(t, T("3h", 0.4f), c("3i").s + 0.6f); }});
  L.add({.kind = Kind::Panel, .cls = "teal", .screen = vec2{0.03f, 0.4f}, .anchor = vec2{0, 0.5f}, .stem = false,
         .panel = PanelSpec{.h4 = {"What Jia found", "小嘉的发现"},
                            .items = {{"Off the main path", "", "不在主路旁"},
                                      {"Closes at 5, just as classes end", "", "五点关门，正是下课时"},
                                      {"Readings are online", "", "阅读资料放到了网上"},
                                      {"Nowhere to study together in the evening", "", "晚上没有一起学习的地方"}}},
         .vis = [=](float t) { return win(t, T("3i", 0.8f), ch.e + 1.2f); }});

  return [=](float t) {
    bool live = t > ch.s - 1 && t < ch.e + 2;

    // Students on the quad: visible while Jia is there; they turn to listen.
    for (size_t i = 0; i < grp.size(); i++) {
      const Figure& f = grp[i];
      f->visible = t > T("3a") && t < c("3f").s + 1;
      pose(f, {.look = std::sin(t * 1.3f + i * 2) * 0.25f * ramp(t, T("3b", 7), T("3b", 8)), .nod = std::max(0.f, std::sin(t * 2.2f + i)) * 0.1f});
    }

    // Threads: grow with their finding, stay for the overview, then the thick ones carry the weight.
    float emphasis = ramp(t, T("3h", 2.6f), T("3h", 4.2f));
    for (const Thread& th : threads) {
      bool show = live && t < ch.e + 1.5f;
      float g = ramp(t, T(th.d.cue, th.d.at), T(th.d.cue, th.d.at + 1.6f));
      th.tube.grow(show ? g : 0);
      scaleIn(th.knot, show ? backOut(ramp(t, T(th.d.cue, th.d.at - 0.3f), T(th.d.cue, th.d.at + 0.4f), linear)) : 0);
      bool weak = th.d.r < 0.15f;
      float o = (1 - ramp(t, T("3i", -0.9f), T("3i", 0.5f))) * (weak ? 1 - 0.6f * emphasis : 1);
      th.m->opacity = 0.85f * o;
      th.knot->material->opacity = 0.95f * o;
    }

    // Finding 1.
    fade(oldBox, win(t, T("3c", 1.6f), c("3d").s + 0.6f));
    oldEdges->material->opacity = 0.65f + 0.35f * std::sin(t * 3);

    // Finding 2: the clock sweeps from 4:40 to 5:00, then students stream out.
    float clockOn = win(t, T("3d", 0.2f), c("3e").s + 0.8f);
    scaleIn(clock, backOut(clockOn) * 1.5f);
    float minutes = lerp(40, 60, ramp(t, T("3d", 0.8f), T("3d", 3.2f)));
    minH->rotation.z = -(minutes / 60) * PI * 2;
    hourH->rotation.z = -((4 + minutes / 60) / 12) * PI * 2;
    for (const Walker& b : burst) {
      float u = (t - b.t0) * b.v / b.len;
      if (u <= 0 || u >= 1) { b.f->visible = false; continue; }
      b.f->visible = true;
      vec3 tp = b.curve->pointAt(u), tt = b.curve->tangentAt(u);
      b.f->position = {tp.x + tt.z * b.side * 0.5f, 0, tp.z - tt.x * b.side * 0.5f};
      b.f->rotation.y = std::atan2(tt.x, tt.z);
      b.f->scale.setScalar(smooth(u / 0.03f) * smooth((1 - u) / 0.04f));
      pose(b.f, {.walking = 1, .phase = (t - b.t0) * b.v * 4.2f});
    }

    // Finding 3: pages in flight.
    for (const Doc& d : docs) {
      float u = (t - d.t0) / 2.4f;
      d.g->visible = u > 0 && u < 1;
      if (!d.g->visible) continue;
      d.g->position = d.curve->point(smooth(u));
      d.g->rotation = {0.2f, u * 5, std::sin(u * 9) * 0.3f};
      d.g->scale.setScalar(1.7f * std::min({1.f, u * 6, (1 - u) * 5}));
    }

    // Finding 4: the evening groups.
    for (const Waiting& w : waiting) {
      w.f->visible = t > T("3e", 4) && t < c("3h").s;
      pose(w.f, {.look = std::sin(t * 0.9f + w.k * 2 + w.gi) * 0.4f});
    }
  };
}

}  // namespace film
