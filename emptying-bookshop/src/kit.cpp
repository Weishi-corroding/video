#include "kit.h"

#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>

namespace film {

// ---------- easing ----------
float linear(float x) { return x; }
float smooth(float x) { x = clamp(x); return x * x * (3 - 2 * x); }
float smoother(float x) { x = clamp(x); return x * x * x * (x * (x * 6 - 15) + 10); }
float easeOut(float x) { return 1 - std::pow(1 - clamp(x), 3.f); }
float easeIn(float x) { return std::pow(clamp(x), 3.f); }
float backOut(float x) {
  x = clamp(x);
  const float s = 1.5f;
  return 1 + (s + 1) * std::pow(x - 1, 3.f) + s * std::pow(x - 1, 2.f);
}
float settle(float x) {
  x = clamp(x);
  return 1 - std::pow(1 - x, 4.f) * std::cos(x * PI * 1.5f);
}
float ramp(float t, float a, float b, Ease f) { return f((t - a) / (b - a)); }
float win(float t, float a, float b, float fin, float fout) { return std::min(ramp(t, a, a + fin), 1 - ramp(t, b - fout, b)); }
float pulse(float t, float a, float d) {
  float x = (t - a) / d;
  return x <= 0 || x >= 1 ? 0 : std::sin(PI * x);
}

double Rng::operator()() {
  a = a + 0x6d2b79f5u;
  uint32_t t = (a ^ (a >> 15)) * (1u | a);
  t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
  return (double)(t ^ (t >> 14)) / 4294967296.0;
}

// ---------- materials ----------
Material* matS(uint32_t color, float rough, float metal) {
  static std::map<std::tuple<uint32_t, float, float>, Material*> shared;
  auto key = std::make_tuple(color, rough, metal);
  auto it = shared.find(key);
  if (it != shared.end()) return it->second;
  Material* m = standard({.color = color, .roughness = rough, .metalness = metal});
  shared[key] = m;
  return m;
}

Material* mat(uint32_t color, const MatOpts& o) {
  return standard({.color = color, .roughness = o.rough, .metalness = o.metal, .emissive = o.emissive, .emissiveIntensity = o.ei,
                   .opacity = o.opacity, .transparent = o.transparent, .depthWrite = o.depthWrite, .side = o.side});
}

Material* glow(uint32_t color, float opacity) {
  return basic({.color = color, .opacity = opacity, .transparent = opacity < 1});
}

// ---------- meshes ----------
Mesh* prop(Geometry* g, Material* m, bool cast, bool receive) {
  Mesh* x = eng::mesh(g, m);
  x->castShadow = cast;
  x->receiveShadow = receive;
  return x;
}

namespace {
Geometry* roundedBoxGeo(float w, float h, float d, float r) {
  static std::map<std::tuple<float, float, float, float>, Geometry*> cache;
  auto key = std::make_tuple(w, h, d, r);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  return cache[key] = geo::roundedBox(w, h, d, 2, r);
}
}  // namespace

Mesh* rbox(float w, float h, float d, Material* m, float r) {
  r = std::min({r, w / 2 - 0.001f, h / 2 - 0.001f, d / 2 - 0.001f});
  return prop(roundedBoxGeo(w, h, d, r), m);
}
Mesh* rbox(float w, float h, float d, uint32_t color, float r) { return rbox(w, h, d, matS(color), r); }

Geometry* cylinderGeo(float rTop, float rBottom, float h, int radial, int hs, bool open) {
  static std::map<std::tuple<float, float, float, int, int, bool>, Geometry*> cache;
  auto key = std::make_tuple(rTop, rBottom, h, radial, hs, open);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  return cache[key] = geo::cylinder(rTop, rBottom, h, radial, hs, open);
}

Geometry* sphereGeo(float r, int ws, int hs, float phiStart, float phiLen, float thetaStart, float thetaLen) {
  static std::map<std::tuple<float, int, int, float, float, float, float>, Geometry*> cache;
  auto key = std::make_tuple(r, ws, hs, phiStart, phiLen, thetaStart, thetaLen);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  return cache[key] = geo::sphere(r, ws, hs, phiStart, phiLen, thetaStart, thetaLen);
}

void fade(Node* obj, float o) {
  obj->visible = o > 0.002f;
  if (!obj->visible) return;
  obj->traverse([&](Node* n) {
    if (n->kind == NodeKind::Group || n->kind == NodeKind::PointLight) return;
    Material* m = static_cast<Mesh*>(n)->material;
    if (!m) return;
    float& base = m->user[0];
    if (base < 0) base = m->opacity;
    float target = base * o;
    m->opacity = target;
    m->transparent = target < 0.999f || base < 1;
  });
}

Node* ownMaterials(Node* obj) {
  obj->traverse([](Node* n) {
    if (n->kind == NodeKind::Group || n->kind == NodeKind::PointLight) return;
    Mesh* m = static_cast<Mesh*>(n);
    if (m->material) m->material = m->material->clone();
  });
  return obj;
}

void scaleIn(Node* obj, float s) {
  obj->visible = s > 0.002f;
  obj->scale.setScalar(std::max(s, 0.0001f));
}

// ---------- canvas text ----------
void CanvasTex::redraw() {
  canvas.clear();
  draw(canvas, canvas.width, canvas.height);
  canvas.toTexture(tex);
}

CanvasTex* canvasTexture(int w, int h, std::function<void(Canvas&, int, int)> draw) {
  static std::vector<std::unique_ptr<CanvasTex>> all;
  all.push_back(std::make_unique<CanvasTex>());
  CanvasTex* ct = all.back().get();
  ct->canvas.resize(w, h);
  ct->tex = newTexture(w, h);
  ct->draw = std::move(draw);
  ct->redraw();
  return ct;
}

float measureSans(const std::string& s, float size, float weight) {
  TextStyle st;
  st.size = size;
  st.weight = weight;
  return textWidth(s, st);
}

void drawCentredLines(Canvas& cv, const std::vector<std::string>& lines, float cx, float cy, float fs, float weight, RGBA color, float lineStep) {
  TextStyle st;
  st.size = fs;
  st.weight = weight;
  st.color = color;
  st.lineHeight = 1;
  float a = fontAscent(st), d = fontDescent(st);
  // A 'middle' baseline: half way down the em box, with the font's ascent and descent scaled to one em.
  float middle = fs * (a - d) / (2 * (a + d));
  int n = (int)lines.size();
  for (int i = 0; i < n; i++) {
    TextBlock b = layoutText(lines[i], st, 0);
    float y = cy + (i - (n - 1) / 2.f) * lineStep;
    // drawText places a block by its top; move it so the baseline lands on y + middle.
    drawText(cv, b, cx - b.width / 2, y + middle - b.firstBaseline);
  }
}

static std::vector<std::string> splitLines(const std::string& s) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string line;
  while (std::getline(ss, line, '\n')) out.push_back(line);
  if (out.empty()) out.push_back("");
  return out;
}

Sign* sign(const SignOpts& o) {
  static std::vector<std::unique_ptr<Sign>> all;
  all.push_back(std::make_unique<Sign>());
  Sign* s = all.back().get();
  s->text = o.text;
  s->fg = o.fg;
  s->bg = o.bg;
  s->border = o.border;
  s->size = o.size;
  s->weight = o.weight;
  int cw = (int)std::lround(o.px * (o.w / o.h)), ch = o.px;
  s->ct = canvasTexture(cw, ch, [s](Canvas& cv, int W, int H) {
    cv.fillRect(0, 0, (float)W, (float)H, rgba(s->bg));
    if (s->border) cv.strokeRect(H * 0.05f, H * 0.05f, W - H * 0.1f, H - H * 0.1f, H * 0.06f, rgba(*s->border));
    std::vector<std::string> lines = splitLines(s->text);
    float fs = H * s->size / lines.size();
    float widest = 0;
    for (auto& l : lines) widest = std::max(widest, measureSans(l, fs, s->weight));
    if (widest > W * 0.9f) fs *= (W * 0.9f) / widest;
    drawCentredLines(cv, lines, W / 2.f, H / 2.f, fs, s->weight, rgba(s->fg), fs * 1.15f);
  });
  Material* m = o.emissive ? basic({.transparent = true, .map = s->ct->tex}) : standard({.roughness = 0.9f, .transparent = true, .map = s->ct->tex});
  s->mesh = eng::mesh(geo::plane(o.w, o.h), m);
  return s;
}

void Sign::set(const std::optional<std::string>& t, std::optional<uint32_t> fgNew, std::optional<uint32_t> bgNew) {
  std::string nt = t ? *t : text;
  uint32_t nf = fgNew ? *fgNew : fg, nb = bgNew ? *bgNew : bg;
  if (nt == text && nf == fg && nb == bg) return;
  text = nt;
  fg = nf;
  bg = nb;
  ct->redraw();
}

// ---------- people ----------
namespace FIG {
Geometry* bodyGeo() {
  static Geometry* g = geo::capsule(0.24f, 0.36f, 4, 12);
  return g;
}
Geometry* headGeo() {
  static Geometry* g = sphereGeo(0.19f, 18, 14);
  return g;
}
}  // namespace FIG

Figure figure(const FigOpts& o) {
  Figure f;
  f.root = group();
  f.body = prop(FIG::bodyGeo(), mat(o.shirt, {.rough = 0.8f}));
  f.body->position.y = FIG::bodyY;
  f.headPivot = group();
  f.headPivot->position.y = FIG::headY;
  Mesh* head = prop(FIG::headGeo(), mat(o.skin, {.rough = 0.7f}));
  f.headPivot->add(head);
  if (o.hair) {
    Mesh* h = prop(sphereGeo(0.195f, 18, 10, 0, PI * 2, 0, PI * 0.55f), mat(*o.hair, {.rough = 0.9f}));
    h->rotation.x = -0.25f;
    h->position.z = -0.01f;
    f.headPivot->add(h);
  }
  f.root->add(f.body, f.headPivot);
  if (o.apron) {
    Mesh* a = rbox(0.4f, 0.5f, 0.05f, mat(PAL::apron, {.rough = 0.9f}), 0.02f);
    a->position = {0, 0.42f, 0.22f};
    Mesh* strap = rbox(0.06f, 0.2f, 0.03f, mat(PAL::apron), 0.01f);
    strap->position = {0, 0.74f, 0.2f};
    f.root->add(a, strap);
  }
  if (o.notebook) {
    f.book = rbox(0.2f, 0.26f, 0.04f, mat(0xf7f3ea), 0.01f);
    f.book->position = {0.3f, 0.5f, 0.12f};
    f.book->rotation = {-0.5f, 0, 0.15f};
    f.root->add(f.book);
  }
  return f;
}

void pose(const Figure& f, const PoseOpts& p) {
  float bob = std::fabs(std::sin(p.phase)) * 0.06f * p.walking;
  f.body->position.y = FIG::bodyY + bob;
  f.body->rotation.z = std::sin(p.phase) * 0.06f * p.walking;
  f.headPivot->position.y = FIG::headY + bob;
  f.headPivot->rotation.y = p.look;
  f.headPivot->rotation.x = p.nod;
}

Figure seated(const FigOpts& o) {
  Figure f = figure(o);
  f.body->scale.y = 0.8f;
  f.body->position.y = FIG::bodyY * 0.8f + 0.28f;
  f.headPivot->position.y = FIG::headY * 0.8f + 0.28f;
  f.isSeated = true;
  return f;
}

// ---------- furniture and small props ----------
static const std::vector<uint32_t> bookColors = {0xb85c4a, 0x4f6f9a, 0xd9b45a, 0x6f9a7a, 0x8a6fae, 0xe0ddd2, 0x9b4f3c, 0x3f5a6e, 0xc9a27a};

Node* shelf(float w, float h, float d, const ShelfOpts& o) {
  static Geometry* unitBox = geo::box(1, 1, 1);
  static Material* bookMat = standard({.roughness = 0.9f});
  Node* g = group();
  Rng r(o.seed);
  Material* fm = matS(o.frame, 0.8f);
  const float t = 0.05f;
  g->add(place(rbox(t, h, d, fm, 0.01f), -w / 2, h / 2, 0));
  g->add(place(rbox(t, h, d, fm, 0.01f), w / 2, h / 2, 0));
  g->add(place(rbox(w, t, d, fm, 0.01f), 0, h - t / 2, 0));
  g->add(place(rbox(w, 0.02f, d, fm, 0.005f), 0, h / 2, -d / 2 + 0.01f));
  float rowH = (h - t) / o.rows;
  // All the books on one shelf are a single instanced mesh.
  std::vector<std::pair<mat4, uint32_t>> books;
  for (int i = 0; i < o.rows; i++) {
    float y = i * rowH;
    g->add(place(rbox(w, t, d, fm, 0.01f), 0, y + t / 2, 0));
    float x = -w / 2 + t;
    while (x < w / 2 - t - 0.06f) {
      float bw = 0.05f + r() * 0.05f;
      float bh = rowH * (0.55f + r() * 0.3f);
      if (o.messy && r() < 0.25f) { x += bw * 1.5f; continue; }
      vec3 pos{x + bw / 2, y + t + bh / 2, 0.01f};
      vec3 rot{0, 0, 0};
      if (o.messy && r() < 0.2f) { rot.z = 0.35f; pos.x += 0.03f; }
      books.push_back({mat4::compose(pos, quat::euler(rot), {bw, bh, d * 0.8f}), pick(r, bookColors)});
      x += bw + 0.008f;
    }
  }
  InstancedMesh* inst = instanced(unitBox, bookMat, (int)books.size());
  for (size_t k = 0; k < books.size(); k++) {
    inst->matrices[k] = books[k].first;
    inst->setColor((int)k, Color::hex(books[k].second));
  }
  inst->receiveShadow = true;
  g->add(inst);
  return g;
}

Node* table(float w, float d, float h, uint32_t top) {
  Node* g = group();
  g->add(place(rbox(w, 0.06f, d, matS(top, 0.7f), 0.02f), 0, h, 0));
  Material* lm = matS(PAL::woodDark);
  for (auto [x, z] : std::vector<std::pair<float, float>>{{-1, -1}, {1, -1}, {-1, 1}, {1, 1}})
    g->add(place(prop(cylinderGeo(0.03f, 0.03f, h, 6), lm), x * (w / 2 - 0.08f), h / 2, z * (d / 2 - 0.08f)));
  return g;
}

Node* roundTable(float r, float h) {
  Node* g = group();
  g->add(place(prop(cylinderGeo(r, r, 0.06f, 24), matS(PAL::wood, 0.7f)), 0, h, 0));
  g->add(place(prop(cylinderGeo(0.05f, 0.05f, h, 8), matS(PAL::woodDark)), 0, h / 2, 0));
  g->add(place(prop(cylinderGeo(0.25f, 0.25f, 0.04f, 16), matS(PAL::woodDark)), 0, 0.02f, 0));
  return g;
}

Node* stool() {
  Node* g = group();
  g->add(place(prop(cylinderGeo(0.18f, 0.18f, 0.05f, 14), matS(PAL::woodDark)), 0, 0.42f, 0));
  g->add(place(prop(cylinderGeo(0.04f, 0.05f, 0.42f, 6), matS(PAL::woodDark)), 0, 0.21f, 0));
  return g;
}

Node* sofa(uint32_t color) {
  Node* g = group();
  Material* m = matS(color, 0.95f);
  g->add(place(rbox(1.6f, 0.35f, 0.7f, m, 0.1f), 0, 0.25f, 0));
  g->add(place(rbox(1.6f, 0.5f, 0.2f, m, 0.08f), 0, 0.55f, -0.27f));
  g->add(place(rbox(0.2f, 0.45f, 0.7f, m, 0.08f), -0.8f, 0.4f, 0));
  g->add(place(rbox(0.2f, 0.45f, 0.7f, m, 0.08f), 0.8f, 0.4f, 0));
  return g;
}

Node* counter(float w, uint32_t color) {
  Node* g = group();
  g->add(place(rbox(w, 0.9f, 0.7f, matS(color, 0.8f), 0.04f), 0, 0.45f, 0));
  g->add(place(rbox(w + 0.1f, 0.06f, 0.8f, matS(0xd8c7a8, 0.6f), 0.02f), 0, 0.93f, 0));
  return g;
}

LampPost lampPost(float h) {
  Node* g = group();
  g->add(place(prop(cylinderGeo(0.05f, 0.07f, h, 8), matS(0x3b4250, 0.6f, 0.3f)), 0, h / 2, 0));
  Mesh* bulb = eng::mesh(sphereGeo(0.16f, 14, 10), standard({.color = 0xfff1d6, .emissive = 0xffd08a, .emissiveIntensity = 0}));
  bulb->position.y = h + 0.05f;
  g->add(bulb);
  return {g, bulb};
}

Node* tree(Rng& r, float s) {
  Node* g = group();
  float trunkH = 0.9f * s;
  g->add(place(prop(cylinderGeo(0.08f * s, 0.12f * s, trunkH, 6), matS(0x6b4f3a)), 0, trunkH / 2, 0));
  uint32_t green = pick(r, std::vector<uint32_t>{0x6f9a62, 0x5d8a58, 0x7fa86a, 0x8bb070});
  if (r() < 0.45f) {
    g->add(place(prop(geo::cone(0.75f * s, 1.9f * s, 8), matS(green, 0.9f)), 0, trunkH + 0.8f * s, 0));
  } else {
    g->add(place(prop(geo::icosahedron(0.85f * s, 1), matS(green, 0.9f)), 0, trunkH + 0.6f * s, 0));
    g->add(place(prop(geo::icosahedron(0.55f * s, 1), matS(green, 0.9f)), 0.35f * s, trunkH + 1.15f * s, 0.1f * s));
  }
  return g;
}

Mesh* hedge(float len, float h, float d) { return rbox(len, h, d, mat(0x5f8a55, {.rough = 0.95f}), 0.25f); }

// ---------- lines and threads ----------
const Curve& keepCurve(Curve* c) {
  static std::vector<std::unique_ptr<Curve>> all;
  all.emplace_back(c);
  return *c;
}

const Curve& arc(vec3 a, vec3 b, float lift) {
  vec3 mid = (a + b) * 0.5f;
  mid.y += lift;
  return keepCurve(new QuadraticBezier(a, mid, b));
}

void GrowTube::grow(float p) const {
  p = clamp(p);
  mesh->visible = p > 0.001f;
  mesh->geometry->setDrawRange(0, (uint32_t)std::ceil(segs * p) * radial * 6);
}

GrowTube growTube(const Curve& curve, float radius, Material* m, int segs, int radial) {
  GrowTube t;
  // Each tube draws its own range, so it needs its own geometry.
  t.mesh = prop(geo::tube(curve, segs, radius, radial, false), m, false, false);
  t.segs = segs;
  t.radial = radial;
  t.grow(0);
  return t;
}

Mesh* disc(float radius, uint32_t color, float opacity) {
  Mesh* m = eng::mesh(geo::circle(radius, 40), basic({.color = color, .opacity = opacity, .transparent = true, .depthWrite = false}));
  m->rotation.x = -PI / 2;
  return m;
}

Mesh* ringMesh(float r, float tube, uint32_t color, float opacity) {
  Mesh* m = eng::mesh(geo::torus(r, tube, 8, 64), basic({.color = color, .opacity = opacity, .transparent = true, .depthWrite = false}));
  m->rotation.x = -PI / 2;
  return m;
}

const Curve& polyPath(const std::vector<std::pair<float, float>>& points, float y) {
  std::vector<vec3> pts;
  for (auto [x, z] : points) pts.push_back({x, y, z});
  auto* c = new CatmullRom(pts, false, 0.05f);
  c->arcLengthDivisions = 400;
  return keepCurve(c);
}

}  // namespace film
