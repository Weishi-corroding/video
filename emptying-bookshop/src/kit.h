// Shared toolkit: palette, easing, materials, and the small props the film is built from.
#pragma once
#include <eng/eng.h>

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace film {
using namespace eng;

// Colour carries meaning, and every coloured label also says what it is:
// teal = a fact Jia found, coral = an assumption or a problem, gold = a named principle,
// blue / violet = the two sides of the shop's contradiction.
namespace ROLE {
constexpr uint32_t fact = 0x4fd1c5, flaw = 0xff7a6b, principle = 0xffcf5a, shopSide = 0x7aa2ff, needSide = 0xd08cff;
}

namespace PAL {
constexpr uint32_t lawn = 0x8fae7e, lawnDark = 0x7b9a6c, path = 0xe6dac4, plinth = 0x2a3140, stone = 0xdccfb6, cream = 0xf0e6d2,
                   brick = 0xc98e6c, roof = 0xa65f48, roofGrey = 0x6f7a86, wood = 0xb98a5e, woodDark = 0x7d5a3c, glass = 0xbfd8e6,
                   apron = 0x2f6b55;
inline const std::vector<uint32_t> skin = {0xf1d0b5, 0xd9a882, 0xb98563, 0x8d5f43, 0xf5dcc8};
inline const std::vector<uint32_t> shirts = {0x5b7fa6, 0xc9785d, 0x6f9a7a, 0xd9b45a, 0x8a6fae, 0x4f6f78, 0xb85c6a, 0xe0ddd2, 0x5e6b8c, 0x9a8b6a};
}  // namespace PAL

// ---------- easing ----------
using Ease = float (*)(float);
float linear(float x);
float smooth(float x);
float smoother(float x);
float easeOut(float x);
float easeIn(float x);
float backOut(float x);
// A heavy object arriving: quick approach, small settle, no bounce above the target.
float settle(float x);
/** 0 → 1 as t goes from a to b. */
float ramp(float t, float a, float b, Ease f = smooth);
/** Visible from a to b, fading in over fin and out over fout. */
float win(float t, float a, float b, float fin = 0.6f, float fout = 0.6f);
/** A single bump of height 1 starting at a. */
float pulse(float t, float a, float d = 0.8f);

// The same deterministic sequence as the original layout (mulberry32).
struct Rng {
  uint32_t a;
  explicit Rng(uint32_t seed) : a(seed) {}
  double operator()();
};
template <class T>
const T& pick(Rng& r, const std::vector<T>& list) {
  return list[(size_t)std::floor(r() * list.size())];
}

inline vec3 v3(float x = 0, float y = 0, float z = 0) { return {x, y, z}; }

// ---------- materials ----------
/** A shared, never-animated material. */
Material* matS(uint32_t color, float rough = 0.85f, float metal = 0);
struct MatOpts {
  float rough = 0.85f, metal = 0;
  uint32_t emissive = 0x000000;
  float ei = 1;
  bool transparent = false;
  float opacity = 1;
  bool depthWrite = true;
  Side side = Side::Front;
};
/** A material of its own, safe to fade or tint. */
Material* mat(uint32_t color, const MatOpts& o = {});
/** Unlit colour. */
Material* glow(uint32_t color, float opacity = 1);

// ---------- meshes ----------
// A mesh that casts and receives shadows (most of the diorama).
Mesh* prop(Geometry* g, Material* m, bool cast = true, bool receive = true);
Mesh* rbox(float w, float h, float d, Material* m, float r = 0.06f);
Mesh* rbox(float w, float h, float d, uint32_t color, float r = 0.06f);
template <class T>
T* place(T* obj, float x, float y, float z, float ry = 0) {
  obj->position = {x, y, z};
  obj->rotation.y = ry;
  return obj;
}
/** Fade a whole group. Materials must not be shared with objects that stay opaque. */
void fade(Node* obj, float o);
/** Give a group its own copies of every material so it can be faded independently. */
Node* ownMaterials(Node* obj);
void scaleIn(Node* obj, float s);

// Cached geometry, so identical shapes share one copy on the GPU.
Geometry* cylinderGeo(float rTop, float rBottom, float h, int radial, int hs = 1, bool open = false);
Geometry* sphereGeo(float r, int ws, int hs, float phiStart = 0, float phiLen = 2 * PI, float thetaStart = 0, float thetaLen = PI);

// ---------- canvas text (for diegetic signs) ----------
struct CanvasTex {
  Canvas canvas;
  Texture* tex = nullptr;
  std::function<void(Canvas&, int, int)> draw;
  void redraw();
};
CanvasTex* canvasTexture(int w, int h, std::function<void(Canvas&, int, int)> draw);
// Draw centred lines of text the way the original canvas signs did (textBaseline 'middle').
void drawCentredLines(Canvas& cv, const std::vector<std::string>& lines, float cx, float cy, float fs, float weight, RGBA color, float lineStep);
// Width of a line at a size and weight, in Inter.
float measureSans(const std::string& s, float size, float weight);

struct SignOpts {
  float w = 1, h = 0.3f;
  std::string text;
  int px = 256;
  float weight = 700;
  uint32_t fg = 0x2b2b2b, bg = 0xf4ead6;
  std::optional<uint32_t> border;
  bool emissive = false;
  float size = 0.62f;
};
/** A flat sign with text drawn on a canvas. set() redraws it when something changed. */
struct Sign {
  Mesh* mesh = nullptr;
  CanvasTex* ct = nullptr;
  std::string text;
  uint32_t fg = 0, bg = 0;
  std::optional<uint32_t> border;
  float size = 0.62f, weight = 700;
  void set(const std::optional<std::string>& t, std::optional<uint32_t> fgNew = {}, std::optional<uint32_t> bgNew = {});
  Material* material() const { return mesh->material; }
};
Sign* sign(const SignOpts& o);

// ---------- people ----------
namespace FIG {
Geometry* bodyGeo();
Geometry* headGeo();
constexpr float bodyY = 0.42f, headY = 1.0f;
}  // namespace FIG

struct FigOpts {
  uint32_t shirt = 0x5b7fa6, skin = 0xf1d0b5;
  bool apron = false, notebook = false;
  std::optional<uint32_t> hair;
};
/** A small figure: capsule body, round head. Optional apron and notebook mark Jia. */
struct Figure {
  Node* root = nullptr;
  Mesh* body = nullptr;
  Node* headPivot = nullptr;
  Mesh* book = nullptr;
  bool isSeated = false;
  Node* operator->() const { return root; }
};
Figure figure(const FigOpts& o = {});
struct PoseOpts {
  float walking = 0, phase = 0, look = 0, nod = 0;
};
/** Walking bob and head turn. phase advances with distance walked. */
void pose(const Figure& f, const PoseOpts& p);
/** A seated figure (shorter body, no bob). */
Figure seated(const FigOpts& o = {});

// ---------- furniture and small props ----------
struct ShelfOpts {
  uint32_t seed = 1;
  int rows = 4;
  bool messy = false;
  uint32_t frame = PAL::woodDark;
};
/** A shelf unit filled with books. messy = some books lying flat or missing. */
Node* shelf(float w, float h, float d, const ShelfOpts& o = {});
Node* table(float w = 1.1f, float d = 0.75f, float h = 0.62f, uint32_t top = PAL::wood);
Node* roundTable(float r = 0.5f, float h = 0.62f);
Node* stool();
Node* sofa(uint32_t color = 0x8a6f5a);
Node* counter(float w = 2.2f, uint32_t color = 0x9c7452);
struct LampPost {
  Node* root;
  Mesh* bulb;
};
LampPost lampPost(float h = 2.6f);
Node* tree(Rng& r, float s = 1);
Mesh* hedge(float len, float h = 0.8f, float d = 0.6f);

// ---------- lines and threads ----------
/** An arc between two points, lifted by `lift` at the middle. */
const Curve& arc(vec3 a, vec3 b, float lift);
/** A tube that can be drawn on progressively: grow(p) shows the first fraction p. */
struct GrowTube {
  Mesh* mesh = nullptr;
  int segs = 80, radial = 8;
  void grow(float p) const;
};
GrowTube growTube(const Curve& curve, float radius, Material* m, int segs = 80, int radial = 8);
/** A soft round disc lying on the ground. */
Mesh* disc(float radius, uint32_t color, float opacity = 0.5f);
Mesh* ringMesh(float r, float tube, uint32_t color, float opacity = 1);
/** A path through points (x, z) with slightly rounded corners, sampled by arc length. */
const Curve& polyPath(const std::vector<std::pair<float, float>>& points, float y = 0);
// Keep a curve alive for the whole program.
const Curve& keepCurve(Curve* c);

}  // namespace film
