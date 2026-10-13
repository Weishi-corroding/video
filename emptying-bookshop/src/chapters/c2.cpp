// Chapter 2 · Three Quick Reactions
// One sweeping cause (isolated), a copied fix (no concrete analysis), giving up after a week (static).
#include "chapters.h"

namespace film {

Update buildC2(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[1];

  W.addLight({{c("2a").s + 2, "overcast", 3}, {c("2j").s + 2, "day", 2.5f}});
  W.addCrowd({{c("2a").s + 1, 0.7f, 2}});
  W.addWarp(c("2h").s, c("2j").s + 0.4f, 0);

  W.addCam({
      {.t = T("2a", 0.6f), .d = 3.2f, .p = {19.2f, 4.0f, -12.6f}, .l = {15.4f, 1.5f, -21.0f}},
      {.t = T("2b", 0.6f), .d = 1.8f, .p = {22.5f, 5.4f, -9.0f}, .l = {16.2f, 1.7f, -19.6f}},
      {.t = T("2c", 2.4f), .d = 2.6f, .p = {31, 24, 3}, .l = {15, 0, -19}},
      {.t = T("2d", 1.0f), .d = 2.6f, .p = {8.5f, 8.2f, -2.5f}, .l = {19.8f, 3.4f, -20}},
      {.t = T("2e", 0.8f), .d = 2.4f, .p = {20.4f, 4.4f, -12.4f}, .l = {16.8f, 1.9f, -21.6f}},
      {.t = T("2g", -0.2f), .d = 2.6f, .p = {19.4f, 3.1f, -16.4f}, .l = {17.5f, 1.15f, -23.6f}},
      {.t = T("2j", 2.4f), .d = 3.0f, .p = {21, 6.5f, -6}, .l = {14.6f, 1.0f, -16}},
  });

  const float f = c("2f").s, j = c("2j").s;
  W.addJia({
      {.t = f + 0.6f, .x = 14.7f, .z = -21.2f, .y = 0, .ry = 0.5f},
      {.t = f + 1.6f, .x = 14.7f, .z = -22.6f, .y = 0.12f},
      {.t = f + 3.4f, .x = 17.4f, .z = -23.7f, .y = 0.12f, .ry = 0.25f},
      {.t = c("2g").s + 0.8f, .x = 17.4f, .z = -23.7f, .y = 0.12f, .ry = 0.25f, .nod = 0.5f},
      {.t = j + 0.6f, .x = 17.4f, .z = -23.7f, .y = 0.12f, .ry = 0.25f},
      {.t = j + 2.6f, .x = 14.7f, .z = -22.6f, .y = 0.12f},
      {.t = j + 5.2f, .x = 14.7f, .z = -20.2f, .y = 0, .ry = 0},
  });

  // ---- reaction 1: a heavy verdict that blocks the view ----
  Node* block = group();
  block->add(rbox(4.6f, 1.3f, 0.7f, mat(0xd8604f, {.rough = 0.7f}), 0.08f));
  Sign* bs = sign({.w = 4.3f, .h = 1.0f, .text = script::tr("NOBODY READS\nANYMORE", "现在没人\n读书了"), .fg = 0xfff7ec, .bg = 0xd8604f, .size = 0.78f});
  bs->mesh->position.z = 0.36f;
  block->add(bs->mesh);
  block->rotation.y = 0.5f;
  W.scene.add(block);
  const vec3 blockAt{17.6f, 0, -17.9f};

  // ---- reaction 2: another campus as a translucent miniature, and its banner ----
  // Tracing paper: a pale, see-through model of another campus with pencilled edges.
  auto holo = [](float o) { return standard({.color = 0xf6f1e4, .roughness = 0.95f, .emissive = 0xf6f1e4, .emissiveIntensity = 0.25f,
                                             .opacity = std::min(1.f, o * 2.2f), .transparent = true, .depthWrite = false, .paper = 1}); };
  auto edge = [] { return basic({.color = 0x5a5046, .opacity = 0.8f, .transparent = true}); };
  Node* ghost = group();
  ghost->position = {24.2f, 6.0f, -17.0f};
  auto addHolo = [&](float w, float h, float d, float x, float y, float z, float o = 0.22f) {
    Geometry* g = geo::box(w, h, d);
    Mesh* m = eng::mesh(g, holo(o));
    m->position = {x, y, z};
    Mesh* e = lines(geo::edges(*g), edge());
    e->position = m->position;
    ghost->add(m, e);
  };
  addHolo(7, 0.15f, 7, 0, 0, 0, 0.18f);
  addHolo(0.9f, 0.04f, 7, 0, 0.1f, 0, 0.5f);  // its main path
  addHolo(1.6f, 1.4f, 1.6f, -2.2f, 0.8f, -2.2f);
  addHolo(1.8f, 1.0f, 1.2f, 2.3f, 0.6f, 2.2f);
  addHolo(1.4f, 1.8f, 1.4f, -2.3f, 1.0f, 2.0f);
  addHolo(1.3f, 0.8f, 1.0f, 1.25f, 0.5f, -0.6f, 0.45f);  // its shop, right on the main path
  Rng crowdR(31);
  struct Dot { Mesh* m; float a, r, s; };
  std::vector<Dot> dots;
  for (int i = 0; i < 14; i++) {
    Mesh* dm = eng::mesh(facetedBall(0.12f, 0), standard({.color = 0xe8a060, .roughness = 0.9f, .opacity = 0.95f, .transparent = true, .paper = 1}));
    Dot dd{dm, 0, 0, 0};
    dd.a = (float)crowdR() * 6.28f;
    dd.r = 0.4f + (float)crowdR() * 0.7f;
    dd.s = 0.3f + (float)crowdR() * 0.5f;
    ghost->add(dm);
    dots.push_back(dd);
  }
  W.scene.add(ghost);

  Sign* banner = sign({.w = 3.4f, .h = 0.55f, .text = script::tr("HALF-PRICE WEEK", "半 价 周"), .fg = 0xffffff, .bg = 0xd8604f, .size = 0.62f});
  banner->material()->side = Side::Double;
  W.scene.add(banner->mesh);
  const vec3 bannerFrom{25.4f, 7.3f, -17.6f};
  const vec3 bannerTo{18.1f, 2.2f, -21.92f};
  const Curve* bannerArc = &arc(bannerFrom, bannerTo, 3.2f);

  // ---- labels ----
  const vec3 sh{17, 2.0f, -22.3f};
  L.add({.kind = Kind::Flaw, .at = blockAt, .lift = 1.6f, .en = "“Nobody reads anymore.”", .zh = "“现在没人读书了。”",
         .kicker = Pair{"Reaction 1", "反应一"}, .vis = [=](float t) { return win(t, c("2b").s + 1.0f, c("2d").s); }});
  L.add({.kind = Kind::Flaw, .cls = "stamp", .at = blockAt, .lift = 0.3f, .anchor = vec2{1, 0.5f}, .offset = {-150, 0}, .stem = false,
         .en = "Assumed, not checked", .zh = "臆断，未经查证", .vis = [=](float t) { return win(t, c("2c").s + 0.8f, c("2d").s); }});
  L.add({.kind = Kind::Flaw, .cls = "stamp", .at = sh, .lift = 1.4f, .anchor = vec2{0, 1}, .offset = {30, -10}, .stem = false,
         .en = "Seen in isolation", .zh = "孤立地看问题", .vis = [=](float t) { return win(t, c("2c").s + 3.6f, c("2d").s); }});
  L.add({.kind = Kind::Tag, .at = ghost, .lift = 2.2f, .en = "Another campus: its shop sits on the main path", .zh = "另一所学校：书店就在主路旁",
         .vis = [=](float t) { return win(t, c("2d").s + 1.4f, c("2e").s + 0.6f); }});
  L.add({.kind = Kind::Flaw, .at = bannerTo, .lift = 0.6f, .en = "Copy their half-price week", .zh = "照搬他们的“半价周”",
         .kicker = Pair{"Reaction 2", "反应二"}, .vis = [=](float t) { return win(t, c("2d").s + 5.6f, c("2e").s + 2.6f); }});
  L.add({.kind = Kind::Flaw, .cls = "stamp", .at = sh, .lift = 1.5f, .stem = false, .en = "Copied, not fitted to these conditions",
         .zh = "照搬，不合这里的条件", .vis = [=](float t) { return win(t, c("2e").s + 3.0f, f + 0.3f); }});
  L.add({.kind = Kind::Card, .cls = "quote", .screen = vec2{0.5f, 0.36f}, .en = "A concrete analysis of a concrete situation", .zh = "对具体情况作具体分析",
         .attr = Pair{"Lenin called it “the living soul of Marxism” (1920)", "列宁称之为“马克思主义的活的灵魂”（1920年）"},
         .vis = [=](float t) { return win(t, f + 0.3f, c("2g").s - 0.1f, 0.7f, 0.6f); }});
  L.add({.kind = Kind::Flaw, .at = W.jia, .lift = 1.5f, .en = "One quiet week: give up", .zh = "冷清了一周：放弃", .kicker = Pair{"Reaction 3", "反应三"},
         .vis = [=](float t) { return win(t, c("2g").s + 1.2f, c("2h").s + 1.2f); }});
  L.add({.kind = Kind::Tag, .cls = "snapframe", .screen = vec2{0.5f, 0.48f}, .stem = false,
         .vis = [=](float t) { return win(t, c("2h").s, j + 0.3f, 0.15f, 0.5f); }});
  L.add({.kind = Kind::Flaw, .cls = "stamp", .screen = vec2{0.5f, 1}, .anchor = vec2{0.5f, 1}, .en = "A snapshot, not a process", .zh = "只看一瞬，不看过程",
         .vis = [=](float t) { return win(t, c("2h").s + 1.6f, c("2i").s + 0.2f); }});
  L.add({.kind = Kind::Panel, .cls = "coral", .screen = vec2{0.5f, 0.42f}, .stem = false,
         .panel = PanelSpec{.h4 = {"Metaphysical thinking", "形而上学的思维方式"},
                            .chips = {{"Isolated", "孤立"}, {"Static", "静止"}, {"One-sided", "片面"}},
                            .foot = Pair{"“Metaphysical” in the Marxist sense: the opposite of dialectics.",
                                         "这里的“形而上学”取马克思主义哲学的含义：与辩证法相对立的思维方式。"}},
         .vis = [=](float t) { return win(t, c("2i").s + 0.3f, j + 0.4f); }});
  L.add({.kind = Kind::Principle, .at = W.jia, .lift = 1.55f, .en = "Dialectics starts by going out to look", .zh = "辩证法：从走出去看一看开始",
         .vis = [=](float t) { return win(t, j + 2.2f, ch.e + 2.5f); }});

  return [=, &W](float t) {
    // Reaction 1: the block drops, then lifts away.
    float drop = settle(ramp(t, c("2b").s + 0.3f, c("2b").s + 1.1f, linear));
    float lift = ramp(t, c("2d").s - 0.4f, c("2d").s + 0.8f);
    bool shown = t > c("2b").s + 0.3f && t < c("2d").s + 0.8f;
    block->visible = shown;
    if (shown) {
      block->position = {blockAt.x, lerp(14, 0.65f, drop) + lift * 8, blockAt.z};
      block->scale.setScalar(1 - lift * 0.6f);
    }
    // Isolation: everything outside the shop dims and greys.
    float iso = ramp(t, c("2c").s + 0.6f, c("2c").s + 2.2f) * (1 - ramp(t, c("2d").s - 0.6f, c("2d").s + 0.4f));
    if (iso > 0) {
      W.fx.focus = vec3{16.8f, 1.2f, -20.5f};
      W.fx.focusR = 0.17f;
      W.fx.focusAmt = iso * 0.9f;
    }

    // Reaction 2: the other campus appears; its banner flies over and is hung on Jia's shop.
    float gIn = backOut(ramp(t, c("2d").s + 0.6f, c("2d").s + 1.6f, linear));
    float gOut = 1 - ramp(t, c("2e").s + 1.0f, c("2e").s + 2.0f);
    scaleIn(ghost, gIn * gOut);
    ghost->rotation.y = -0.4f + std::sin(t * 0.3f) * 0.05f;
    for (size_t i = 0; i < dots.size(); i++) {
      const Dot& d = dots[i];
      float a = d.a + t * d.s;
      d.m->position = {1.25f + std::cos(a) * d.r * 1.4f, 0.2f, -0.6f + 0.9f + std::sin(a) * d.r * 0.6f + (i % 2) * 0.2f};
    }
    float fly = ramp(t, c("2d").s + 4.2f, c("2d").s + 6.0f);
    bool bannerOn = t > c("2d").s + 1.5f && t < c("2g").s + 1.2f;
    banner->mesh->visible = bannerOn;
    if (bannerOn) {
      banner->mesh->position = bannerArc->point(fly);
      banner->mesh->rotation = {0, lerp(-0.4f, 0, fly), std::sin(fly * PI) * 0.25f + 0.04f * fly};
      banner->mesh->scale.setScalar(lerp(0.35f, 1, fly));
      fade(banner->mesh, 1 - ramp(t, c("2g").s, c("2g").s + 1.2f));
    }

    // Reaction 3: the snapshot. A flash, colour drains, and the crowd freezes (see addWarp above).
    W.fx.flash = std::max(W.fx.flash, pulse(t, c("2h").s - 0.05f, 0.5f) * 0.8f);
    float drain = win(t, c("2h").s + 0.05f, j + 0.8f, 0.3f, 1.2f);
    W.fx.sat = std::min(W.fx.sat, 1 - 0.88f * drain);
  };
}

}  // namespace film
