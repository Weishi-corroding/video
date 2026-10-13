// A small 2D rasteriser for sign textures and the overlay (labels, captions, titles).
// Colours are sRGB with straight alpha; the canvas stores premultiplied values and blends in sRGB
// space, the way browsers composite.
#pragma once
#include <cstdint>
#include <vector>
#include "material.h"

namespace eng {

struct RGBA {
  float r = 0, g = 0, b = 0, a = 0;
};
// 0xRRGGBB with an alpha.
inline RGBA rgba(uint32_t hex, float a = 1) {
  return {((hex >> 16) & 255) / 255.f, ((hex >> 8) & 255) / 255.f, (hex & 255) / 255.f, a};
}
inline RGBA withAlpha(RGBA c, float a) { return {c.r, c.g, c.b, c.a * a}; }

// A coverage mask (0..1) placed at (x, y).
struct Mask {
  int x = 0, y = 0, w = 0, h = 0;
  std::vector<float> a;
  void resize(int x_, int y_, int w_, int h_) { x = x_; y = y_; w = w_; h = h_; a.assign((size_t)w * h, 0.f); }
  // Gaussian blur (three box passes), growing the mask to make room.
  void blur(float sigma);
};

class Canvas {
 public:
  int width = 0, height = 0;
  std::vector<float> px;  // premultiplied r g b a

  Canvas() = default;
  Canvas(int w, int h) { resize(w, h); }
  void resize(int w, int h) { width = w; height = h; px.assign((size_t)w * h * 4, 0.f); }
  void clear(RGBA c = {});

  void fillRect(float x, float y, float w, float h, RGBA c);
  // A stroke centred on the rectangle's edges.
  void strokeRect(float x, float y, float w, float h, float lineWidth, RGBA c);
  void fillRoundRect(float x, float y, float w, float h, float r, RGBA c);
  // The band between a rounded rectangle and an inner one (used for borders).
  void fillRoundRectRing(float x, float y, float w, float h, float r, float ix, float iy, float iw, float ih, float ir, RGBA c);
  void fillCircle(float cx, float cy, float r, RGBA c);
  void fillMask(const Mask& m, int dx, int dy, RGBA c);
  // Composite another canvas at an integer offset.
  void draw(const Canvas& src, int dx, int dy, float opacity = 1);

  // Straight-alpha RGBA8 for a texture.
  void toTexture(Texture* t) const;
  // Rounded-rectangle coverage of pixel (px, py).
  static float roundRectCoverage(float px, float py, float x, float y, float w, float h, float r);

 private:
  void blend(int i, RGBA c, float cov);
};

// Composite a premultiplied layer onto an RGBA8 frame (sRGB, opaque) at an integer offset.
void compositeOnto(uint8_t* frame, int fw, int fh, const Canvas& layer, int dx, int dy, float opacity);
// Blur the frame inside a rounded rectangle (a CSS backdrop blur), mixing by `amount`.
void backdropBlur(uint8_t* frame, int fw, int fh, float x, float y, float w, float h, float r, float sigma, float amount);
// Fill a rectangle on an RGBA8 frame with a colour (straight alpha).
void fillOnto(uint8_t* frame, int fw, int fh, float x, float y, float w, float h, RGBA c);

}  // namespace eng
