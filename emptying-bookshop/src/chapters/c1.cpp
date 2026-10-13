// Chapter 1 · An Emptying Shop
// The campus, Jia, the shop's history as a swap shelf, and a time-lapse of fewer visitors each week.
#include "chapters.h"

namespace film {

Update buildC1(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[0];

  W.addLight({{0, "day", 1}});
  // The crowd walks on once the model has stood up.
  W.addCrowd({{0, 0, 1}, {4.8f, 0.85f, 1.6f}});
  W.addWarp(c("1c").s + 0.2f, c("1c").e, 4);

  W.addCam({
      {.t = 0, .p = {46, 40, 52}, .l = {3, 0, -7}, .fov = 38, .push = 0.06f},
      {.t = T("1a", 3.0f), .d = 5.5f, .p = {27, 9.5f, -5}, .l = {16, 1.8f, -23}},
      {.t = T("1b", 1.2f), .d = 2.4f, .p = {25, 10.5f, -2}, .l = {13.2f, 3.4f, -23}},
      {.t = T("1c", 0.9f), .d = 2.2f, .p = {24.5f, 8.5f, -4}, .l = {13.5f, 0.8f, -17.5f}},
      {.t = T("1d", 1.8f), .d = 2.4f, .p = {18.6f, 3.0f, -14.6f}, .l = {14.9f, 1.3f, -21.4f}},
      {.t = T("1e", 1.6f), .d = 3.6f, .p = {36, 22, 12}, .l = {9, 0, -12}},
  });

  // Jia starts behind the counter, then walks to the doorway to look out.
  const float d = c("1d").s;
  W.addJia({
      {.t = 0, .x = 17.6f, .z = -23.9f, .y = 0.12f, .ry = 0.3f},
      {.t = d + 0.4f, .x = 17.6f, .z = -23.9f, .y = 0.12f, .ry = 0.3f},
      {.t = d + 2.6f, .x = 14.7f, .z = -22.5f, .y = 0.12f},
      {.t = d + 4.4f, .x = 14.7f, .z = -21.2f, .y = 0, .ry = 0.5f},
  });

  // ---- the shop's origin: a swap shelf in a dorm lounge, shown in a floating vignette ----
  Node* bubble = group();
  bubble->position = {9.2f, 6.6f, -24.5f};
  bubble->add(place(eng::mesh(cylinderGeo(2.4f, 2.25f, 0.28f, 48), matS(0xefe6d4, 0.85f)), 0, -0.14f, 0));
  bubble->add(place(eng::mesh(cylinderGeo(1.5f, 1.5f, 0.02f, 40), matS(0xb5694f, 0.95f)), 0, 0.01f, 0.2f));
  bubble->add(place(sofa(0x7c8fa6), -0.4f, 0, 0.45f, 0.15f));
  bubble->add(place(shelf(1.1f, 1.3f, 0.35f, {.seed = 9, .rows = 3, .messy = true}), 1.15f, 0, -0.9f, -0.4f));
  Sign* swap = sign({.w = 0.9f, .h = 0.26f, .text = script::tr("FREE SWAP", "免费换书"), .px = 120, .fg = 0xfff7e6, .bg = 0xb5694f});
  swap->mesh->position = {1.15f, 1.48f, -0.7f};
  swap->mesh->rotation.y = -0.4f;
  bubble->add(swap->mesh);
  Rng r(21);
  for (auto [x, z] : std::vector<std::pair<float, float>>{{-0.85f, 0.45f}, {0.05f, 0.5f}}) {
    Figure f = seated({.shirt = pick(r, PAL::shirts), .skin = pick(r, PAL::skin)});
    f->position = {x, 0, z};
    f->rotation.y = 0.15f;
    bubble->add(f.root);
  }
  for (int i = 0; i < 4; i++) {
    Mesh* b = rbox(0.28f, 0.05f, 0.2f, matS(pick(r, std::vector<uint32_t>{0xb85c4a, 0x4f6f9a, 0xd9b45a, 0x6f9a7a}), 0.9f), 0.01f);
    float bx = 0.5f + (float)r() * 0.8f;
    float bz = 0.3f + (float)r() * 0.8f;
    b->position = {bx, 0.03f, bz};
    b->rotation.y = (float)r() * 3;
    bubble->add(b);
  }
  W.scene.add(bubble);
  GrowTube link = growTube(arc({10.6f, 6.2f, -24.2f}, {14.6f, 3.7f, -24.4f}, 1.6f), 0.045f, glow(0xf4efe6, 0.75f), 50, 6);
  W.scene.add(link.mesh);

  // ---- visitors along the side path: a few at first, then a time-lapse that thins out ----
  std::vector<std::pair<float, float>> pts{{4.8f, -10.4f}};
  for (size_t i = 1; i + 1 < SIDE_PATH.size(); i++) pts.push_back(SIDE_PATH[i]);
  pts.push_back({14.6f, -21.9f});
  const Curve* path = &polyPath(pts);
  const float len = path->length();
  const float cs = c("1c").s;
  struct Visitor { Figure f; float t0, v, side; };
  std::vector<std::pair<float, float>> spawns;
  for (float s : {0.f, 3.6f, 8.2f, 13.5f}) spawns.push_back({ch.s + s, 1.3f});
  for (float s : {0.3f, 0.7f, 1.05f, 1.4f, 1.85f, 2.4f, 3.1f, 4.0f, 5.3f, 7.0f}) spawns.push_back({cs + s, 7.5f});
  Rng vr(8);
  std::vector<Visitor> visitors;
  for (auto [t0, v] : spawns) {
    FigOpts fo{.shirt = pick(vr, PAL::shirts), .skin = pick(vr, PAL::skin)};
    if (vr() < 0.6) fo.hair = pick(vr, std::vector<uint32_t>{0x2a2220, 0x4a3426, 0x1d1d22});
    Figure f = figure(fo);
    W.scene.add(f.root);
    float side = ((float)vr() - 0.5f) * 0.5f;
    visitors.push_back({f, t0, v, side});
  }

  // ---- labels ----
  L.add({.kind = Kind::Title, .screen = vec2{0.5f, 0.42f}, .stem = false, .en = "The Emptying Bookshop", .zh = "日渐冷清的书店",
         .sub = Pair{"How to apply Marxist principles to improve dialectical thinking skills", "运用马克思主义基本原理，提升辩证思维能力"},
         .vis = [=](float t) { return win(t, 0.5f, c("1a").s - 0.2f, 1.2f, 1.0f); }});
  L.add({.kind = Kind::Tag, .at = W.jia, .lift = 1.55f, .en = "Jia runs the student second-hand bookshop", .zh = "小嘉经营着学生二手书店",
         .vis = [=](float t) { return win(t, c("1a").s + 3.4f, c("1b").s + 0.8f); }});
  L.add({.kind = Kind::Tag, .at = bubble, .lift = 3.0f, .en = "Years ago: a free swap shelf in a dorm lounge", .zh = "几年前：宿舍休息室里的免费换书架",
         .vis = [=](float t) { return win(t, c("1b").s + 0.9f, c("1c").s + 0.4f); }});
  L.add({.kind = Kind::Tag, .at = vec3{18.5f, 3.6f, -23}, .en = "Later: a proper shop", .zh = "后来：一家正式的书店",
         .vis = [=](float t) { return win(t, c("1b").s + 3.6f, c("1c").s + 0.4f); }});
  auto* week = L.add({.kind = Kind::Tag, .cls = "small plain", .at = vec3{14.6f, 2.9f, -21.6f}, .en = "This term · week 1", .zh = "本学期 · 第1周",
                      .vis = [=](float t) { return win(t, cs + 0.3f, c("1d").s + 0.2f); }});
  L.add({.kind = Kind::Tag, .at = vec3{12.6f, 0.6f, -14.2f}, .anchor = vec2{1, 1}, .offset = {-10, 0}, .en = "Fewer visitors each week",
         .zh = "上门的人一周比一周少", .vis = [=](float t) { return win(t, cs + 3.0f, c("1d").s + 0.6f); }});
  L.add({.kind = Kind::Card, .screen = vec2{0.5f, 0.14f}, .anchor = vec2{0.5f, 0}, .en = "How should Jia think about the problem?",
         .zh = "该怎样思考这个问题？", .vis = [=](float t) { return win(t, c("1d").s + 2.6f, c("1e").s + 0.8f); }});
  L.add({.kind = Kind::Note, .screen = vec2{0, 1}, .anchor = vec2{0, 1}, .offset = {14, 0}, .stem = false,
         .en = "Jia, the shop and the campus are fictional.", .zh = "小嘉、书店和校园都是虚构的。",
         .vis = [=](float t) { return win(t, c("1e").s + 0.4f, ch.e + 0.2f); }});

  return [=, &W, &L](float t) mutable {
    if (t < 2) W.fx.fade = std::max(W.fx.fade, 1 - ramp(t, 0.1f, 1.8f));

    float bIn = backOut(ramp(t, c("1b").s + 0.2f, c("1b").s + 1.1f, linear));
    float bOut = 1 - ramp(t, c("1c").s, c("1c").s + 0.6f);
    scaleIn(bubble, bIn * bOut * 1.35f);
    bubble->position.y = 6.6f + std::sin(t * 0.9f) * 0.08f;
    bubble->rotation.y = -0.35f + std::sin(t * 0.25f) * 0.05f;
    link.grow(ramp(t, c("1b").s + 2.4f, c("1b").s + 3.6f) * bOut);

    for (const Visitor& v : visitors) {
      float u = (t - v.t0) * v.v / len;
      if (u <= 0 || u >= 1 || t > ch.e) { v.f->visible = false; continue; }
      v.f->visible = true;
      vec3 tp = path->pointAt(u), tt = path->tangentAt(u);
      v.f->position = {tp.x + tt.z * v.side, 0, tp.z - tt.x * v.side};
      v.f->rotation.y = std::atan2(tt.x, tt.z);
      v.f->scale.setScalar(smooth(u / 0.04f) * smooth((1 - u) / 0.05f));
      pose(v.f, {.walking = 1, .phase = (t - v.t0) * v.v * 4.2f});
    }

    int wk = 1 + (int)std::min(5.f, std::floor(clamp((t - cs - 0.3f) / 7.2f) * 6));
    L.set(week, "This term · week " + std::to_string(wk), "本学期 · 第" + std::to_string(wk) + "周");
  };
}

}  // namespace film
