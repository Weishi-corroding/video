#include "eng/canvas.h"

#include <algorithm>
#include <cmath>

namespace eng {

namespace {

// Box sizes whose three passes approximate a Gaussian of the given sigma.
void gaussBoxes(float sigma, int out[3]) {
  const int n = 3;
  float wIdeal = std::sqrt(12 * sigma * sigma / n + 1);
  int wl = (int)std::floor(wIdeal);
  if (wl % 2 == 0) wl--;
  int wu = wl + 2;
  float mIdeal = (12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4.f * wl - 4);
  int m = (int)std::lround(mIdeal);
  for (int i = 0; i < n; i++) out[i] = std::max(1, i < m ? wl : wu);
}

// One box pass along a line of `n` values spaced by `stride`. Outside the line counts as zero, or as
// the edge value when `clampEdge` is set.
void boxLine(float* p, int n, int stride, int radius, bool clampEdge, std::vector<float>& tmp) {
  if (radius <= 0 || n <= 0) return;
  tmp.resize(n);
  for (int i = 0; i < n; i++) tmp[i] = p[i * stride];
  auto at = [&](int i) -> float {
    if (i < 0) return clampEdge ? tmp[0] : 0.f;
    if (i >= n) return clampEdge ? tmp[n - 1] : 0.f;
    return tmp[i];
  };
  float inv = 1.f / (2 * radius + 1);
  float sum = 0;
  for (int i = -radius; i <= radius; i++) sum += at(i);
  for (int i = 0; i < n; i++) {
    p[i * stride] = sum * inv;
    sum += at(i + radius + 1) - at(i - radius);
  }
}

void blurPlanes(std::vector<float*>& planes, int w, int h, float sigma, bool clampEdge) {
  int boxes[3];
  gaussBoxes(sigma, boxes);
  std::vector<float> tmp;
  for (float* p : planes)
    for (int b = 0; b < 3; b++) {
      int r = (boxes[b] - 1) / 2;
      for (int y = 0; y < h; y++) boxLine(p + (size_t)y * w, w, 1, r, clampEdge, tmp);
      for (int x = 0; x < w; x++) boxLine(p + x, h, w, r, clampEdge, tmp);
    }
}

inline float sdRoundRect(float px, float py, float cx, float cy, float hx, float hy, float r) {
  float qx = std::fabs(px - cx) - hx + r, qy = std::fabs(py - cy) - hy + r;
  float ox = std::max(qx, 0.f), oy = std::max(qy, 0.f);
  return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.f) - r;
}

inline uint8_t to8(float v) { return (uint8_t)std::lround(clamp(v, 0.f, 1.f) * 255.f); }

}  // namespace

void Mask::blur(float sigma) {
  if (sigma <= 0.1f || w == 0) return;
  int pad = (int)std::ceil(sigma * 3);
  std::vector<float> g((size_t)(w + 2 * pad) * (h + 2 * pad), 0.f);
  int W = w + 2 * pad, H = h + 2 * pad;
  for (int yy = 0; yy < h; yy++) std::copy(a.begin() + (size_t)yy * w, a.begin() + (size_t)(yy + 1) * w, g.begin() + (size_t)(yy + pad) * W + pad);
  std::vector<float*> planes{g.data()};
  blurPlanes(planes, W, H, sigma, false);
  a.swap(g);
  x -= pad; y -= pad; w = W; h = H;
}

float Canvas::roundRectCoverage(float px, float py, float x, float y, float w, float h, float r) {
  r = std::min({r, w / 2, h / 2});
  float d = sdRoundRect(px, py, x + w / 2, y + h / 2, w / 2, h / 2, std::max(r, 0.f));
  return clamp(0.5f - d);
}

void Canvas::clear(RGBA c) {
  for (size_t i = 0; i < px.size(); i += 4) {
    px[i] = c.r * c.a; px[i + 1] = c.g * c.a; px[i + 2] = c.b * c.a; px[i + 3] = c.a;
  }
}

void Canvas::blend(int i, RGBA c, float cov) {
  float a = c.a * cov;
  if (a <= 0) return;
  float k = 1 - a;
  float* p = &px[(size_t)i * 4];
  p[0] = c.r * a + p[0] * k;
  p[1] = c.g * a + p[1] * k;
  p[2] = c.b * a + p[2] * k;
  p[3] = a + p[3] * k;
}

void Canvas::fillRect(float x, float y, float w, float h, RGBA c) {
  int x0 = std::max(0, (int)std::floor(x)), x1 = std::min(width, (int)std::ceil(x + w));
  int y0 = std::max(0, (int)std::floor(y)), y1 = std::min(height, (int)std::ceil(y + h));
  for (int yy = y0; yy < y1; yy++) {
    float cy = std::min(y + h, yy + 1.f) - std::max(y, (float)yy);
    if (cy <= 0) continue;
    for (int xx = x0; xx < x1; xx++) {
      float cx = std::min(x + w, xx + 1.f) - std::max(x, (float)xx);
      if (cx > 0) blend(yy * width + xx, c, cx * cy);
    }
  }
}

void Canvas::strokeRect(float x, float y, float w, float h, float lw, RGBA c) {
  float hl = lw / 2;
  fillRect(x - hl, y - hl, w + lw, lw, c);
  fillRect(x - hl, y + h - hl, w + lw, lw, c);
  fillRect(x - hl, y + hl, lw, h - lw, c);
  fillRect(x + w - hl, y + hl, lw, h - lw, c);
}

void Canvas::fillRoundRect(float x, float y, float w, float h, float r, RGBA c) {
  int x0 = std::max(0, (int)std::floor(x)), x1 = std::min(width, (int)std::ceil(x + w));
  int y0 = std::max(0, (int)std::floor(y)), y1 = std::min(height, (int)std::ceil(y + h));
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) {
      float cov = roundRectCoverage(xx + 0.5f, yy + 0.5f, x, y, w, h, r);
      if (cov > 0) blend(yy * width + xx, c, cov);
    }
}

void Canvas::fillRoundRectRing(float x, float y, float w, float h, float r, float ix, float iy, float iw, float ih, float ir, RGBA c) {
  int x0 = std::max(0, (int)std::floor(x)), x1 = std::min(width, (int)std::ceil(x + w));
  int y0 = std::max(0, (int)std::floor(y)), y1 = std::min(height, (int)std::ceil(y + h));
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) {
      float px_ = xx + 0.5f, py_ = yy + 0.5f;
      float cov = roundRectCoverage(px_, py_, x, y, w, h, r);
      if (cov <= 0) continue;
      cov *= 1 - (iw > 0 && ih > 0 ? roundRectCoverage(px_, py_, ix, iy, iw, ih, ir) : 0.f);
      if (cov > 0) blend(yy * width + xx, c, cov);
    }
}

void Canvas::fillCircle(float cx, float cy, float r, RGBA c) {
  int x0 = std::max(0, (int)std::floor(cx - r - 1)), x1 = std::min(width, (int)std::ceil(cx + r + 1));
  int y0 = std::max(0, (int)std::floor(cy - r - 1)), y1 = std::min(height, (int)std::ceil(cy + r + 1));
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) {
      float d = std::hypot(xx + 0.5f - cx, yy + 0.5f - cy) - r;
      float cov = clamp(0.5f - d);
      if (cov > 0) blend(yy * width + xx, c, cov);
    }
}

void Canvas::fillMask(const Mask& m, int dx, int dy, RGBA c) {
  for (int yy = 0; yy < m.h; yy++) {
    int ty = m.y + yy + dy;
    if (ty < 0 || ty >= height) continue;
    for (int xx = 0; xx < m.w; xx++) {
      int tx = m.x + xx + dx;
      if (tx < 0 || tx >= width) continue;
      float a = m.a[(size_t)yy * m.w + xx];
      if (a > 0) blend(ty * width + tx, c, a);
    }
  }
}

void Canvas::draw(const Canvas& src, int dx, int dy, float opacity) {
  for (int yy = 0; yy < src.height; yy++) {
    int ty = yy + dy;
    if (ty < 0 || ty >= height) continue;
    for (int xx = 0; xx < src.width; xx++) {
      int tx = xx + dx;
      if (tx < 0 || tx >= width) continue;
      const float* s = &src.px[((size_t)yy * src.width + xx) * 4];
      float a = s[3] * opacity;
      if (a <= 0) continue;
      float* d = &px[((size_t)ty * width + tx) * 4];
      float k = 1 - a;
      d[0] = s[0] * opacity + d[0] * k;
      d[1] = s[1] * opacity + d[1] * k;
      d[2] = s[2] * opacity + d[2] * k;
      d[3] = a + d[3] * k;
    }
  }
}

void Canvas::toTexture(Texture* t) const {
  t->width = width;
  t->height = height;
  t->pixels.resize((size_t)width * height * 4);
  for (size_t i = 0; i < (size_t)width * height; i++) {
    const float* p = &px[i * 4];
    float a = p[3];
    uint8_t* o = &t->pixels[i * 4];
    if (a <= 1e-5f) { o[0] = o[1] = o[2] = o[3] = 0; continue; }
    o[0] = to8(p[0] / a); o[1] = to8(p[1] / a); o[2] = to8(p[2] / a); o[3] = to8(a);
  }
  t->touch();
}

void compositeOnto(uint8_t* frame, int fw, int fh, const Canvas& layer, int dx, int dy, float opacity) {
  if (opacity <= 0) return;
  for (int yy = 0; yy < layer.height; yy++) {
    int ty = yy + dy;
    if (ty < 0 || ty >= fh) continue;
    for (int xx = 0; xx < layer.width; xx++) {
      int tx = xx + dx;
      if (tx < 0 || tx >= fw) continue;
      const float* s = &layer.px[((size_t)yy * layer.width + xx) * 4];
      float a = s[3] * opacity;
      if (a <= 0) continue;
      uint8_t* d = &frame[((size_t)ty * fw + tx) * 4];
      float k = 1 - a;
      d[0] = to8(s[0] * opacity + d[0] / 255.f * k);
      d[1] = to8(s[1] * opacity + d[1] / 255.f * k);
      d[2] = to8(s[2] * opacity + d[2] / 255.f * k);
    }
  }
}

void backdropBlur(uint8_t* frame, int fw, int fh, float x, float y, float w, float h, float r, float sigma, float amount) {
  if (amount <= 0 || sigma <= 0) return;
  int pad = (int)std::ceil(sigma * 3);
  int x0 = std::max(0, (int)std::floor(x) - pad), x1 = std::min(fw, (int)std::ceil(x + w) + pad);
  int y0 = std::max(0, (int)std::floor(y) - pad), y1 = std::min(fh, (int)std::ceil(y + h) + pad);
  int W = x1 - x0, H = y1 - y0;
  if (W <= 0 || H <= 0) return;
  std::vector<float> ch[3];
  for (auto& c : ch) c.resize((size_t)W * H);
  for (int yy = 0; yy < H; yy++)
    for (int xx = 0; xx < W; xx++) {
      const uint8_t* p = &frame[((size_t)(yy + y0) * fw + xx + x0) * 4];
      for (int k = 0; k < 3; k++) ch[k][(size_t)yy * W + xx] = p[k] / 255.f;
    }
  std::vector<float*> planes{ch[0].data(), ch[1].data(), ch[2].data()};
  blurPlanes(planes, W, H, sigma, true);
  for (int yy = 0; yy < H; yy++)
    for (int xx = 0; xx < W; xx++) {
      float cov = Canvas::roundRectCoverage(xx + x0 + 0.5f, yy + y0 + 0.5f, x, y, w, h, r) * amount;
      if (cov <= 0) continue;
      uint8_t* p = &frame[((size_t)(yy + y0) * fw + xx + x0) * 4];
      for (int k = 0; k < 3; k++) p[k] = to8(lerp(p[k] / 255.f, ch[k][(size_t)yy * W + xx], cov));
    }
}

void fillOnto(uint8_t* frame, int fw, int fh, float x, float y, float w, float h, RGBA c) {
  int x0 = std::max(0, (int)std::floor(x)), x1 = std::min(fw, (int)std::ceil(x + w));
  int y0 = std::max(0, (int)std::floor(y)), y1 = std::min(fh, (int)std::ceil(y + h));
  for (int yy = y0; yy < y1; yy++) {
    float cy = std::min(y + h, yy + 1.f) - std::max(y, (float)yy);
    for (int xx = x0; xx < x1; xx++) {
      float cx = std::min(x + w, xx + 1.f) - std::max(x, (float)xx);
      float a = c.a * cx * cy;
      if (a <= 0) continue;
      uint8_t* p = &frame[((size_t)yy * fw + xx) * 4];
      p[0] = to8(lerp(p[0] / 255.f, c.r, a));
      p[1] = to8(lerp(p[1] / 255.f, c.g, a));
      p[2] = to8(lerp(p[2] / 255.f, c.b, a));
    }
  }
}

}  // namespace eng
