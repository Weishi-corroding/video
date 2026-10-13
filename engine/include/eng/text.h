// Text: shaping with HarfBuzz, rasterising with FreeType, and paragraph layout with line breaking
// for Latin and Chinese text.
#pragma once
#include <string>
#include <vector>
#include "canvas.h"

namespace eng {

// Latin families, and Chinese ones. A Chinese family asked for at weight 600 or more uses its bold
// file if one is registered. Characters a font lacks come from its counterpart: Sans <-> CJK,
// Serif -> CJKSerif.
enum class Font { Sans, Serif, SerifItalic, CJK, CJKBold, CJKSerif, CJKSerifBold };

// Register the font file for a family. Variable fonts get their weight from the style.
void setFontFile(Font f, const std::string& path);

struct TextStyle {
  Font font = Font::Sans;
  float size = 16;  // pixels
  float weight = 400;
  RGBA color{1, 1, 1, 1};
  float letterSpacing = 0;  // pixels added after each character
  bool uppercase = false;
  float lineHeight = 1.3f;  // multiple of the size
  bool strike = false;
  RGBA strikeColor{1, 1, 1, 1};
  float strikeWidth = 1;
};

// A run of text with one style. Several spans flow together into one paragraph.
struct Span {
  std::string text;
  TextStyle style;
  float marginLeft = 0;  // space before the span, in pixels
};

enum class Align { Left, Center };

struct TextBlock {
  struct Glyph {
    int inst;
    unsigned id;
    float x, y;  // pen position on the baseline
    float size;
    RGBA color;
  };
  struct Rule {
    float x, y, w, h;
    RGBA color;
  };
  std::vector<Glyph> glyphs;
  std::vector<Rule> rules;
  // The box: shrink-to-fit width (the full available width once any line wraps) and total height.
  float width = 0, height = 0;
  int lines = 0;
  float firstBaseline = 0;
};

// Lay out spans as one paragraph. maxWidth <= 0 means no wrapping.
TextBlock layoutText(const std::vector<Span>& spans, float maxWidth, Align align = Align::Left);
inline TextBlock layoutText(const std::string& text, const TextStyle& s, float maxWidth, Align align = Align::Left) {
  return layoutText(std::vector<Span>{{text, s}}, maxWidth, align);
}
// Width of a single unwrapped line.
float textWidth(const std::string& text, const TextStyle& s);
// Draw a block with its top-left corner at (x, y).
void drawText(Canvas& c, const TextBlock& b, float x, float y);
// The block's coverage, for shadows. The mask is placed in canvas coordinates.
Mask textMask(const TextBlock& b, float x, float y);
// Font metrics in pixels for a style: distance from the top of a line box to the baseline, and line height.
float fontAscent(const TextStyle& s);
float fontDescent(const TextStyle& s);

}  // namespace eng
