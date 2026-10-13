#include "eng/text.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_OUTLINE_H
#include <hb-ot.h>
#include <hb.h>

#include <cmath>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace eng {

namespace {

FT_Library ftLib() {
  static FT_Library lib = [] {
    FT_Library l;
    if (FT_Init_FreeType(&l)) throw std::runtime_error("FreeType failed to initialise");
    return l;
  }();
  return lib;
}

std::map<Font, std::string>& fontFiles() {
  static std::map<Font, std::string> m;
  return m;
}

// One font at one weight: a FreeType face for rasterising and a HarfBuzz font for shaping.
struct Inst {
  Font font;
  int weight;
  FT_Face ft = nullptr;
  hb_font_t* hb = nullptr;
  unsigned upem = 1000;
  float asc = 0.8f, desc = 0.2f;              // in em
  float strikePos = 0.3f, strikeSize = 0.06f;  // in em
  long ftSize = 0;
};
std::vector<Inst>& insts() {
  static std::vector<Inst> v;
  return v;
}

std::map<std::string, hb_face_t*>& hbFaces() {
  static std::map<std::string, hb_face_t*> m;
  return m;
}

bool registered(Font f) { return fontFiles().count(f) > 0; }

// The file to use for a family at a weight: bold Chinese faces, and Chinese serif falling back to sans.
Font resolve(Font f, float weight) {
  bool bold = weight >= 600;
  if (f == Font::CJKSerif && !registered(Font::CJKSerif)) f = Font::CJK;
  if (f == Font::CJKSerifBold && !registered(Font::CJKSerifBold)) f = registered(Font::CJKSerif) ? Font::CJKSerif : Font::CJK;
  if (f == Font::CJKBold && !registered(Font::CJKBold)) f = Font::CJK;
  if (bold && f == Font::CJK && registered(Font::CJKBold)) return Font::CJKBold;
  if (bold && f == Font::CJKSerif && registered(Font::CJKSerifBold)) return Font::CJKSerifBold;
  return f;
}

bool isCJKFamily(Font f) { return f == Font::CJK || f == Font::CJKBold || f == Font::CJKSerif || f == Font::CJKSerifBold; }

int instance(Font f, float weightF) {
  f = resolve(f, weightF);
  int weight = (int)std::lround(weightF);
  auto& v = insts();
  for (size_t i = 0; i < v.size(); i++)
    if (v[i].font == f && v[i].weight == weight) return (int)i;
  auto it = fontFiles().find(f);
  if (it == fontFiles().end()) throw std::runtime_error("no font file registered for a family in use");
  const std::string& path = it->second;
  Inst in;
  in.font = f;
  in.weight = weight;
  if (FT_New_Face(ftLib(), path.c_str(), 0, &in.ft)) throw std::runtime_error("cannot open font " + path);
  hb_face_t*& face = hbFaces()[path];
  if (!face) {
    hb_blob_t* blob = hb_blob_create_from_file_or_fail(path.c_str());
    if (!blob) throw std::runtime_error("cannot read font " + path);
    face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);
  }
  in.upem = hb_face_get_upem(face);
  in.hb = hb_font_create(face);
  hb_font_set_scale(in.hb, (int)in.upem, (int)in.upem);
  // Variable fonts: set the weight axis for both shaper and rasteriser.
  hb_variation_t var{HB_TAG('w', 'g', 'h', 't'), (float)weight};
  hb_font_set_variations(in.hb, &var, 1);
  FT_MM_Var* mm = nullptr;
  if ((in.ft->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS) && FT_Get_MM_Var(in.ft, &mm) == 0) {
    std::vector<FT_Fixed> coords(mm->num_axis);
    for (FT_UInt a = 0; a < mm->num_axis; a++) {
      coords[a] = mm->axis[a].def;
      if (mm->axis[a].tag == FT_MAKE_TAG('w', 'g', 'h', 't'))
        coords[a] = (FT_Fixed)(std::clamp((float)weight, mm->axis[a].minimum / 65536.f, mm->axis[a].maximum / 65536.f) * 65536);
    }
    FT_Set_Var_Design_Coordinates(in.ft, mm->num_axis, coords.data());
    FT_Done_MM_Var(ftLib(), mm);
  }
  hb_font_extents_t ext;
  if (hb_font_get_h_extents(in.hb, &ext)) {
    in.asc = ext.ascender / (float)in.upem;
    in.desc = -ext.descender / (float)in.upem;
  }
  hb_position_t pos;
  if (hb_ot_metrics_get_position(in.hb, HB_OT_METRICS_TAG_STRIKEOUT_OFFSET, &pos)) in.strikePos = pos / (float)in.upem;
  if (hb_ot_metrics_get_position(in.hb, HB_OT_METRICS_TAG_STRIKEOUT_SIZE, &pos)) in.strikeSize = pos / (float)in.upem;
  v.push_back(in);
  return (int)v.size() - 1;
}

bool hasGlyph(int inst, uint32_t cp) {
  hb_codepoint_t g;
  return hb_font_get_nominal_glyph(insts()[inst].hb, cp, &g) && g != 0;
}

// ---- UTF-8 and character classes ----
std::vector<uint32_t> decode(const std::string& s) {
  std::vector<uint32_t> out;
  for (size_t i = 0; i < s.size();) {
    unsigned char c = s[i];
    uint32_t cp;
    int n;
    if (c < 0x80) { cp = c; n = 1; }
    else if ((c >> 5) == 6) { cp = c & 0x1f; n = 2; }
    else if ((c >> 4) == 14) { cp = c & 0x0f; n = 3; }
    else { cp = c & 0x07; n = 4; }
    for (int k = 1; k < n && i + k < s.size(); k++) cp = (cp << 6) | (s[i + k] & 0x3f);
    out.push_back(cp);
    i += n;
  }
  return out;
}

bool isWide(uint32_t c) {
  return (c >= 0x2E80 && c <= 0x9FFF) || (c >= 0xF900 && c <= 0xFAFF) || (c >= 0xFF00 && c <= 0xFFEF) || c >= 0x20000;
}
bool noBreakBefore(uint32_t c) {
  switch (c) {
    case 0xFF0C: case 0x3002: case 0x3001: case 0xFF1B: case 0xFF1A: case 0xFF01: case 0xFF1F: case 0xFF09:
    case 0x300D: case 0x300F: case 0x201D: case 0x2019: case 0x300B: case 0x3009: case 0x2026: case 0x2014:
    case ')': case ',': case '.': case ';': case ':': case '!': case '?': case '%':
      return true;
  }
  return false;
}
bool noBreakAfter(uint32_t c) {
  switch (c) {
    case 0xFF08: case 0x300C: case 0x300E: case 0x201C: case 0x2018: case 0x300A: case 0x3008: case '(':
      return true;
  }
  return false;
}
bool isSpace(uint32_t c) { return c == ' ' || c == '\t'; }
uint32_t upper(uint32_t c) {
  if (c >= 'a' && c <= 'z') return c - 32;
  if (c >= 0xE0 && c <= 0xFE && c != 0xF7) return c - 32;
  return c;
}

// ---- shaping ----
struct Shaped {
  struct G { int inst; unsigned id; float x, y, adv; };
  std::vector<G> glyphs;
  float width = 0;
};

// Shape a short run, choosing a font per character (the style's font, then the fallback).
Shaped shape(const std::vector<uint32_t>& cps, const TextStyle& st) {
  Shaped out;
  if (cps.empty()) return out;
  int primary = instance(st.font, st.weight);
  Font fb = isCJKFamily(st.font) ? (st.font == Font::CJKSerif || st.font == Font::CJKSerifBold ? Font::Serif : Font::Sans)
            : st.font == Font::Sans ? Font::CJK : Font::CJKSerif;
  int fallback = instance(fb, st.weight);
  size_t i = 0;
  float pen = 0;
  while (i < cps.size()) {
    int use = hasGlyph(primary, cps[i]) || !hasGlyph(fallback, cps[i]) ? primary : fallback;
    size_t j = i + 1;
    while (j < cps.size()) {
      int u = hasGlyph(primary, cps[j]) || !hasGlyph(fallback, cps[j]) ? primary : fallback;
      if (u != use) break;
      j++;
    }
    Inst& in = insts()[use];
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf32(buf, cps.data() + i, (int)(j - i), 0, (int)(j - i));
    hb_buffer_guess_segment_properties(buf);
    int scale = (int)std::lround(st.size * 64);
    hb_font_set_scale(in.hb, scale, scale);
    hb_shape(in.hb, buf, nullptr, 0);
    unsigned n;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &n);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &n);
    for (unsigned k = 0; k < n; k++) {
      float adv = pos[k].x_advance / 64.f;
      bool lastOfCluster = k + 1 == n || info[k + 1].cluster != info[k].cluster;
      if (lastOfCluster) adv += st.letterSpacing;
      out.glyphs.push_back({use, info[k].codepoint, pen + pos[k].x_offset / 64.f, -pos[k].y_offset / 64.f, adv});
      pen += adv;
    }
    hb_font_set_scale(in.hb, (int)in.upem, (int)in.upem);
    hb_buffer_destroy(buf);
    i = j;
  }
  out.width = pen;
  return out;
}

struct Metrics { float a, d; };  // above and below the baseline within a line box
Metrics lineMetrics(const TextStyle& st) {
  const Inst& in = insts()[instance(st.font, st.weight)];
  float asc = in.asc * st.size, desc = in.desc * st.size;
  float lead = st.size * st.lineHeight - (asc + desc);
  return {asc + lead / 2, desc + lead / 2};
}

// ---- glyph bitmaps ----
struct Bitmap { int w = 0, h = 0, left = 0, top = 0; std::vector<uint8_t> a; };
const Bitmap& glyphBitmap(int inst, unsigned id, float size, int sub) {
  static std::unordered_map<uint64_t, Bitmap> cache;
  long sz = std::lround(size * 64);
  uint64_t key = ((uint64_t)inst << 56) ^ ((uint64_t)id << 32) ^ ((uint64_t)(sz & 0xffffff) << 4) ^ (uint64_t)sub;
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  Bitmap& bm = cache[key];
  Inst& in = insts()[inst];
  if (in.ftSize != sz) {
    FT_Set_Char_Size(in.ft, 0, sz, 72, 72);
    in.ftSize = sz;
  }
  if (FT_Load_Glyph(in.ft, id, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP)) return bm;
  FT_GlyphSlot slot = in.ft->glyph;
  if (slot->format == FT_GLYPH_FORMAT_OUTLINE) FT_Outline_Translate(&slot->outline, sub * 16, 0);
  if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL)) return bm;
  const FT_Bitmap& b = slot->bitmap;
  bm.w = (int)b.width;
  bm.h = (int)b.rows;
  bm.left = slot->bitmap_left;
  bm.top = slot->bitmap_top;
  bm.a.resize((size_t)bm.w * bm.h);
  for (int y = 0; y < bm.h; y++)
    for (int x = 0; x < bm.w; x++) bm.a[(size_t)y * bm.w + x] = b.buffer[y * b.pitch + x];
  return bm;
}

template <class F>
void forEachPixel(const TextBlock& b, float ox, float oy, F&& f) {
  for (const auto& g : b.glyphs) {
    float gx = ox + g.x, gy = std::round(oy + g.y);
    float fx = std::floor(gx);
    int sub = std::min(3, (int)((gx - fx) * 4));
    const Bitmap& bm = glyphBitmap(g.inst, g.id, g.size, sub);
    int x0 = (int)fx + bm.left, y0 = (int)gy - bm.top;
    for (int y = 0; y < bm.h; y++)
      for (int x = 0; x < bm.w; x++) {
        uint8_t a = bm.a[(size_t)y * bm.w + x];
        if (a) f(x0 + x, y0 + y, a / 255.f, g.color);
      }
  }
}

}  // namespace

void setFontFile(Font f, const std::string& path) { fontFiles()[f] = path; }

float fontAscent(const TextStyle& s) {
  const Inst& in = insts()[instance(s.font, s.weight)];
  return in.asc * s.size;
}
float fontDescent(const TextStyle& s) {
  const Inst& in = insts()[instance(s.font, s.weight)];
  return in.desc * s.size;
}

float textWidth(const std::string& text, const TextStyle& s) {
  std::vector<uint32_t> cps = decode(text);
  if (s.uppercase) for (auto& c : cps) c = upper(c);
  return shape(cps, s).width;
}

TextBlock layoutText(const std::vector<Span>& spans, float maxWidth, Align align) {
  // Break each span into tokens: a word (or one Chinese character) plus the spaces that follow it.
  struct Token { int span; Shaped word; float spaceW; float lead; };
  std::vector<Token> tokens;
  for (int si = 0; si < (int)spans.size(); si++) {
    const TextStyle& st = spans[si].style;
    std::vector<uint32_t> cps = decode(spans[si].text);
    if (st.uppercase) for (auto& c : cps) c = upper(c);
    float spaceW = shape({' '}, st).width;
    float lead = spans[si].marginLeft;
    size_t i = 0;
    while (i < cps.size()) {
      size_t j = i;
      std::vector<uint32_t> word;
      while (j < cps.size() && !isSpace(cps[j])) {
        word.push_back(cps[j]);
        j++;
        if (j >= cps.size() || isSpace(cps[j])) break;
        uint32_t a = cps[j - 1], b = cps[j];
        bool brk = false;
        if ((isWide(a) || isWide(b)) && !noBreakBefore(b) && !noBreakAfter(a)) brk = true;
        if ((a == '-' || a == 0x2013 || a == 0x2014) && j >= 2 && !isSpace(cps[j - 2]) && b != '-' && !noBreakBefore(b)) brk = true;
        if (brk) break;
      }
      int spaces = 0;
      while (j < cps.size() && isSpace(cps[j])) { spaces++; j++; }
      tokens.push_back({si, shape(word, st), spaceW * spaces, lead});
      lead = 0;
      i = j;
    }
  }

  // Greedy line filling.
  struct Line { int first, last; float width; };
  std::vector<Line> lines;
  float maxContent = 0, lineW = 0;
  int start = 0;
  bool wrapped = false;
  for (int k = 0; k < (int)tokens.size(); k++) {
    float w = tokens[k].word.width + tokens[k].lead;
    if (maxWidth > 0 && k > start && lineW + w > maxWidth + 0.01f) {
      float trimmed = lineW - tokens[k - 1].spaceW;
      lines.push_back({start, k - 1, trimmed});
      maxContent = std::max(maxContent, trimmed);
      start = k;
      lineW = 0;
      wrapped = true;
    }
    lineW += w + tokens[k].spaceW;
  }
  if (!tokens.empty()) {
    float trimmed = lineW - tokens.back().spaceW;
    lines.push_back({start, (int)tokens.size() - 1, trimmed});
    maxContent = std::max(maxContent, trimmed);
  }

  TextBlock out;
  out.width = wrapped ? maxWidth : maxContent;
  out.lines = (int)lines.size();
  const TextStyle& strut = spans.empty() ? TextStyle{} : spans[0].style;
  float y = 0;
  if (lines.empty() && !spans.empty()) {
    Metrics m = lineMetrics(strut);
    out.height = m.a + m.d;
    out.firstBaseline = m.a;
    return out;
  }
  for (size_t li = 0; li < lines.size(); li++) {
    const Line& L = lines[li];
    Metrics m = lineMetrics(strut);
    for (int k = L.first; k <= L.last; k++) {
      Metrics s = lineMetrics(spans[tokens[k].span].style);
      m.a = std::max(m.a, s.a);
      m.d = std::max(m.d, s.d);
    }
    float base = y + m.a;
    if (li == 0) out.firstBaseline = base;
    float x = align == Align::Center ? (out.width - L.width) / 2 : 0;
    // Strike-through runs per span on this line.
    int strikeSpan = -1;
    float strikeX0 = 0, strikeX1 = 0;
    auto flushStrike = [&]() {
      if (strikeSpan < 0) return;
      const TextStyle& st = spans[strikeSpan].style;
      const Inst& in = insts()[instance(st.font, st.weight)];
      float hgt = st.strikeWidth;
      out.rules.push_back({strikeX0, base - in.strikePos * st.size - hgt / 2, strikeX1 - strikeX0, hgt, st.strikeColor});
      strikeSpan = -1;
    };
    for (int k = L.first; k <= L.last; k++) {
      const Token& t = tokens[k];
      const TextStyle& st = spans[t.span].style;
      x += t.lead;
      for (const auto& g : t.word.glyphs) out.glyphs.push_back({g.inst, g.id, x + g.x, base + g.y, st.size, st.color});
      if (st.strike) {
        if (strikeSpan != t.span) { flushStrike(); strikeSpan = t.span; strikeX0 = x; }
        strikeX1 = x + t.word.width;
      } else {
        flushStrike();
      }
      x += t.word.width + (k < L.last ? t.spaceW : 0);
    }
    flushStrike();
    y += m.a + m.d;
  }
  out.height = y;
  return out;
}

void drawText(Canvas& c, const TextBlock& b, float x, float y) {
  forEachPixel(b, x, y, [&](int px, int py, float a, const RGBA& col) {
    if (px < 0 || py < 0 || px >= c.width || py >= c.height) return;
    float al = a * col.a, k = 1 - al;
    float* p = &c.px[((size_t)py * c.width + px) * 4];
    p[0] = col.r * al + p[0] * k;
    p[1] = col.g * al + p[1] * k;
    p[2] = col.b * al + p[2] * k;
    p[3] = al + p[3] * k;
  });
  for (const auto& r : b.rules) c.fillRect(x + r.x, y + r.y, r.w, r.h, r.color);
}

Mask textMask(const TextBlock& b, float x, float y) {
  // Bounds first, then coverage.
  int x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30);
  forEachPixel(b, x, y, [&](int px, int py, float, const RGBA&) {
    x0 = std::min(x0, px); y0 = std::min(y0, py); x1 = std::max(x1, px + 1); y1 = std::max(y1, py + 1);
  });
  Mask m;
  if (x1 <= x0) return m;
  m.resize(x0, y0, x1 - x0, y1 - y0);
  forEachPixel(b, x, y, [&](int px, int py, float a, const RGBA&) {
    float& d = m.a[(size_t)(py - y0) * m.w + (px - x0)];
    d = d + a * (1 - d);
  });
  return m;
}

}  // namespace eng
