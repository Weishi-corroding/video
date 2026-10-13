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
  Material* m = standard({.color = color, .roughness = rough, .metalness = metal, .paper = 1});
  shared[key] = m;
  return m;
}

Material* mat(uint32_t color, const MatOpts& o) {
  return standard({.color = color, .roughness = o.rough, .metalness = o.metal, .emissive = o.emissive, .emissiveIntensity = o.ei,
                   .opacity = o.opacity, .transparent = o.transparent, .depthWrite = o.depthWrite, .side = o.side, .paper = o.paper});
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

Geometry* facetedCylinder(float rTop, float rBottom, float h, int radial) {
  static std::map<std::tuple<float, float, float, int>, Geometry*> cache;
  auto key = std::make_tuple(rTop, rBottom, h, radial);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  return cache[key] = geo::flat(*geo::cylinder(rTop, rBottom, h, radial, 1, false));
}

Geometry* facetedCone(float r, float h, int radial) {
  static std::map<std::tuple<float, float, int>, Geometry*> cache;
  auto key = std::make_tuple(r, h, radial);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  return cache[key] = geo::flat(*geo::cone(r, h, radial));
}

Geometry* facetedBall(float r, int detail) {
  static std::map<std::pair<float, int>, Geometry*> cache;
  auto key = std::make_pair(r, detail);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  return cache[key] = geo::flat(*geo::icosahedron(r, detail));
}

Node* card(const std::vector<vec2>& outline, float depth, Material* face, Material* edge) {
  Node* g = group();
  g->add(prop(geo::extrude(outline, depth, geo::Part::Faces), face));
  g->add(prop(geo::extrude(outline, depth, geo::Part::Edges), edge ? edge : face));
  return g;
}

std::vector<vec2> cloudOutline(float w, float h, uint32_t seed) {
  // A flat bottom and a row of round bumps along the top, like a cloud cut with scissors.
  Rng r(seed);
  std::vector<vec2> pts;
  float x0 = -w / 2, x1 = w / 2, y0 = -h / 2;
  int bumps = 3 + (int)(r() * 2);
  std::vector<float> xs, rs;
  for (int i = 0; i < bumps; i++) {
    float u = (i + 0.5f) / bumps;
    float rad = h * (0.42f + 0.32f * (float)r()) * (1 - std::fabs(u - 0.5f) * 0.9f);
    xs.push_back(x0 + w * u);
    rs.push_back(std::max(rad, h * 0.22f));
  }
  pts.push_back({x1, y0});
  // Trace the upper envelope of the circles from right to left.
  const int N = 72;
  for (int k = 0; k <= N; k++) {
    float x = x1 - (x1 - x0) * k / N;
    float top = y0 + h * 0.18f;
    for (int i = 0; i < bumps; i++) {
      float dx = x - xs[i];
      if (std::fabs(dx) < rs[i]) top = std::max(top, y0 + h * 0.3f + std::sqrt(rs[i] * rs[i] - dx * dx));
    }
    pts.push_back({x, std::min(top, y0 + h)});
  }
  pts.push_back({x0, y0});
  return pts;
}

std::vector<vec2> polygonOutline(int sides, float r, float inner, float rot) {
  std::vector<vec2> pts;
  int n = inner > 0 ? sides * 2 : sides;
  for (int i = 0; i < n; i++) {
    float a = rot + (float)i / n * 2 * PI;
    float rr = inner > 0 && i % 2 ? inner : r;
    pts.push_back({std::cos(a) * rr, std::sin(a) * rr});
  }
  return pts;
}

Texture* stripeTexture(uint32_t a, uint32_t b, int count, bool vertical, float lineFrac) {
  const int N = 256;
  Texture* t = newTexture(N, N);
  RGBA ca = rgba(a), cb = rgba(b);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (vertical ? x : y) / (float)N * count;
      float f = u - std::floor(u);
      // A soft edge between bands, a little over a texel wide.
      float k = clamp((f - lineFrac) * N / count * 0.8f + 0.5f, 0.f, 1.f) * clamp((1 - f) * N / count * 0.8f + 0.5f, 0.f, 1.f);
      RGBA c{ca.r + (cb.r - ca.r) * k, ca.g + (cb.g - ca.g) * k, ca.b + (cb.b - ca.b) * k, 1};
      uint8_t* p = &t->pixels[((size_t)y * N + x) * 4];
      p[0] = (uint8_t)std::lround(c.r * 255);
      p[1] = (uint8_t)std::lround(c.g * 255);
      p[2] = (uint8_t)std::lround(c.b * 255);
      p[3] = 255;
    }
  return t;
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
  Material* m = o.emissive ? basic({.map = s->ct->tex}) : standard({.roughness = 0.9f, .map = s->ct->tex, .paper = 0.7f});
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
Geometry* legsGeo() {
  static Geometry* g = facetedCylinder(0.15f, 0.165f, 0.26f, 7);
  return g;
}
Geometry* bodyGeo() {
  static Geometry* g = facetedCylinder(0.165f, 0.235f, 0.56f, 8);
  return g;
}
Geometry* headGeo() {
  static Geometry* g = sphereGeo(0.19f, 20, 14);
  return g;
}
Geometry* hairGeo() {
  // A cap over the back and top of the head, tipped back a little.
  static Geometry* g = [] {
    Geometry* cap = geo::sphere(0.198f, 20, 10, 0, PI * 2, 0, PI * 0.56f);
    return geo::merge({{cap, mat4::compose({0, 0.0f, -0.012f}, quat::euler({-0.3f, 0, 0}), {1, 1, 1})}});
  }();
  return g;
}
Geometry* eyesGeo() {
  // Two small dots on the front of the head (relative to the head's centre).
  static Geometry* g = [] {
    Geometry* dot = geo::sphere(0.024f, 8, 6);
    return geo::merge({{dot, mat4::compose({-0.068f, 0.025f, 0.172f}, {}, {1, 1, 0.5f})},
                       {dot, mat4::compose({0.068f, 0.025f, 0.172f}, {}, {1, 1, 0.5f})}});
  }();
  return g;
}
Geometry* armGeo() {
  static Geometry* g = [] {
    Geometry* a = geo::flat(*geo::cylinder(0.05f, 0.045f, 0.4f, 5, 1, false));
    return geo::merge({{a, mat4::translation({0, -0.2f, 0})}});
  }();
  return g;
}
}  // namespace FIG

Figure figure(const FigOpts& o) {
  Figure f;
  f.root = group();
  Material* shirt = mat(o.shirt, {.rough = 0.85f});
  Material* skin = mat(o.skin, {.rough = 0.75f});
  f.legs = prop(FIG::legsGeo(), mat(o.trousers ? *o.trousers : PAL::trousers[(o.shirt >> 3) % PAL::trousers.size()], {.rough = 0.9f}));
  f.legs->position.y = FIG::legsY;
  f.bodyPivot = group();
  f.body = prop(FIG::bodyGeo(), shirt);
  f.body->position.y = FIG::bodyY;
  f.headPivot = group();
  f.headPivot->position.y = FIG::headY;
  f.headPivot->add(prop(FIG::headGeo(), skin));
  f.headPivot->add(prop(FIG::eyesGeo(), matS(PAL::ink, 0.6f), false, false));
  if (o.hair) f.headPivot->add(prop(FIG::hairGeo(), mat(*o.hair, {.rough = 0.95f})));
  f.armL = group();
  f.armR = group();
  for (Node* arm : {f.armL, f.armR}) {
    bool left = arm == f.armL;
    arm->position = {left ? -FIG::shoulderX : FIG::shoulderX, FIG::shoulderY, 0};
    arm->rotation.z = left ? -0.12f : 0.12f;
    arm->add(prop(FIG::armGeo(), shirt));
    Mesh* hand = prop(sphereGeo(0.055f, 10, 8), skin);
    hand->position.y = -0.42f;
    arm->add(hand);
  }
  f.bodyPivot->add(f.body, f.headPivot, f.armL, f.armR);
  f.root->add(f.legs, f.bodyPivot);
  if (o.apron) {
    Material* am = mat(PAL::apron, {.rough = 0.9f});
    Mesh* a = rbox(0.36f, 0.46f, 0.04f, am, 0.015f);
    a->position = {0, 0.44f, 0.215f};
    a->rotation.x = -0.1f;
    Mesh* bib = rbox(0.22f, 0.2f, 0.035f, am, 0.012f);
    bib->position = {0, 0.68f, 0.19f};
    bib->rotation.x = -0.15f;
    f.bodyPivot->add(a, bib);
  }
  if (o.notebook) {
    // Held in the right hand, open toward Jia.
    f.armR->rotation = {-0.75f, 0, 0.18f};
    f.book = rbox(0.2f, 0.26f, 0.035f, mat(0xf7f3ea, {.rough = 0.8f}), 0.008f);
    f.book->position = {0.02f, -0.42f, 0.06f};
    f.book->rotation = {-0.6f, 0, 0};
    f.armR->add(f.book);
  }
  return f;
}

void pose(const Figure& f, const PoseOpts& p) {
  float bob = std::fabs(std::sin(p.phase)) * 0.05f * p.walking;
  float sway = std::sin(p.phase) * 0.05f * p.walking;
  if (!f.isSeated) {
    f.bodyPivot->position.y = bob;
    f.bodyPivot->rotation.z = sway;
    f.legs->position.y = FIG::legsY + bob * 0.4f;
    // Arms swing against the steps; an arm holding the notebook stays up.
    float swing = std::sin(p.phase) * 0.55f * p.walking;
    f.armL->rotation.x = swing;
    if (!f.book) f.armR->rotation.x = -swing;
  }
  f.headPivot->rotation.y = p.look;
  f.headPivot->rotation.x = p.nod;
}

Figure seated(const FigOpts& o) {
  Figure f = figure(o);
  // On a stool: the body rises to the seat, the legs point forward, the hands rest on the table.
  f.bodyPivot->position.y = 0.2f;
  f.legs->rotation.x = PI / 2;
  f.legs->position = {0, 0.44f, 0.12f};
  f.armL->rotation = {-0.95f, 0, -0.1f};
  if (!f.book) f.armR->rotation = {-0.95f, 0, 0.1f};
  f.isSeated = true;
  return f;
}

// ---------- furniture and small props ----------
static const std::vector<uint32_t> bookColors = {0xc86a54, 0x5a7fae, 0xe2bd62, 0x79a886, 0x977cbc, 0xece4d2, 0xa85a44, 0x4a6a80, 0xd3ad82};

Node* shelf(float w, float h, float d, const ShelfOpts& o) {
  static Geometry* unitBox = geo::box(1, 1, 1);
  static Material* bookMat = standard({.roughness = 0.9f, .paper = 1});
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
  Material* post = matS(0x3b4250, 0.7f);
  g->add(place(prop(facetedCylinder(0.045f, 0.065f, h, 6), post), 0, h / 2, 0));
  // A small lantern: a cap, four corner posts and the bulb inside.
  g->add(place(prop(facetedCone(0.2f, 0.16f, 4), post), 0, h + 0.27f, 0, PI / 4));
  for (int i = 0; i < 4; i++) {
    float a = PI / 4 + i * PI / 2;
    g->add(place(rbox(0.025f, 0.26f, 0.025f, post, 0.005f), std::cos(a) * 0.1f, h + 0.06f, std::sin(a) * 0.1f));
  }
  g->add(place(rbox(0.22f, 0.03f, 0.22f, post, 0.01f), 0, h - 0.08f, 0));
  Mesh* bulb = eng::mesh(sphereGeo(0.085f, 14, 10), standard({.color = 0xfff1d6, .emissive = 0xffd08a, .emissiveIntensity = 0}));
  bulb->position.y = h + 0.06f;
  g->add(bulb);
  return {g, bulb};
}

Node* tree(Rng& r, float s) {
  Node* g = group();
  float trunkH = 0.85f * s;
  g->add(place(prop(facetedCylinder(0.07f * s, 0.11f * s, trunkH, 5), matS(0x7a5a40, 0.9f)), 0, trunkH / 2, 0));
  uint32_t green = pick(r, std::vector<uint32_t>{0x7fae6a, 0x6b9e62, 0x8fbd72, 0x9cc47a, 0x5f9461});
  double kind = r();
  if (kind < 0.42) {
    // Three folded cones, each a little smaller and turned.
    for (int i = 0; i < 3; i++) {
      float rr = (0.8f - i * 0.17f) * s, hh = (1.05f - i * 0.12f) * s;
      Mesh* c = prop(facetedCone(rr, hh, 7), matS(green, 0.9f));
      c->position.y = trunkH + 0.25f * s + i * 0.5f * s;
      c->rotation.y = i * 0.45f;
      g->add(c);
    }
  } else if (kind < 0.8) {
    // A faceted crown of two or three paper balls.
    g->add(place(prop(facetedBall(0.82f * s, 1), matS(green, 0.9f)), 0, trunkH + 0.55f * s, 0));
    g->add(place(prop(facetedBall(0.55f * s, 1), matS(green, 0.9f)), 0.38f * s, trunkH + 1.05f * s, 0.12f * s));
    if (r() < 0.5) g->add(place(prop(facetedBall(0.46f * s, 0), matS(green, 0.9f)), -0.42f * s, trunkH + 0.82f * s, -0.18f * s));
  } else {
    // Two crossed cut-outs of a round tree, as in a pop-up book.
    std::vector<vec2> crown = polygonOutline(16, 0.95f, 0.82f, 0.1f);
    for (vec2& q : crown) q = {q.x, q.y * 1.1f};
    static Material* edge = matS(0xe9f0dc, 0.9f);
    for (int k = 0; k < 2; k++) {
      Node* cut = card(crown, 0.04f, matS(green, 0.9f), edge);
      cut->position.y = trunkH + 0.85f * s;
      cut->rotation.y = k * PI / 2;
      cut->scale.setScalar(s);
      g->add(cut);
    }
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
