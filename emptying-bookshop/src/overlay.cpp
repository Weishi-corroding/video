#include "overlay.h"

#include <cmath>
#include <sstream>

#include "script.h"

namespace film {

namespace {

const RGBA INK = rgba(0xf4efe6), INK_DIM = rgba(0xc9d0da);
constexpr float LBL = 13.44f;  // label size at the 1280 px design width (clamp(12px, 1.05vw, 17px))
constexpr float CAP = 19.84f;  // caption size (clamp(15px, 1.55vw, 24px))

bool has(const std::string& cls, const char* c) {
  std::istringstream in(cls);
  std::string w;
  while (in >> w)
    if (w == c) return true;
  return false;
}

TextStyle st(Font f, float size, float weight, RGBA color, float lh = 1.3f, float ls = 0, bool upper = false) {
  TextStyle s;
  s.font = f;
  s.size = size;
  s.weight = weight;
  s.color = color;
  s.lineHeight = lh;
  s.letterSpacing = ls;
  s.uppercase = upper;
  return s;
}

struct TextShadow {
  float dx, dy, blur;
  RGBA color;
};

// One block in a vertical stack. paint(x, y, cw) draws it with its top-left at (x, y) inside a
// content box cw wide.
struct El {
  float mt = 0, mb = 0, w = 0, h = 0, opacity = 1;
  std::function<void(Canvas&, float, float, float)> paint;
};

El textEl(std::vector<Span> spans, float maxW, Align align, std::optional<TextShadow> sh = {}, float mt = 0) {
  auto tb = std::make_shared<TextBlock>(layoutText(spans, maxW, align));
  El e;
  e.mt = mt;
  e.w = tb->width;
  e.h = tb->height;
  e.paint = [tb, align, sh](Canvas& c, float x, float y, float cw) {
    float bx = align == Align::Center ? x + (cw - tb->width) / 2 : x;
    if (sh) {
      Mask m = textMask(*tb, bx + sh->dx, y + sh->dy);
      m.blur(sh->blur / 2);
      c.fillMask(m, 0, 0, sh->color);
    }
    drawText(c, *tb, bx, y);
  };
  return e;
}

// Stack blocks vertically; returns content width and height.
std::pair<float, float> measure(const std::vector<El>& els) {
  float w = 0, h = 0;
  for (const El& e : els) {
    w = std::max(w, e.w);
    h += e.mt + e.h + e.mb;
  }
  return {w, h};
}

void paintStack(Canvas& c, const std::vector<El>& els, float x, float y, float cw) {
  for (const El& e : els) {
    y += e.mt;
    if (e.opacity < 0.999f) {
      // Group opacity: draw the block on its own, then blend it in.
      int pad = 4;
      Canvas tmp((int)std::ceil(cw) + 2 * pad, (int)std::ceil(e.h) + 2 * pad);
      e.paint(tmp, (float)pad, (float)pad, cw);
      c.draw(tmp, (int)std::round(x) - pad, (int)std::round(y) - pad, e.opacity);
    } else {
      e.paint(c, x, y, cw);
    }
    y += e.h + e.mb;
  }
}

struct Layer {
  Canvas canvas;
  int ox = 0, oy = 0;  // where the box's top-left corner sits in the canvas
  float w = 0, h = 0;
  float radius = 0;
  bool backdrop = false;
};

RGBA accentOf(const LabelSpec& s) {
  if (has(s.cls, "stamp")) return rgba(ROLE::flaw);
  switch (s.kind) {
    case Kind::Fact: return rgba(ROLE::fact);
    case Kind::Flaw: return rgba(ROLE::flaw);
    case Kind::Principle: return rgba(ROLE::principle);
    case Kind::Shop: return rgba(ROLE::shopSide);
    case Kind::Need: return rgba(ROLE::needSide);
    case Kind::Panel:
      if (has(s.cls, "gold")) return rgba(ROLE::principle);
      if (has(s.cls, "teal")) return rgba(ROLE::fact);
      if (has(s.cls, "coral")) return rgba(ROLE::flaw);
      return rgba(0xcfd6e0);
    default: return rgba(0xcfd6e0);
  }
}

// Build a label's picture: shadow, background, border and content, the way the CSS draws it.
Layer buildLayer(const LabelSpec& s, bool zh, float S, float t) {
  const bool card = s.kind == Kind::Card, title = s.kind == Kind::Title, panel = s.kind == Kind::Panel, chapter = s.kind == Kind::Chapter;
  const bool note = s.kind == Kind::Note, snap = has(s.cls, "snapframe"), small = has(s.cls, "small"), plain = has(s.cls, "plain");
  const bool stamp = has(s.cls, "stamp"), strike = has(s.cls, "strike"), quote = has(s.cls, "quote"), gold = has(s.cls, "gold");
  const RGBA accent = accentOf(s);

  float F = LBL * (card ? 1.45f : panel ? 1.18f : note ? 0.85f : small ? 0.88f : 1.f) * S;
  // Padding (top, right, bottom, left) in ems.
  float pt = 0.42f, pr = 0.75f, pb = 0.48f, pl = 0.75f;
  if (small) { pt = pb = 0.3f; pr = pl = 0.6f; }
  if (card) { pt = 1.1f; pr = pl = 1.6f; pb = 1.2f; }
  if (panel) { pt = 0.8f; pr = pl = 1.f; pb = 0.9f; }
  if (chapter || snap) pt = pr = pb = pl = 0;
  pt *= F; pr *= F; pb *= F; pl *= F;
  // Borders.
  float bl = 3 * S, bt = 0, br = 0, bb = 0;
  RGBA bcol = accent;
  if (s.kind == Kind::Principle) { bt = br = bb = 1 * S; bcol = {1, 207 / 255.f, 90 / 255.f, 0.55f}; }
  if (stamp) { bl = bt = br = bb = 2 * S; bcol = rgba(ROLE::flaw); }
  if (plain) bcol = {0, 0, 0, 0};
  if (note) bcol = {1, 1, 1, 0.25f};
  if (card || panel) bl = 0;
  if (card && gold) { bl = bt = br = bb = 1 * S; bcol = {1, 207 / 255.f, 90 / 255.f, 0.5f}; }
  if (chapter || title || snap) bl = bt = br = bb = 0;
  // Background.
  RGBA bg{12 / 255.f, 16 / 255.f, 26 / 255.f, 0.8f};
  if (plain) bg.a = 0.62f;
  if (stamp) bg = {30 / 255.f, 12 / 255.f, 12 / 255.f, 0.82f};
  if (card) bg = {10 / 255.f, 13 / 255.f, 22 / 255.f, 0.78f};
  if (note) bg.a = 0.55f;
  if (chapter || title || snap) bg.a = 0;
  float radius = (stamp ? 6 : card ? 12 : panel ? 10 : snap ? 2 : 7) * S;
  bool boxShadow = !(chapter || title || snap);
  float maxW = (card ? 860 : title ? 980 : panel ? 560 : chapter ? 768 : 420) * S;
  float avail = maxW - pl - pr - bl - br;
  float weight = stamp ? 700 : note ? 450 : 520;
  RGBA color = note ? INK_DIM : INK;
  Align align = card || title ? Align::Center : Align::Left;

  std::vector<El> els;
  // Kicker.
  std::optional<Pair> kick = s.kicker;
  if (!kick && !s.noKicker) {
    if (s.kind == Kind::Fact) kick = Pair{"Finding", "发现"};
    if (s.kind == Kind::Principle) kick = Pair{"Principle", "原理"};
  }
  if (kick) {
    float ks = 0.72f * F;
    std::vector<Span> sp{{kick->first, st(Font::Sans, ks, 700, accent, 1.3f, 0.08f * ks, true)}};
    if (zh) sp.push_back({kick->second, st(Font::CJK, ks, 450, accent), 0.5f * ks});
    El e = textEl(sp, avail, align);
    e.mb = 0.15f * ks;
    els.push_back(e);
  }
  // Main text.
  if (!s.en.empty()) {
    if (card) {
      std::string text = quote ? "“" + s.en + "”" : s.en;
      els.push_back(textEl({{text, st(quote ? Font::SerifItalic : Font::Serif, F, 500, INK)}}, avail, align));
    } else if (title) {
      float ts = 58.88f * S;
      els.push_back(textEl({{s.en, st(Font::Serif, ts, 600, INK, 1.05f)}}, avail, align, TextShadow{0, 4 * S, 30 * S, {0, 0, 0, 0.6f}}));
    } else {
      TextStyle ts = st(Font::Sans, F, weight, strike ? INK_DIM : color, 1.3f, stamp ? 0.01f * F : 0);
      if (strike) {
        ts.strike = true;
        ts.strikeColor = rgba(ROLE::flaw);
        ts.strikeWidth = 2 * S;
      }
      els.push_back(textEl({{s.en, ts}}, avail, align));
    }
  }
  // Chinese line.
  if (!s.zh.empty() && (zh || title)) {
    if (card) {
      float zs = 0.86f * F;
      els.push_back(textEl({{s.zh, st(Font::CJK, zs, 450, INK_DIM)}}, avail, align, {}, 0.45f * zs));
    } else if (title) {
      float zs = 26.88f * S;
      els.push_back(textEl({{s.zh, st(Font::CJK, zs, 450, INK)}}, avail, align, TextShadow{0, 2 * S, 16 * S, {0, 0, 0, 0.6f}}, 0.3f * zs));
    } else {
      float zs = 0.94f * F;
      els.push_back(textEl({{s.zh, st(Font::CJK, zs, 450, INK_DIM)}}, avail, align, {}, 0.12f * zs));
    }
  }
  // Title subline.
  if (s.sub) {
    float ss = 18.56f * S;
    El en = textEl({{s.sub->first, st(Font::Sans, ss, 520, INK_DIM)}}, avail, align, TextShadow{0, 2 * S, 12 * S, {0, 0, 0, 0.7f}}, 0.9f * ss);
    els.push_back(en);
    float zs = 0.92f * ss;
    els.push_back(textEl({{s.sub->second, st(Font::CJK, zs, 450, INK)}}, avail, align, TextShadow{0, 2 * S, 16 * S, {0, 0, 0, 0.6f}}, 0.15f * zs));
  }
  // Card attribution.
  if (s.attr) {
    float as = 0.6f * F;
    els.push_back(textEl({{s.attr->first, st(Font::Sans, as, 520, INK_DIM, 1.3f, 0.02f * as)}}, avail, align, {}, 0.7f * as));
    if (zh) els.push_back(textEl({{s.attr->second, st(Font::CJK, as, 450, INK_DIM)}}, avail, align, {}, 0.1f * as));
  }
  // Chapter tag.
  if (s.chapter) {
    auto& [num, ttl, czh] = *s.chapter;
    float ns = 0.8f * F;
    els.push_back(textEl({{num, st(Font::Sans, ns, 700, rgba(ROLE::principle), 1.3f, 0.14f * ns, true)}}, avail, Align::Left));
    float ts = 1.9f * F;
    els.push_back(textEl({{ttl, st(Font::Serif, ts, 560, INK, 1.15f)}}, avail, Align::Left, TextShadow{0, 2 * S, 14 * S, {0, 0, 0, 0.55f}}));
    if (zh) {
      float zs = 1.15f * F;
      els.push_back(textEl({{czh, st(Font::CJK, zs, 450, INK)}}, avail, Align::Left, TextShadow{0, 2 * S, 10 * S, {0, 0, 0, 0.6f}}, 0.12f * zs));
    }
  }
  // Panel: heading, list or chips, footnote.
  if (s.panel) {
    const PanelSpec& P = *s.panel;
    float hs = 0.78f * F;
    std::vector<Span> h4{{P.h4.first, st(Font::Sans, hs, 700, accent, 1.3f, 0.1f * hs, true)}};
    if (zh) h4.push_back({P.h4.second, st(Font::CJK, 0.94f * hs, 450, INK_DIM), 0.5f * 0.94f * hs});
    El he = textEl(h4, avail, Align::Left);
    he.mb = 0.5f * hs;
    els.push_back(he);
    for (size_t i = 0; i < P.items.size(); i++) {
      const PanelItem& it = P.items[i];
      float ns = 0.8f * F, D = 1.5f * ns, gap = 0.6f * F;
      float txW = avail - D - gap;
      std::vector<Span> tx{{it.p.empty() ? it.en : it.en + " ", st(Font::Sans, F, 520, INK)}};
      if (!it.p.empty()) tx.push_back({"· " + it.p, st(Font::Sans, F, 650, accent)});
      auto main = std::make_shared<TextBlock>(layoutText(tx, txW, Align::Left));
      std::shared_ptr<TextBlock> zline;
      float zs = 0.92f * F, zmt = 0.12f * zs;
      if (zh && !it.zh.empty()) zline = std::make_shared<TextBlock>(layoutText(it.zh, st(Font::CJK, zs, 450, INK_DIM), txW));
      float txH = main->height + (zline ? zmt + zline->height : 0);
      // The number circle's text baseline lines up with the first line of the item text.
      TextStyle nst = st(Font::Sans, ns, 700, rgba(0x141414));
      auto num = std::make_shared<TextBlock>(layoutText(std::to_string(i + 1), nst, 0));
      float circleBase = (D - num->height) / 2 + num->firstBaseline;
      float circleTop = main->firstBaseline - circleBase;
      float top = std::min(0.f, circleTop), bottom = std::max(txH, circleTop + D);
      float padY = 0.22f * F;
      El e;
      e.w = D + gap + std::max(main->width, zline ? zline->width : 0.f);
      e.h = padY * 2 + (bottom - top);
      e.opacity = P.itemOpacity ? P.itemOpacity((int)i, t) : 1;
      e.paint = [=](Canvas& c, float x, float y, float) {
        float y0 = y + padY - top;
        c.fillCircle(x + D / 2, y0 + circleTop + D / 2, D / 2, accent);
        drawText(c, *num, x + (D - num->width) / 2, y0 + circleTop + (D - num->height) / 2);
        drawText(c, *main, x + D + gap, y0);
        if (zline) drawText(c, *zline, x + D + gap, y0 + main->height + zmt);
      };
      els.push_back(e);
    }
    if (!P.chips.empty()) {
      // A wrapping row of pill-shaped chips.
      float gap = 0.5f * F, px = 0.7f * F, py = 0.3f * F, bw = 1 * S;
      struct Chip { std::shared_ptr<TextBlock> en, zh; float w, h; };
      std::vector<Chip> chips;
      for (const auto& [en, czh] : P.chips) {
        Chip ch;
        ch.en = std::make_shared<TextBlock>(layoutText(en, st(Font::Sans, F, 650, INK), 0));
        if (zh) ch.zh = std::make_shared<TextBlock>(layoutText(czh, st(Font::CJK, 0.9f * F, 450, INK_DIM), 0));
        ch.w = std::max(ch.en->width, ch.zh ? ch.zh->width : 0.f) + 2 * px + 2 * bw;
        ch.h = ch.en->height + (ch.zh ? ch.zh->height : 0.f) + 2 * py + 2 * bw;
        chips.push_back(ch);
      }
      std::vector<std::pair<float, float>> pos;
      float x = 0, y = 0, rowH = 0, w = 0;
      for (const Chip& ch : chips) {
        if (x > 0 && x + ch.w > avail) { x = 0; y += rowH + gap; rowH = 0; }
        pos.push_back({x, y});
        x += ch.w + gap;
        w = std::max(w, x - gap);
        rowH = std::max(rowH, ch.h);
      }
      El e;
      e.w = w;
      e.h = y + rowH;
      e.paint = [=](Canvas& c, float x0, float y0, float) {
        for (size_t i = 0; i < chips.size(); i++) {
          const Chip& ch = chips[i];
          float cx = x0 + pos[i].first, cy = y0 + pos[i].second;
          float r = ch.h / 2;
          c.fillRoundRectRing(cx, cy, ch.w, ch.h, r, cx + bw, cy + bw, ch.w - 2 * bw, ch.h - 2 * bw, r - bw, accent);
          drawText(c, *ch.en, cx + bw + px, cy + bw + py);
          if (ch.zh) drawText(c, *ch.zh, cx + bw + px, cy + bw + py + ch.en->height);
        }
      };
      els.push_back(e);
    }
    if (P.foot) {
      float fs = 0.82f * F;
      els.push_back(textEl({{P.foot->first, st(Font::Sans, fs, 450, INK_DIM)}}, avail, Align::Left, {}, 0.6f * fs));
      if (zh) {
        float zs = 0.94f * fs;
        els.push_back(textEl({{P.foot->second, st(Font::CJK, zs, 450, INK_DIM)}}, avail, Align::Left, {}, 0.12f * zs));
      }
    }
  }

  auto [cw, chh] = measure(els);
  float w = cw + pl + pr + bl + br, h = chh + pt + pb + bt + bb;
  if (snap) { w = 793.6f * S; h = 403.2f * S; }

  Layer L;
  int m = (int)std::ceil((title ? 60 : 36) * S);
  L.canvas.resize((int)std::ceil(w) + 2 * m, (int)std::ceil(h) + 2 * m);
  L.ox = L.oy = m;
  L.w = w;
  L.h = h;
  L.radius = radius;
  L.backdrop = boxShadow;
  Canvas& c = L.canvas;
  float X = (float)m, Y = (float)m;
  if (boxShadow) {
    // 0 6px 18px rgba(0,0,0,0.25), visible only outside the box.
    Mask sm;
    int sw = (int)std::ceil(w), sh = (int)std::ceil(h);
    sm.resize(m, m + (int)std::round(6 * S), sw, sh);
    for (int yy = 0; yy < sh; yy++)
      for (int xx = 0; xx < sw; xx++) sm.a[(size_t)yy * sw + xx] = Canvas::roundRectCoverage(xx + 0.5f, yy + 0.5f, 0, 0, w, h, radius);
    sm.blur(9 * S);
    for (int yy = 0; yy < sm.h; yy++)
      for (int xx = 0; xx < sm.w; xx++) sm.a[(size_t)yy * sm.w + xx] *= 1 - Canvas::roundRectCoverage(sm.x + xx + 0.5f, sm.y + yy + 0.5f, X, Y, w, h, radius);
    c.fillMask(sm, 0, 0, {0, 0, 0, 0.25f});
  }
  if (bg.a > 0) c.fillRoundRect(X, Y, w, h, radius, bg);
  if ((bl > 0 || bt > 0 || br > 0 || bb > 0) && bcol.a > 0) {
    // Sides without a border reach past the box so they leave no line.
    float ix = X + bl, iy = bt > 0 ? Y + bt : Y - radius, ix2 = br > 0 ? X + w - br : X + w + radius, iy2 = bb > 0 ? Y + h - bb : Y + h + radius;
    c.fillRoundRectRing(X, Y, w, h, radius, ix, iy, ix2 - ix, iy2 - iy, std::max(0.f, radius - std::max(bl, bt)), bcol);
  }
  if (snap) {
    float k = 10 * S;
    c.fillRoundRectRing(X, Y, w, h, radius, X + k, Y + k, w - 2 * k, h - 2 * k, 0, rgba(0xf7f3ea));
  }
  paintStack(c, els, X + bl + pl, Y + bt + pt, cw);
  return L;
}

}  // namespace

struct Labels::Label {
  LabelSpec spec;
  std::string key;
  Layer layer;
  float stemH = 0;
};

Labels::Labels(World& W, int width, int height, bool zh) : W_(W), w_(width), h_(height), zh_(zh), S_(height / 720.f) {}
Labels::~Labels() = default;

Labels::Label* Labels::add(LabelSpec spec) {
  auto L = std::make_unique<Label>();
  L->spec = std::move(spec);
  items_.push_back(std::move(L));
  return items_.back().get();
}

void Labels::set(Label* l, const std::string& en, const std::string& zh) {
  l->spec.en = en;
  l->spec.zh = zh;
}

void Labels::draw(uint8_t* frame, float t, const SafeArea& safe) {
  struct Box { float left, top, w, h; };
  std::vector<Box> placed;
  const float S = S_;
  for (auto& up : items_) {
    Label& L = *up;
    const LabelSpec& s = L.spec;
    float o = s.vis ? s.vis(t) : 1;
    if (o <= 0.003f) continue;

    // Rebuild the picture when its content changes.
    std::string key = s.en + "\x1f" + s.zh;
    if (s.panel && s.panel->itemOpacity)
      for (size_t i = 0; i < s.panel->items.size(); i++) key += "\x1f" + std::to_string((int)std::lround(s.panel->itemOpacity((int)i, t) * 100));
    if (key != L.key || L.layer.canvas.width == 0) {
      L.layer = buildLayer(s, zh_, S, t);
      L.key = key;
    }
    const Layer& ly = L.layer;
    bool screen = s.screen.has_value();
    vec2 anchor = s.anchor ? *s.anchor : (screen ? vec2{0.5f, 0.5f} : vec2{0.5f, 1});
    bool stem = s.stem ? *s.stem : !screen;

    float x, y;
    if (screen) {
      x = s.screen->x * w_;
      y = s.screen->y * (safe.bottom - safe.top) + safe.top;
    } else {
      vec3 p = s.at.fn ? s.at.fn(t) : s.at.node ? s.at.node->worldPosition() : *s.at.point;
      p.y += s.lift;
      vec3 n = W_.camera.project(p);
      if (n.z > 1) continue;
      x = (n.x + 1) / 2 * w_;
      y = (1 - n.y) / 2 * h_;
    }
    float stemH = stem ? 16 * S : 0;
    float left = x + s.offset.x * S - ly.w * anchor.x;
    float top = y + s.offset.y * S - ly.h * anchor.y - stemH;
    Box box{left, top, ly.w, ly.h};
    if (!screen) {
      // Nudge upward past any anchored label already placed that overlaps.
      float g6 = 6 * S;
      for (int pass = 0; pass < 3; pass++) {
        bool moved = false;
        for (const Box& b : placed)
          if (box.left < b.left + b.w + g6 && box.left + box.w + g6 > b.left && box.top < b.top + b.h + g6 && box.top + box.h + g6 > b.top) {
            box.top = b.top - box.h - 8 * S;
            moved = true;
          }
        if (!moved) break;
      }
    }
    // Keep inside the safe area.
    float cl = std::min(std::max(box.left, safe.left), w_ - safe.right - box.w);
    float ct = std::min(std::max(box.top, safe.top), safe.bottom - box.h);
    bool shifted = std::fabs(cl - left) > 2 * S || std::fabs(ct - top) > 2 * S;
    box.left = cl;
    box.top = ct;
    if (!screen) placed.push_back(box);

    float rise = (1 - o) * 6 * S;
    int px = (int)std::lround(box.left), py = (int)std::lround(box.top + rise);
    if (ly.backdrop) backdropBlur(frame, w_, h_, (float)px, (float)py, ly.w, ly.h, ly.radius, 6 * S, o);
    compositeOnto(frame, w_, h_, ly.canvas, px - ly.ox, py - ly.oy, o);
    if (stem && !shifted) {
      // Point the stem at the anchor.
      float sx = std::round(std::min(std::max(x - box.left, 10 * S), box.w - 10 * S));
      RGBA a = accentOf(s);
      fillOnto(frame, w_, h_, px + sx - 1 * S, py + ly.h, 2 * S, 16 * S, withAlpha(a, 0.85f * o));
    }
  }
}

// ---------------- captions ----------------
struct Captions::Cache {
  const Cue* cue = nullptr;
  Layer layer;
};

Captions::Captions(int width, int height, bool zh) : w_(width), h_(height), zh_(zh), S_(height / 720.f), cache_(std::make_shared<Cache>()) {}

SafeArea Captions::safeArea() const {
  // Labels stay clear of the caption band, sized for the lines that can appear there.
  const float fs = CAP, capBottom = 74;
  float band = 2 * 1.32f * fs;
  if (zh_) band += 2 * 1.26f * fs;
  band += 16;
  return {14 * S_, 14 * S_, 14 * S_, h_ - (capBottom + band + 10) * S_};
}

void Captions::draw(uint8_t* frame, float t) {
  // The cue on screen, or the one that just ended (it fades out over 0.18 s).
  const Cue* q = script::cueAt(t);
  float o = 0;
  if (q) o = clamp((t - q->s) / 0.18f);
  else {
    for (const Cue& k : script::cues())
      if (t >= k.e && t < k.e + 0.18f) { q = &k; o = 1 - (t - k.e) / 0.18f; }
  }
  if (!q || o <= 0) return;
  const float S = S_;
  if (cache_->cue != q) {
    cache_->cue = q;
    float fsz = CAP * S;
    float pt = 0.35f * 16 * S, px = 0.9f * 16 * S, pb = 0.42f * 16 * S;
    float maxW = 980 * S - 2 * px;
    std::vector<El> els;
    els.push_back(textEl({{q->en, st(Font::Sans, fsz, 500, INK, 1.32f)}}, maxW, Align::Center));
    if (zh_) {
      float zs = 0.9f * fsz;
      els.push_back(textEl({{q->zh, st(Font::CJK, zs, 400, rgba(0xdfe6ee), 1.4f)}}, maxW, Align::Center, {}, 0.1f * zs));
    }
    auto [cw, chh] = measure(els);
    Layer& L = cache_->layer;
    L.w = cw + 2 * px;
    L.h = chh + pt + pb;
    L.canvas.resize((int)std::ceil(L.w) + 2, (int)std::ceil(L.h) + 2);
    L.ox = L.oy = 1;
    L.canvas.fillRoundRect(1, 1, L.w, L.h, 8 * S, {6 / 255.f, 8 / 255.f, 14 / 255.f, 0.74f});
    paintStack(L.canvas, els, 1 + px, 1 + pt, cw);
  }
  const Layer& L = cache_->layer;
  int x = (int)std::lround((w_ - L.w) / 2), y = (int)std::lround(h_ - 74 * S - L.h);
  compositeOnto(frame, w_, h_, L.canvas, x - L.ox, y - L.oy, o);
}

}  // namespace film
