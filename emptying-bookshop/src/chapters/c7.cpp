// Chapter 7 · Your Own Bookshop
// Jia's turning points pinned to the campus, a summary, the question for the viewer, and one action.
#include "chapters.h"

namespace film {

namespace {
struct Step {
  vec3 at;
  const char* en;
  const char* p;
  const char* zh;
  const char* pz;
};
const Step STEPS[6] = {
    {{-5.2f, 0, -6.4f}, "Look before judging", "Start from facts", "先调查，再判断", "从实际出发"},
    {{4.0f, 0, 0.6f}, "Trace the connections", "Universal connection", "理清各种联系", "普遍联系"},
    {{16.2f, 3.5f, -25.4f}, "Find what shapes the rest", "Principal contradiction", "找到制约全局的问题", "主要矛盾"},
    {{19.6f, 0, -19.6f}, "Test ideas in practice", "Practice tests knowledge", "在实践中检验想法", "实践检验认识"},
    {{11.3f, 1.8f, -18.7f}, "Let small changes add up", "Quantity → quality", "让细小的变化积累", "量变到质变"},
    {{17.8f, 7.0f, -25}, "Expect the process to keep moving", "Negation of the negation", "预料事情还会继续发展", "否定之否定"},
};
}  // namespace

Update buildC7(Ctx& ctx) {
  World& W = ctx.W;
  Labels& L = ctx.L;
  const Chapter ch = script::chapters()[6];
  const float DURATION = script::duration();

  W.addLight({{T("7a", 1.0f), "dusk", 3}, {T("7d", 3.0f), "golden", 5}});
  W.addCrowd({{T("7a", 0.5f), 0.8f, 3}});
  W.addCam({
      {.t = T("7a", 1.2f), .d = 3.6f, .p = {32, 42, 40}, .l = {-7, 0, -9}, .fov = 42, .push = 0.02f},
      {.t = T("7d", 6.0f), .d = 6.0f, .p = {38, 36, 26}, .l = {-5, 0, -11}, .fov = 42},
      {.t = T("7e", 0.8f), .d = 3.4f, .p = {23.5f, 6.6f, -9}, .l = {17, 1.7f, -21.2f}},
      {.t = ch.e, .d = 6.5f, .p = {31, 17, 2}, .l = {15.5f, 2.5f, -20}},
  });
  W.addJia({{.t = T("7e", 0.4f), .x = 17.8f, .z = -18.0f, .y = 0, .ry = 0.5f}});

  // ---- numbered pins ----
  const float times[6] = {T("7b", 0.2f), T("7b", 2.4f), T("7c", 0.3f), T("7c", 2.4f), T("7c", 4.4f), T("7c", 6.4f)};
  struct Pin { Node* g; Mesh* head; float t0; };
  std::vector<Pin> pins;
  for (int i = 0; i < 6; i++) {
    Node* g = group();
    Mesh* stem = eng::mesh(facetedCylinder(0.06f, 0.06f, 2.6f, 6), standard({.color = 0x8a6646, .roughness = 0.9f, .paper = 1}));
    stem->position.y = 1.3f;
    Mesh* head = eng::mesh(facetedBall(0.55f, 1), standard({.color = 0xf2c25a, .roughness = 0.85f, .emissive = 0xffb02e, .emissiveIntensity = 0.5f, .paper = 1}));
    head->position.y = 2.9f;
    CanvasTex* num = canvasTexture(128, 128, [i](Canvas& cv, int w, int h) {
      drawCentredLines(cv, {std::to_string(i + 1)}, w / 2.f, h / 2.f + 4, 92, 800, rgba(0x1a1406), 92);
    });
    Material* sm = basic({.transparent = true, .depthTest = false, .map = num->tex});
    Mesh* sp = sprite(sm);
    sp->position.y = 2.9f;
    sp->scale.setScalar(0.9f);
    sp->renderOrder = 5;
    Geometry* baseGeo = geo::circle(0.7f, 28);
    baseGeo->rotateX(-PI / 2);
    Mesh* base = eng::mesh(baseGeo, basic({.color = 0xe8c060, .opacity = 0.45f, .transparent = true}));
    base->position.y = 0.03f;
    g->add(stem, head, sp, base);
    g->position = STEPS[i].at;
    W.scene.add(g);
    pins.push_back({g, head, times[i]});
  }

  // ---- summary panel ----
  PanelSpec panel;
  panel.h4 = {"Jia's turning points", "小嘉的转折点"};
  for (const Step& s : STEPS) panel.items.push_back({s.en, s.p, std::string(s.zh) + "——" + s.pz});
  // Each item lights up when its pin appears (a 0.4 s transition, as in the player).
  std::vector<float> t0s(times, times + 6);
  panel.itemOpacity = [t0s](int i, float t) { return lerp(0.28f, 1.f, smooth((t - t0s[i]) / 0.4f)); };
  L.add({.kind = Kind::Panel, .cls = "gold summary", .screen = vec2{0.03f, 0.46f}, .anchor = vec2{0, 0.5f}, .stem = false, .panel = panel,
         .vis = [=](float t) { return win(t, T("7a", 1.6f), c("7e").s + 0.2f, 0.8f, 0.8f); }});

  L.add({.kind = Kind::Card, .cls = "big", .screen = vec2{0.5f, 0.42f}, .en = "So: what’s the emptying bookshop in your own life?",
         .zh = "那么，你生活中那家“日渐冷清的书店”是什么？", .vis = [=](float t) { return win(t, T("7e", 0.3f), c("7f").s + 0.2f, 0.8f, 0.6f); }});
  L.add({.kind = Kind::Card, .cls = "gold", .screen = vec2{0.5f, 0.42f}, .en = "Pick one problem. Before you judge it, ask three people who live with it.",
         .zh = "选一个问题。在下判断之前，先问问三个身处其中的人。", .kicker = Pair{"This week", "本周试一试"},
         .vis = [=](float t) { return win(t, T("7f", 0.3f), c("7f").e + 1.2f, 0.7f, 0.8f); }});
  L.add({.kind = Kind::Title, .screen = vec2{0.5f, 0.45f}, .stem = false, .en = "The Emptying Bookshop", .zh = "日渐冷清的书店",
         .sub = Pair{"Jia, the shop and the campus are fictional. The principles are tools: test them in practice.",
                     "小嘉、书店和校园都是虚构的。这些原理是工具，要在实践中检验。"},
         .vis = [=](float t) { return win(t, c("7f").e + 1.6f, DURATION + 10, 1.2f, 0.1f); }});

  return [=, &W](float t) {
    bool live = t > ch.s - 0.5f;
    for (size_t i = 0; i < pins.size(); i++) {
      const Pin& p = pins[i];
      float k = live ? backOut(ramp(t, p.t0, p.t0 + 0.7f, linear)) : 0;
      float out = 1 - ramp(t, T("7e", 0.0f), T("7e", 1.6f));
      scaleIn(p.g, k * out * 1.2f);
      p.head->material->emissiveIntensity = 0.5f + 0.5f * std::max(0.f, std::sin(t * 2.4f - i * 0.6f)) * ramp(t, T("7d"), T("7d", 1));
    }
    W.fx.fade = std::max({W.fx.fade, ramp(t, c("7f").e + 0.6f, c("7f").e + 2.4f) * 0.82f, ramp(t, DURATION - 1.6f, DURATION - 0.1f)});
  };
}

}  // namespace film
