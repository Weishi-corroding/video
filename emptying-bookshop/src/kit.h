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

// Everything is made of paper: matte, slightly warm colours, as coloured card and cartridge paper.
namespace PAL {
constexpr uint32_t lawn = 0x9dbf7c, lawnDark = 0x7fa466, path = 0xf0e4c8, stone = 0xe8dcc2, cream = 0xf5ecda, brick = 0xd48c68,
                   roof = 0xc56a4f, roofGrey = 0x8494a3, wood = 0xc49a6c, woodDark = 0x8a6646, glass = 0xcfe2ea, apron = 0x2f7560,
                   kraft = 0xc9a77c, ink = 0x2f2a26, pageEdge = 0xf3ead6, cover = 0x2c5d63, coverDark = 0x214a4f, gilt = 0xd9b45a;
inline const std::vector<uint32_t> skin = {0xf3d3b8, 0xe0b08a, 0xc48e6a, 0x9a6a4c, 0xf7e0cc};
inline const std::vector<uint32_t> shirts = {0x6a8fb8, 0xd98a6a, 0x7fae8a, 0xe6c06a, 0x9a80c0, 0x5f8a94, 0xc96f7c, 0xece6d6, 0x6e7ca6, 0xb09c76};
inline const std::vector<uint32_t> trousers = {0x3e4658, 0x524638, 0x34405a, 0x5f5648, 0x46505e};
inline const std::vector<uint32_t> hair = {0x2a2220, 0x4a3426, 0x1d1d22, 0x6b4a2e, 0x3a2a22};
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
  float paper = 1;  // the paper look (0 for glass, metal, lights)
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
// The same shapes with crisp facets, like folded paper.
Geometry* facetedCylinder(float rTop, float rBottom, float h, int radial);
Geometry* facetedCone(float r, float h, int radial);
Geometry* facetedBall(float r, int detail = 1);

// ---------- cut card ----------
/** A shape cut from card: faces in one colour, the cut edge a paler one. */
Node* card(const std::vector<vec2>& outline, float depth, Material* face, Material* edge = nullptr);
/** A cloud silhouette (for card(), in a w × h box centred on the origin). */
std::vector<vec2> cloudOutline(float w, float h, uint32_t seed);
/** A regular polygon, or a star with `inner` > 0. */
std::vector<vec2> polygonOutline(int sides, float r, float inner = 0, float rot = 0);
/** Stripes for awnings and page edges: `count` bands alternating between two colours. */
Texture* stripeTexture(uint32_t a, uint32_t b, int count, bool vertical, float lineFrac = 0.5f);

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
// Paper dolls: trousers and a rolled-paper torso, folded at the facets, card arms, a round head with
// two dots for eyes, and a cap of hair.
namespace FIG {
Geometry* legsGeo();
Geometry* bodyGeo();
Geometry* headGeo();
Geometry* hairGeo();
Geometry* eyesGeo();
Geometry* armGeo();   // hangs down from the shoulder (pivot at its top)
constexpr float legsY = 0.13f, bodyY = 0.5f, headY = 0.98f, shoulderY = 0.74f, shoulderX = 0.215f;
}  // namespace FIG

struct FigOpts {
  uint32_t shirt = 0x5b7fa6, skin = 0xf1d0b5;
  bool apron = false, notebook = false;
  std::optional<uint32_t> hair;
  std::optional<uint32_t> trousers;
};
/** A small figure. Optional apron and notebook mark Jia. */
struct Figure {
  Node* root = nullptr;
  Node* bodyPivot = nullptr;  // everything above the legs; bobs when walking
  Mesh* body = nullptr;
  Mesh* legs = nullptr;
  Node* headPivot = nullptr;
  Node* armL = nullptr;
  Node* armR = nullptr;
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
/** A papercraft tree: stacked folded cones, a faceted crown, or two crossed cut-outs. */
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
