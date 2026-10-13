#include "overlay.h"

#include <cmath>
#include <sstream>

#include "script.h"

namespace film {

// The overlay is paper too: labels are cream tags held on with a strip of coloured tape and pinned to
// what they name; quotations are printed cards; the title is a book label. Text is in the film's
// language only, at a 1280 × 720 design size scaled to the output.

namespace {

const RGBA INK = rgba(0x2b2620), INK_DIM = rgba(0x6e6458), PAPER = rgba(0xf8f2e4), PAPER_WARM = rgba(0xf3e9d4);
const RGBA SHADOW = {0.16f, 0.11f, 0.06f, 0.3f};

bool zh() { return script::zh(); }
Font body() { return zh() ? Font::CJK : Font::Sans; }
Font serif() { return zh() ? Font::CJKSerif : Font::Serif; }
// Label size at the design width: Chinese needs a little more than Latin to read as easily.
float LBL() { return zh() ? 15.5f : 13.44f; }
float CAP() { return zh() ? 25.f : 19.84f; }

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

// One block in a vertical stack. paint(x, y, cw) draws it with its top-left at (x, y) inside a
// content box cw wide.
struct El {
  float mt = 0, mb = 0, w = 0, h = 0, opacity = 1;
  std::function<void(Canvas&, float, float, float)> paint;
};

El textEl(std::vector<Span> spans, float maxW, Align align, float mt = 0) {
  auto tb = std::make_shared<TextBlock>(layoutText(spans, maxW, align));
  El e;
  e.mt = mt;
  e.w = tb->width;
  e.h = tb->height;
  e.paint = [tb, align](Canvas& c, float x, float y, float cw) {
    float bx = align == Align::Center ? x + (cw - tb->width) / 2 : x;
    drawText(c, *tb, bx, y);
  };
  return e;
}

// A short horizontal rule, centred, as a block.
El ruleEl(float w, float thick, RGBA color, float mt, float mb) {
  El e;
  e.mt = mt;
  e.mb = mb;
  e.w = w;
  e.h = thick;
  e.paint = [=](Canvas& c, float x, float y, float cw) { c.fillRect(x + (cw - w) / 2, y, w, thick, color); };
  return e;
}

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
  bool pinned = true;  // a 3D label gets a thread and a pin
};

// The role colours, deepened so they read on cream paper.
RGBA accentOf(const LabelSpec& s) {
  if (has(s.cls, "stamp")) return rgba(0xc4473a);
  switch (s.kind) {
    case Kind::Fact: return rgba(0x1f8a80);
    case Kind::Flaw: return rgba(0xc4513f);
    case Kind::Principle: return rgba(0xb07d12);
    case Kind::Shop: return rgba(0x3d68c8);
    case Kind::Need: return rgba(0x9248c0);
    case Kind::Panel:
      if (has(s.cls, "gold")) return rgba(0xb07d12);
      if (has(s.cls, "teal")) return rgba(0x1f8a80);
      if (has(s.cls, "coral")) return rgba(0xc4513f);
      return INK_DIM;
    default: return rgba(0x8a7e6c);
  }
}

// A hash in 0..1 for paper grain.
float grain(int x, int y, uint32_t seed) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return ((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

// A rounded card of paper with a soft shadow below it and a faint grain.
void paperCard(Canvas& c, float X, float Y, float w, float h, float r, RGBA color, float S, uint32_t seed, float shadow = 1) {
  if (shadow > 0) {
    Mask sm;
    int sw = (int)std::ceil(w), sh = (int)std::ceil(h);
    sm.resize((int)X, (int)(Y + std::round(3 * S)), sw, sh);
    for (int yy = 0; yy < sh; yy++)
      for (int xx = 0; xx < sw; xx++) sm.a[(size_t)yy * sw + xx] = Canvas::roundRectCoverage(xx + 0.5f, yy + 0.5f, 0, 0, w, h, r);
    sm.blur(6 * S);
    c.fillMask(sm, 0, 0, withAlpha(SHADOW, shadow));
  }
  c.fillRoundRect(X, Y, w, h, r, color);
  int x0 = (int)X, y0 = (int)Y, x1 = (int)std::ceil(X + w), y1 = (int)std::ceil(Y + h);
  for (int y = std::max(0, y0); y < std::min(c.height, y1); y++)
    for (int x = std::max(0, x0); x < std::min(c.width, x1); x++) {
      float cov = Canvas::roundRectCoverage(x + 0.5f, y + 0.5f, X, Y, w, h, r);
      if (cov <= 0) continue;
      float k = 1 + (grain(x, y, seed) - 0.5f) * 0.05f * cov;
      float* p = &c.px[((size_t)y * c.width + x) * 4];
      p[0] *= k;
      p[1] *= k;
      p[2] *= k;
    }
}

// Build a label's picture.
Layer buildLayer(const LabelSpec& s, float S, float t) {
  const bool card = s.kind == Kind::Card, title = s.kind == Kind::Title, panel = s.kind == Kind::Panel, chapter = s.kind == Kind::Chapter;
  const bool note = s.kind == Kind::Note, snap = has(s.cls, "snapframe"), small = has(s.cls, "small"), plain = has(s.cls, "plain");
  const bool stamp = has(s.cls, "stamp"), strike = has(s.cls, "strike"), quote = has(s.cls, "quote"), gold = has(s.cls, "gold");
  const bool big = has(s.cls, "big");
  const RGBA accent = accentOf(s);
  auto txt = [](const std::string& en, const std::string& z) -> std::string { return zh() && !z.empty() ? z : en; };
  auto pairTxt = [&](const Pair& p) { return txt(p.first, p.second); };

  float F = LBL() * (card ? (big ? 1.6f : 1.42f) : panel ? 1.12f : note ? 0.86f : small ? 0.88f : 1.f) * S;
  float pt = 0.42f, pr = 0.75f, pb = 0.5f, pl = 0.75f;
  if (small) { pt = pb = 0.3f; pr = pl = 0.6f; }
  if (card) { pt = 1.15f; pr = pl = 1.7f; pb = 1.2f; }
  if (panel) { pt = 0.95f; pr = pl = 1.1f; pb = 1.0f; }
  if (title) { pt = 1.1f; pr = pl = 2.2f; pb = 1.2f; }
  if (chapter) { pt = 0.5f; pr = 1.0f; pb = 0.6f; pl = 1.2f; }
  if (snap) pt = pr = pb = pl = 0;
  pt *= F; pr *= F; pb *= F; pl *= F;
  float maxW = (card ? 860 : title ? 900 : panel ? 560 : chapter ? 600 : 400) * S;
  float avail = maxW - pl - pr;
  float weight = stamp ? 700 : note ? 400 : 500;
  RGBA color = note ? INK_DIM : INK;
  Align align = card || title ? Align::Center : Align::Left;

  std::vector<El> els;
  // Kicker: a small heading in the role colour.
  std::optional<Pair> kick = s.kicker;
  if (!kick && !s.noKicker) {
    if (s.kind == Kind::Fact) kick = Pair{"Finding", "发现"};
    if (s.kind == Kind::Principle) kick = Pair{"Principle", "原理"};
  }
  if (kick) {
    float ks = (zh() ? 0.78f : 0.72f) * F;
    El e = textEl({{pairTxt(*kick), st(body(), ks, 700, accent, 1.3f, zh() ? 0.1f * ks : 0.08f * ks, !zh())}}, avail, align);
    e.mb = 0.2f * ks;
    els.push_back(e);
  }
  // Main text.
  std::string main = txt(s.en, s.zh);
  if (!main.empty()) {
    if (card) {
      std::string text = quote ? "“" + main + "”" : main;
      els.push_back(textEl({{text, st(serif(), F, zh() ? 600 : 500, INK, 1.45f)}}, avail, align));
    } else if (title) {
      float ts = (zh() ? 50.f : 56.f) * S;
      els.push_back(ruleEl(avail * 0.4f, 1.2f * S, accent, 0, 0.55f * F));
      els.push_back(textEl({{main, st(serif(), ts, 700, INK, 1.1f, zh() ? 0.12f * ts : 0)}}, avail, align));
    } else {
      TextStyle ts = st(body(), F, weight, strike ? INK_DIM : stamp ? accent : color, 1.38f, stamp && !zh() ? 0.01f * F : 0);
      if (strike) {
        ts.strike = true;
        ts.strikeColor = rgba(0xc4513f);
        ts.strikeWidth = 2 * S;
      }
      els.push_back(textEl({{main, ts}}, avail, align));
    }
  }
  // Title subline (and, in Chinese, the English title as a small line of type).
  if (s.sub) {
    float ss = 17.f * S;
    els.push_back(textEl({{pairTxt(*s.sub), st(body(), ss, 500, INK_DIM, 1.4f)}}, avail, align, 0.7f * ss));
    if (zh() && title && !s.en.empty())
      els.push_back(textEl({{s.en, st(Font::Serif, 0.8f * ss, 500, INK_DIM, 1.3f, 0.12f * ss, true)}}, avail, align, 0.5f * ss));
    els.push_back(ruleEl(avail * 0.4f, 1.2f * S, accent, 0.7f * ss, 0));
  }
  // Card attribution.
  if (s.attr) {
    float as = 0.58f * F;
    els.push_back(textEl({{pairTxt(*s.attr), st(body(), as, 500, INK_DIM, 1.35f)}}, avail, align, 0.75f * as));
  }
  // Chapter tag.
  if (s.chapter) {
    auto& [num, ttl, czh] = *s.chapter;
    float ns = 0.8f * F;
    els.push_back(textEl({{num, st(body(), ns, 700, rgba(0xb07d12), 1.3f, zh() ? 0.2f * ns : 0.14f * ns, !zh())}}, avail, Align::Left));
    float ts = 1.75f * F;
    els.push_back(textEl({{txt(ttl, czh), st(serif(), ts, 700, INK, 1.2f)}}, avail, Align::Left, 0.1f * ts));
  }
  // Panel: heading, list or chips, footnote.
  if (s.panel) {
    const PanelSpec& P = *s.panel;
    float hs = 0.82f * F;
    El he = textEl({{pairTxt(P.h4), st(body(), hs, 700, accent, 1.3f, 0.1f * hs, !zh())}}, avail, Align::Left);
    he.mb = 0.55f * hs;
    els.push_back(he);
    for (size_t i = 0; i < P.items.size(); i++) {
      const PanelItem& it = P.items[i];
      float ns = 0.78f * F, D = 1.55f * ns, gap = 0.6f * F;
      float txW = avail - D - gap;
      // In Chinese the item and its principle are written with an em dash between them.
      std::vector<Span> tx;
      if (zh()) {
        const std::string dashes = "——";
        size_t dash = it.zh.find(dashes);
        if (dash != std::string::npos) {
          tx.push_back({it.zh.substr(0, dash), st(body(), F, 500, INK)});
          tx.push_back({" · " + it.zh.substr(dash + dashes.size()), st(body(), F, 700, accent)});
        } else {
          tx.push_back({it.zh.empty() ? it.en : it.zh, st(body(), F, 500, INK)});
        }
      } else {
        tx.push_back({it.p.empty() ? it.en : it.en + " ", st(body(), F, 520, INK)});
        if (!it.p.empty()) tx.push_back({"· " + it.p, st(body(), F, 650, accent)});
      }
      auto mainT = std::make_shared<TextBlock>(layoutText(tx, txW, Align::Left));
      TextStyle nst = st(Font::Sans, ns, 700, rgba(0xffffff));
      auto num = std::make_shared<TextBlock>(layoutText(std::to_string(i + 1), nst, 0));
      float circleBase = (D - num->height) / 2 + num->firstBaseline;
      float circleTop = mainT->firstBaseline - circleBase;
      float top = std::min(0.f, circleTop), bottom = std::max(mainT->height, circleTop + D);
      float padY = 0.24f * F;
      El e;
      e.w = D + gap + mainT->width;
      e.h = padY * 2 + (bottom - top);
      e.opacity = P.itemOpacity ? P.itemOpacity((int)i, t) : 1;
      e.paint = [=](Canvas& c, float x, float y, float) {
        float y0 = y + padY - top;
        c.fillCircle(x + D / 2, y0 + circleTop + D / 2, D / 2, accent);
        drawText(c, *num, x + (D - num->width) / 2, y0 + circleTop + (D - num->height) / 2);
        drawText(c, *mainT, x + D + gap, y0);
      };
      els.push_back(e);
    }
    if (!P.chips.empty()) {
      float gap = 0.5f * F, px = 0.75f * F, py = 0.32f * F, bw = 1.2f * S;
      struct Chip { std::shared_ptr<TextBlock> tb; float w, h; };
      std::vector<Chip> chips;
      for (const auto& cp : P.chips) {
        Chip ch;
        ch.tb = std::make_shared<TextBlock>(layoutText(pairTxt(cp), st(body(), F * 1.05f, 700, accent), 0));
        ch.w = ch.tb->width + 2 * px + 2 * bw;
        ch.h = ch.tb->height + 2 * py + 2 * bw;
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
      e.mt = 0.2f * F;
      e.paint = [=](Canvas& c, float x0, float y0, float) {
        for (size_t i = 0; i < chips.size(); i++) {
          const Chip& ch = chips[i];
          float cx = x0 + pos[i].first, cy = y0 + pos[i].second;
          float r = ch.h / 2;
          c.fillRoundRectRing(cx, cy, ch.w, ch.h, r, cx + bw, cy + bw, ch.w - 2 * bw, ch.h - 2 * bw, r - bw, accent);
          drawText(c, *ch.tb, cx + bw + px, cy + bw + py);
        }
      };
      els.push_back(e);
    }
    if (P.foot) {
      float fs = 0.8f * F;
      els.push_back(textEl({{pairTxt(*P.foot), st(body(), fs, 400, INK_DIM, 1.45f)}}, avail, Align::Left, 0.7f * fs));
    }
  }

  auto [cw, chh] = measure(els);
  float w = cw + pl + pr, h = chh + pt + pb;
  if (snap) { w = 793.6f * S; h = 403.2f * S; }

  Layer L;
  int m = (int)std::ceil(24 * S);
  L.canvas.resize((int)std::ceil(w) + 2 * m, (int)std::ceil(h) + 2 * m);
  L.ox = L.oy = m;
  L.w = w;
  L.h = h;
  Canvas& c = L.canvas;
  float X = (float)m, Y = (float)m;
  uint32_t seed = (uint32_t)std::hash<std::string>{}(main + s.cls);
  float r = (card || title ? 4 : 3) * S;
  if (snap) {
    // A photograph's white border.
    float k = 10 * S;
    c.fillRoundRectRing(X, Y, w, h, 2 * S, X + k, Y + k, w - 2 * k, h - 2 * k, 0, rgba(0xfbf8f0));
    L.pinned = false;
  } else {
    RGBA bg = card || title ? PAPER_WARM : PAPER;
    if (plain || note) bg.a = 0.9f;
    paperCard(c, X, Y, w, h, r, bg, S, seed, plain || note ? 0.6f : 1.f);
    if (card || title) {
      // A printed border inside the card.
      float k = 6 * S;
      RGBA line = gold ? rgba(0xc9a24a) : rgba(0xcdbf9f);
      c.fillRoundRectRing(X + k, Y + k, w - 2 * k, h - 2 * k, 2 * S, X + k + 1.2f * S, Y + k + 1.2f * S, w - 2 * k - 2.4f * S, h - 2 * k - 2.4f * S, 1 * S, line);
    }
    if (stamp) {
      // A rubber stamp: a double red border.
      c.fillRoundRectRing(X + 3 * S, Y + 3 * S, w - 6 * S, h - 6 * S, 3 * S, X + 5 * S, Y + 5 * S, w - 10 * S, h - 10 * S, 2 * S, accent);
      c.fillRoundRectRing(X + 7 * S, Y + 7 * S, w - 14 * S, h - 14 * S, 2 * S, X + 8 * S, Y + 8 * S, w - 16 * S, h - 16 * S, 1 * S, withAlpha(accent, 0.7f));
    }
    if (chapter) c.fillRect(X, Y, 5 * S, h, rgba(0xc9a24a));
    // A strip of tape in the role colour holds tags and panels on (cards get a plain one).
    bool tape = !(plain || note || chapter || stamp || title);
    if (tape) {
      float tw = (card || panel ? 54 : 34) * S, th = 11 * S;
      RGBA tc = withAlpha(card ? rgba(0xd8c9a6) : accent, card ? 0.7f : 0.6f);
      float tx = card ? X + (w - tw) / 2 : X + 10 * S;
      c.fillRect(tx, Y - th * 0.55f, tw, th, tc);
    }
  }
  paintStack(c, els, X + pl, Y + pt, cw);
  return L;
}

}  // namespace

struct Labels::Label {
  LabelSpec spec;
  std::string key;
  Layer layer;
};

Labels::Labels(World& W, int width, int height) : W_(W), w_(width), h_(height), zh_(script::zh()), S_(height / 720.f) {}
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
      L.layer = buildLayer(s, S, t);
      L.key = key;
    }
    const Layer& ly = L.layer;
    bool screen = s.screen.has_value();
    vec2 anchor = s.anchor ? *s.anchor : (screen ? vec2{0.5f, 0.5f} : vec2{0.5f, 1});
    bool stem = (s.stem ? *s.stem : !screen) && ly.pinned;

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
    float stemH = stem ? 18 * S : 0;
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
    if (stem && !shifted) {
      // A thread from the tag down to a pin in what it names.
      float sx = std::round(std::min(std::max(x - box.left, 12 * S), box.w - 12 * S));
      fillOnto(frame, w_, h_, px + sx - 0.75f * S, py + ly.h - 2 * S, 1.5f * S, stemH + 2 * S - rise, withAlpha(INK, 0.55f * o));
      Canvas pin((int)std::ceil(12 * S), (int)std::ceil(12 * S));
      float pr = 3.6f * S, cxy = pin.width / 2.f;
      pin.fillCircle(cxy, cxy + 0.8f * S, pr + 0.6f * S, {0, 0, 0, 0.25f});
      pin.fillCircle(cxy, cxy, pr, accentOf(s));
      pin.fillCircle(cxy - 1.1f * S, cxy - 1.1f * S, pr * 0.35f, {1, 1, 1, 0.55f});
      compositeOnto(frame, w_, h_, pin, (int)std::lround(px + sx - cxy), (int)std::lround(y - cxy), o);
    }
    compositeOnto(frame, w_, h_, ly.canvas, px - ly.ox, py - ly.oy, o);
  }
}

// ---------------- captions ----------------
struct Captions::Cache {
  const Cue* cue = nullptr;
  Layer layer;
};

Captions::Captions(int width, int height) : w_(width), h_(height), zh_(script::zh()), S_(height / 720.f), cache_(std::make_shared<Cache>()) {}

SafeArea Captions::safeArea() const {
  // Labels stay clear of the caption band, sized for two lines of captions.
  const float fs = CAP(), capBottom = 46;
  float band = 2 * 1.4f * fs + 18;
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
    float fsz = CAP() * S;
    float pt = 0.3f * fsz, px = 0.75f * fsz, pb = 0.38f * fsz;
    float maxW = (zh_ ? 900 : 980) * S - 2 * px;
    std::vector<El> els;
    els.push_back(textEl({{q->text(), st(body(), fsz, 500, rgba(0xfbf6ea), 1.4f, zh_ ? 0.04f * fsz : 0)}}, maxW, Align::Center));
    auto [cw, chh] = measure(els);
    Layer& L = cache_->layer;
    L.w = cw + 2 * px;
    L.h = chh + pt + pb;
    L.canvas.resize((int)std::ceil(L.w) + 2, (int)std::ceil(L.h) + 2);
    L.ox = L.oy = 1;
    L.canvas.fillRoundRect(1, 1, L.w, L.h, 6 * S, {28 / 255.f, 22 / 255.f, 16 / 255.f, 0.62f});
    paintStack(L.canvas, els, 1 + px, 1 + pt, cw);
  }
  const Layer& L = cache_->layer;
  int x = (int)std::lround((w_ - L.w) / 2), y = (int)std::lround(h_ - 46 * S - L.h);
  compositeOnto(frame, w_, h_, L.canvas, x - L.ox, y - L.oy, o);
}

}  // namespace film
