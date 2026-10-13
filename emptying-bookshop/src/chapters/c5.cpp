// Chapter 5 · Test, Accumulate, Revise
// Three small tests; regulars accumulate until the shop changes in kind; a setback; a revision.
#include "chapters.h"

namespace film {

Update buildC5(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[4];

  W.addLight({{T("5a", 2), "dusk", 3}, {T("5b", 2.5f), "evening", 3}});
  W.addCrowd({{T("5a", 1), 0.4f, 2}});
  W.addShop({{.t = T("5b", 1.2f), .door = script::tr("OPEN\nUNTIL 9", "营业到\n晚上9点"), .lit = 1}});

  W.addCam({
      {.t = T("5a", 0.8f), .d = 3.0f, .p = {21, 5.6f, -11.5f}, .l = {16.8f, 1.4f, -21.2f}},
      {.t = T("5b", 2.6f), .d = 2.4f, .p = {18.6f, 5.0f, -11.8f}, .l = {14.8f, 1.4f, -21.0f}},
      {.t = T("5c", 0.6f), .d = 2.4f, .p = {22.5f, 6.0f, -11.0f}, .l = {18.0f, 1.2f, -20.6f}},
      {.t = T("5d", 0.4f), .d = 2.4f, .p = {19.5f, 6.4f, -9.6f}, .l = {16.2f, 1.2f, -20.4f}},
      {.t = T("5g", 0.6f), .d = 2.6f, .p = {24, 8.8f, -5.5f}, .l = {16.2f, 1.2f, -21}},
      {.t = T("5i", 0.6f), .d = 2.6f, .p = {26, 13.5f, 0.5f}, .l = {11, 1.5f, -19}},
      {.t = T("5j", 0.8f), .d = 2.4f, .p = {27, 9.5f, -7.5f}, .l = {19, 1.2f, -21.4f}},
      {.t = T("5k", 1.0f), .d = 3.2f, .p = {29, 12.5f, -2.5f}, .l = {16.5f, 2, -21}},
  });

  W.addJia({
      {.t = T("5a", 0.2f), .x = 16.9f, .z = -19.4f, .y = 0, .ry = PI},
      {.t = T("5a", 1.2f), .x = 16.9f, .z = -19.4f, .y = 0, .ry = 0.3f},
      {.t = T("5b", 2.0f), .x = 14.9f, .z = -20.6f, .y = 0, .ry = 0.2f},
      {.t = T("5c", 2.2f), .x = 16.6f, .z = -18.2f, .y = 0, .ry = 0.4f},
      {.t = T("5j", 0.4f), .x = 16.6f, .z = -18.2f, .y = 0, .ry = 0.4f},
      {.t = T("5j", 3.2f), .x = 22.6f, .z = -18.4f, .y = 0, .ry = -0.6f},
      {.t = T("5k", 1.6f), .x = 22.6f, .z = -18.4f, .y = 0, .ry = -0.4f},
      {.t = T("5k", 5.0f), .x = 17.8f, .z = -18.0f, .y = 0, .ry = 0.3f},
  });

  Rng r(61);

  // ---- test 2: the swap shelf ----
  Node* swap = group();
  swap->add(shelf(1.3f, 1.3f, 0.42f, {.seed = 12, .rows = 3}));
  Sign* swapSign = sign({.w = 1.3f, .h = 0.32f, .text = script::tr("SWAP · A BOOK FOR A BOOK", "换书架 · 以书换书"), .px = 120, .fg = 0xfff7e6, .bg = 0x2f6b55});
  swapSign->mesh->position = {0, 1.5f, 0.05f};
  swap->add(swapSign->mesh);
  swap->position = {12.8f, 0, -21.3f};
  W.scene.add(swap);

  // ---- test 3: a patio of borrowed tables, lights, and a sign ----
  Node* patio = group();
  patio->add(place(rbox(6.6f, 0.1f, 3.4f, matS(0xa98763, 0.85f), 0.03f), 18.7f, 0.05f, -20.15f));
  Node* studySign = group();
  studySign->add(place(rbox(0.06f, 1.0f, 0.06f, PAL::woodDark, 0.01f), -0.3f, 0.5f, 0));
  studySign->add(place(rbox(0.06f, 1.0f, 0.06f, PAL::woodDark, 0.01f), 0.3f, 0.5f, 0));
  Sign* ss = sign({.w = 0.9f, .h = 0.6f, .text = script::tr("STUDY\nHERE", "欢迎来\n学习"), .px = 160, .fg = 0x2f3a44, .bg = 0xfbf6ea});
  ss->mesh->position = {0, 1.0f, 0.04f};
  studySign->add(ss->mesh);
  place(studySign, 15.9f, 0.1f, -18.5f, 0.3f);
  patio->add(studySign);
  // String lights between two poles.
  std::vector<Mesh*> bulbs;
  for (float x : {15.4f, 22.1f}) patio->add(place(rbox(0.08f, 2.7f, 0.08f, 0x3b4250, 0.02f), x, 1.35f, -18.45f));
  for (int i = 0; i <= 12; i++) {
    float u = i / 12.f;
    Mesh* b = eng::mesh(sphereGeo(0.07f, 10, 8), standard({.color = 0xfff1d6, .emissive = 0xffc77a, .emissiveIntensity = 0}));
    b->position = {lerp(15.4f, 22.1f, u), 2.6f - std::sin(u * PI) * 0.45f, -18.45f};
    patio->add(b);
    bulbs.push_back(b);
  }
  W.scene.add(patio);
  PointLight* patioLight = pointLight(0xffc98a, 0, 12, 1.5f);
  patioLight->position = {18.7f, 2.6f, -19.6f};
  W.scene.add(patioLight);

  struct Table { Node* g; vec3 home; };
  std::vector<Table> tables;
  for (auto [x, z] : std::vector<std::pair<float, float>>{{16.3f, -20.5f}, {17.9f, -19.8f}, {19.5f, -20.5f}, {21.1f, -19.8f}}) {
    Node* g = group();
    g->add(roundTable(0.48f));
    for (int k = 0; k < 4; k++) {
      float a = (k / 4.f) * PI * 2 + PI / 4;
      g->add(place(stool(), std::cos(a) * 0.72f, 0, std::sin(a) * 0.72f));
      Mesh* bk = rbox(0.24f, 0.04f, 0.17f, matS(pick(r, std::vector<uint32_t>{0xb85c4a, 0x4f6f9a, 0xd9b45a, 0xe0ddd2}), 0.9f), 0.01f);
      bk->position = {std::cos(a) * 0.25f, 0.66f, std::sin(a) * 0.25f};
      bk->rotation.y = -a;
      g->add(bk);
    }
    g->position = {x, 0.1f, z};
    W.scene.add(g);
    tables.push_back({g, {x, 0.1f, z}});
  }
  // Where the talk tables go after the revision, behind a hedge on the far side.
  const vec3 corner[2] = {{25.2f, 0, -20.2f}, {25.4f, 0, -22.6f}};
  Mesh* hedgeM = hedge(5.6f, 1.1f, 0.7f);
  hedgeM->rotation.y = PI / 2;
  hedgeM->position = {23.6f, 0.55f, -21.2f};
  W.scene.add(hedgeM);
  Sign* timetable = sign({.w = 0.55f, .h = 0.75f, .text = script::tr("QUIET\n6–9\n—\nTALK:\nFAR SIDE", "6–9点\n安静自习\n—\n讨论请到\n远侧"), .px = 200, .fg = 0x2f3a44, .bg = 0xfbf6ea, .size = 0.82f});
  timetable->mesh->position = {13.85f, 1.45f, -21.84f};
  W.scene.add(timetable->mesh);

  // ---- people: seats fill week by week, then a Tuesday group, then others ----
  struct Seat { Figure f; int ti; float a; int k; float t0 = -1; };
  std::vector<Seat> seats;
  for (size_t ti = 0; ti < tables.size(); ti++)
    for (int k = 0; k < 4; k++) {
      float a = (k / 4.f) * PI * 2 + PI / 4;
      FigOpts fo{.shirt = pick(r, PAL::shirts), .skin = pick(r, PAL::skin)};
      if (r() < 0.7) fo.hair = pick(r, std::vector<uint32_t>{0x2a2220, 0x4a3426, 0x1d1d22, 0x6b4a2e});
      Figure f = seated(fo);
      W.scene.add(f.root);
      seats.push_back({f, (int)ti, a, k});
    }
  // Fill order and the time each seat is taken.
  const int order[16] = {1, 6, 9, 4, 13, 2, 7, 10, 0, 5, 8, 11, 12, 14, 15, 3};
  const float times[16] = {
      T("5d", 0.6f), T("5d", 0.6f), T("5d", 2.4f), T("5d", 4.2f), T("5d", 4.2f),
      T("5e", 1.0f), T("5e", 2.6f), T("5e", 4.2f),
      T("5f", 0.6f), T("5f", 0.6f), T("5f", 0.9f), T("5f", 0.9f), T("5f", 1.2f), T("5f", 3.2f), T("5f", 3.4f), T("5f", 3.6f),
  };
  for (int n = 0; n < 16; n++) seats[order[n]].t0 = times[n];
  std::vector<Figure> browsers;
  for (int i = 0; i < 2; i++) {
    Figure f = figure({.shirt = pick(r, PAL::shirts), .skin = pick(r, PAL::skin), .hair = 0x2a2220});
    place(f.root, 12.4f + i * 0.8f, 0, -20.3f, i ? -0.4f : 0.3f);
    W.scene.add(f.root);
    browsers.push_back(f);
  }

  // ---- the jar: each regular adds a bead; at a threshold the beads link up ----
  Node* jar = group();
  jar->position = {11.3f, 0, -18.7f};
  jar->scale.setScalar(1.25f);
  jar->add(place(rbox(1.2f, 0.7f, 1.2f, matS(0x3d4555, 0.6f), 0.08f), 0, 0.35f, 0));
  Mesh* glass = eng::mesh(geo::cylinder(0.56f, 0.56f, 1.35f, 32, 1, true),
                          standard({.color = 0xcfe6f2, .roughness = 0.1f, .opacity = 0.22f, .transparent = true, .depthWrite = false, .side = Side::Double}));
  glass->position.y = 1.38f;
  jar->add(glass);
  jar->add(place(eng::mesh(cylinderGeo(0.56f, 0.56f, 0.03f, 32), standard({.color = 0xcfe6f2, .opacity = 0.3f, .transparent = true})), 0, 0.72f, 0));
  const int NB = 46;
  Material* beadMat = standard({.color = 0xffffff, .roughness = 0.4f, .emissive = 0xffcf5a, .emissiveIntensity = 0});
  InstancedMesh* beads = instanced(sphereGeo(0.085f, 12, 8), beadMat, NB);
  beads->frustumCulled = false;
  jar->add(beads);
  std::vector<vec3> rest;
  std::vector<float> beadT;
  for (int i = 0; i < NB; i++) {
    int layer = i / 8, k = i % 8;
    float a = (k / 8.f) * PI * 2 + layer * 0.4f;
    float rr = k == 7 ? 0.05f : 0.32f + (layer % 2) * 0.06f;
    rest.push_back({std::cos(a) * rr, 0.83f + layer * 0.15f, std::sin(a) * rr});
    beadT.push_back(lerp(T("5d", 0.4f), c("5f").e, std::pow(i / (float)(NB - 1), 0.9f)));
  }
  std::vector<vec3> pairs;
  for (int i = 0; i < NB; i++)
    for (int j = i + 1; j < NB; j++)
      if (distance(rest[i], rest[j]) < 0.36f) { pairs.push_back(rest[i]); pairs.push_back(rest[j]); }
  Mesh* links = lines(geo::lineSegments(pairs), basic({.color = 0xffcf5a, .opacity = 0, .transparent = true}));
  jar->add(links);
  W.scene.add(jar);

  // ---- rings: the change spreading out, then noise reaching the library ----
  std::vector<Mesh*> pulses, noise;
  for (int i = 0; i < 2; i++) { pulses.push_back(ringMesh(1, 0.07f, 0xffcf5a, 0.8f)); W.scene.add(pulses.back()); }
  for (int i = 0; i < 4; i++) { noise.push_back(ringMesh(1, 0.06f, 0xff7a6b, 0.8f)); W.scene.add(noise.back()); }

  // ---- labels ----
  L.add({.kind = Kind::Tag, .at = W.jia, .lift = 1.55f, .en = "From analysis to small tests", .zh = "把分析变成小试验",
         .vis = [=](float t) { return win(t, T("5a", 1.6f), c("5b").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .at = vec3{15.2f, 2.0f, -21.9f}, .en = "Open until 9", .zh = "营业到晚上九点", .kicker = Pair{"Test 1", "试验一"},
         .vis = [=](float t) { return win(t, T("5b", 1.4f), c("5d").s); }});
  L.add({.kind = Kind::Tag, .at = vec3{12.8f, 1.9f, -21.3f}, .anchor = vec2{0.75f, 1}, .en = "Swap shelf: a book for a book", .zh = "换书架：以书换书",
         .kicker = Pair{"Test 2", "试验二"}, .vis = [=](float t) { return win(t, T("5b", 4.0f), c("5d").s); }});
  L.add({.kind = Kind::Tag, .at = vec3{19.4f, 1.5f, -20.2f}, .en = "Tables, lights, and a “Study here” sign", .zh = "桌子、灯串和“欢迎来学习”的牌子",
         .kicker = Pair{"Test 3", "试验三"}, .vis = [=](float t) { return win(t, T("5c", 2.0f), c("5d").s + 0.3f); }});
  auto* week = L.add({.kind = Kind::Tag, .cls = "small plain", .at = vec3{14.9f, 2.7f, -21.8f}, .stem = false, .en = "Week 1", .zh = "第1周",
                      .vis = [=](float t) { return win(t, T("5d", 0.3f), c("5e").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .at = vec3{19.5f, 1.6f, -20.0f}, .en = "On the surface, little has changed", .zh = "表面上，变化不大",
         .vis = [=](float t) { return win(t, T("5d", 3.6f), c("5e").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .at = jar, .lift = 2.9f, .en = "Adding up: people who know the shop, and their friends", .zh = "在积累：熟悉这家店的人，和他们的朋友",
         .vis = [=](float t) { return win(t, T("5e", 1.0f), c("5g").s); }});
  L.add({.kind = Kind::Tag, .at = vec3{18.7f, 1.6f, -20.0f}, .en = "Tuesday study group", .zh = "周二学习小组",
         .vis = [=](float t) { return win(t, T("5f", 1.0f), c("5g").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .cls = "strike", .at = vec3{17, 3.7f, -22.3f}, .en = "A place you pass through", .zh = "路过的地方",
         .vis = [=](float t) { return win(t, T("5g", 3.4f), c("5h").s + 0.6f); }});
  L.add({.kind = Kind::Tag, .at = vec3{18.7f, 1.6f, -20.0f}, .en = "A place you meet", .zh = "相聚的地方",
         .vis = [=](float t) { return win(t, T("5g", 5.6f), c("5h").e); }});
  L.add({.kind = Kind::Principle, .at = jar, .lift = 2.9f, .en = "Quantitative change → qualitative change", .zh = "量变 → 质变",
         .vis = [=](float t) { return win(t, T("5h", 0.6f), c("5i").s + 0.2f); }});
  L.add({.kind = Kind::Flaw, .at = vec3{7, 7.2f, -18}, .en = "Library: “Too noisy!”", .zh = "图书馆：“太吵了！”",
         .vis = [=](float t) { return win(t, T("5i", 1.6f), c("5j").s + 0.6f); }});
  L.add({.kind = Kind::Flaw, .at = vec3{17, 3.8f, -22.3f}, .en = "Practice reveals what the analysis missed", .zh = "实践暴露出分析中遗漏的问题",
         .vis = [=](float t) { return win(t, T("5i", 3.4f), c("5j").s + 0.6f); }});
  L.add({.kind = Kind::Tag, .at = vec3{17.2f, 1.5f, -20.4f}, .en = "Quiet study by the window", .zh = "窗边安静自习",
         .vis = [=](float t) { return win(t, T("5j", 2.6f), c("5k").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .at = vec3{25.3f, 1.5f, -21.4f}, .en = "Talk corner, far side", .zh = "远侧讨论角",
         .vis = [=](float t) { return win(t, T("5j", 3.6f), c("5k").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .at = vec3{13.85f, 1.9f, -21.8f}, .anchor = vec2{1, 1}, .en = "Timetable on the door", .zh = "门上贴出时间表",
         .vis = [=](float t) { return win(t, T("5j", 5.6f), c("5k").s + 0.4f); }});
  L.add({.kind = Kind::Card, .cls = "quote", .screen = vec2{0.5f, 0.28f}, .en = "Practice, knowledge, again practice, and again knowledge.",
         .zh = "实践、认识、再实践、再认识。", .attr = Pair{"Mao Zedong, “On Practice” (1937)", "毛泽东《实践论》（1937年）"},
         .vis = [=](float t) { return win(t, T("5k", 0.4f), ch.e + 0.6f, 0.7f, 0.8f); }});

  const Color cold = Color::hex(0xdfe6ee), warm = Color::hex(0xffd27a);

  return [=, &W, &L](float t) {
    bool live = t > ch.s - 0.5f;  // from here to the end of the film, the changes persist

    scaleIn(swap, live ? backOut(ramp(t, T("5b", 3.6f), T("5b", 4.4f), linear)) : 0);
    float patioIn = live ? ramp(t, T("5c", 0.3f), T("5c", 1.3f)) : 0;
    patio->visible = patioIn > 0.01f;
    patio->position.y = (1 - patioIn) * -0.3f;
    float glowAmt = live ? ramp(t, T("5c", 1.0f), T("5c", 2.0f)) * (0.55f + 0.45f * ramp(t, T("5g", 2.4f), T("5g", 4.0f))) : 0;
    for (Mesh* b : bulbs) b->material->emissiveIntensity = 3 * glowAmt * (0.4f + 0.6f * W.night);
    patioLight->intensity = glowAmt * 9 * (0.3f + 0.7f * W.night);
    patioLight->color = Color::hex(0xffe0b0).lerp(warm, ramp(t, T("5g", 2.4f), T("5g", 4.0f)));

    // Tables: in, then the two talk tables move behind the hedge.
    float move = ramp(t, T("5j", 0.6f), T("5j", 2.8f));
    for (size_t i = 0; i < tables.size(); i++) {
      const Table& tb = tables[i];
      float s = live ? backOut(ramp(t, T("5c", 0.8f + i * 0.25f), T("5c", 1.6f + i * 0.25f), linear)) : 0;
      scaleIn(tb.g, s);
      tb.g->position = i >= 2 ? lerp(tb.home, corner[i - 2], move) : tb.home;
    }
    scaleIn(hedgeM, live ? backOut(ramp(t, T("5j", 0.2f), T("5j", 1.0f), linear)) : 0);
    hedgeM->scale.x = hedgeM->scale.z = hedgeM->scale.y;
    fade(timetable->mesh, live ? ramp(t, T("5j", 5.0f), T("5j", 5.6f)) : 0);

    for (const Seat& s : seats) {
      float on = live && s.t0 >= 0 ? ramp(t, s.t0, s.t0 + 0.5f) : 0;
      s.f->visible = on > 0.01f;
      if (!s.f->visible) continue;
      Node* tb = tables[s.ti].g;
      s.f->position = {tb->position.x + std::cos(s.a) * 0.72f, 0.1f + (1 - on) * 0.4f, tb->position.z + std::sin(s.a) * 0.72f};
      s.f->rotation.y = std::atan2(-std::cos(s.a), -std::sin(s.a));
      s.f->scale.setScalar(on);
      float talking = s.ti >= 2 ? 1 : 0.3f;
      pose(s.f, {.look = std::sin(t * (1.1f + s.k * 0.3f) + s.k * 2) * 0.35f * talking, .nod = 0.25f + std::sin(t * 1.7f + s.k) * 0.08f});
    }
    for (size_t i = 0; i < browsers.size(); i++) {
      browsers[i]->visible = live && t > T("5e", 1.8f + i * 1.2f);
      pose(browsers[i], {.look = std::sin(t * 0.8f + i) * 0.3f, .nod = 0.2f});
    }

    // Beads drop in one by one; at the threshold they turn gold and link into a network.
    float linked = ramp(t, T("5g", 2.6f), T("5g", 4.6f));
    for (int i = 0; i < NB; i++) {
      float u = live ? ramp(t, beadT[i], beadT[i] + 0.7f, [](float x) { return x * x; }) : 0;
      vec3 p = rest[i];
      p.y += (1 - u) * 1.6f;
      float sc = u > 0 ? 1 : 0.0001f;
      beads->matrices[i] = mat4::compose(p, {}, {sc, sc, sc});
      beads->setColor(i, cold.lerp(warm, linked));
    }
    beadMat->emissiveIntensity = linked * 0.8f;
    links->material->opacity = linked * 0.9f;
    jar->visible = live && t > T("5d", 0.0f) - 0.4f;
    scaleIn(jar, 1.25f * (live ? backOut(ramp(t, T("5d", -0.4f), T("5d", 0.4f), linear)) : 0));

    // A gold ring spreads from the shop at the change in kind.
    for (size_t i = 0; i < pulses.size(); i++) {
      Mesh* m = pulses[i];
      float u = (t - T("5g", 3.0f + i * 0.9f)) / 2.6f;
      m->visible = live && u > 0 && u < 1;
      if (!m->visible) continue;
      float R = lerp(1.5f, 16, smooth(u));
      m->scale = {R, R, 1};
      m->position = {17.5f, 0.25f, -20.5f};
      m->material->opacity = 0.8f * (1 - u);
    }
    // Noise: rings reach the library; after the revision they are smaller and stop at the hedge.
    for (size_t i = 0; i < noise.size(); i++) {
      Mesh* m = noise[i];
      const float period = 1.6f;
      float start = T("5i", 0.4f);
      float k = (t - start - i * 0.4f) / period;
      float u = k - std::floor(k);
      bool revised = t > T("5j", 2.6f);
      m->visible = live && t > start + i * 0.4f && t < c("5k").s;
      if (!m->visible) continue;
      float R = revised ? lerp(0.6f, 2.4f, u) : lerp(0.8f, 12, u);
      m->scale = {R, R, 1};
      m->position = revised ? vec3{25.3f, 0.7f, -21.4f} : vec3{20.3f, 0.7f, -20.2f};
      m->material->opacity = 0.75f * (1 - u) * (revised ? 0.7f : 1);
    }

    int wk = 1 + (int)std::min(2.f, std::floor(clamp((t - T("5d", 0.3f)) / 4.0f) * 3));
    L.set(week, "Week " + std::to_string(wk), "第" + std::to_string(wk) + "周");
  };
}

}  // namespace film
