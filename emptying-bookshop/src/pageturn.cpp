#include "pageturn.h"

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>

#include "kit.h"

namespace film {

namespace {

struct Px {
  float r, g, b;
};

Px fetch(const uint8_t* img, int w, int h, float x, float y) {
  // Bilinear, clamped to the frame.
  x = std::clamp(x - 0.5f, 0.f, (float)w - 1.001f);
  y = std::clamp(y - 0.5f, 0.f, (float)h - 1.001f);
  int x0 = (int)x, y0 = (int)y;
  float fx = x - x0, fy = y - y0;
  const uint8_t* p = img + ((size_t)y0 * w + x0) * 4;
  const uint8_t* q = p + (size_t)w * 4;
  auto mix = [&](int c) {
    float a = p[c] + (p[c + 4] - p[c]) * fx, b = q[c] + (q[c + 4] - q[c]) * fx;
    return (a + (b - a) * fy) / 255.f;
  };
  return {mix(0), mix(1), mix(2)};
}

float hash(int x, int y) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return ((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

}  // namespace

void pageTurn(const uint8_t* page, const uint8_t* under, uint8_t* out, int w, int h, float progress) {
  // The page rolls around a cylinder whose axis sweeps from the bottom-right corner off to the left.
  // d points from the axis toward the part already lifted; s is a pixel's distance from the axis
  // along d, u a point's distance along the page from the axis before it was lifted.
  const float dx = 1, dy = 0.32f, dl = std::sqrt(dx * dx + dy * dy);
  const float Dx = dx / dl, Dy = dy / dl;
  const float R = 0.075f * h * (0.8f + 0.4f * std::sin(PI * progress));
  float a0 = std::max({0.f, w * Dx, h * Dy, w * Dx + h * Dy}) + R;  // the far corner
  float a1 = -PI * R - 0.02f * w;                                     // everything turned and gone
  float e = smoother(progress);
  const float a = a0 + (a1 - a0) * e;
  const RGBA paper = rgba(0xf6efe0);

  // Distance from (qx, qy) along d to the edge of the frame: how far the page reaches beyond the axis.
  auto reach = [&](float qx, float qy) {
    float tx = Dx > 0 ? (w - qx) / Dx : 1e9f, ty = Dy > 0 ? (h - qy) / Dy : 1e9f;
    return std::min(tx, ty);
  };
  // Colour at one sample position (sub-pixel), front to back: the flipped-over part, the cylinder,
  // the flat page, and what lies under it.
  auto sample = [&](float x, float y, Px& c) {
    float s = x * Dx + y * Dy - a;
    float qx = x - s * Dx, qy = y - s * Dy;
    float maxU = reach(qx, qy);
    auto back = [&](float u, float shade) {
      // The back of the sheet: cream, with the print showing faintly through.
      Px f = fetch(page, w, h, qx + u * Dx, qy + u * Dy);
      float l = 0.3f * f.r + 0.59f * f.g + 0.11f * f.b;
      float k = 0.94f + 0.06f * l;
      c = {paper.r * k * shade, paper.g * k * shade, paper.b * k * shade};
    };
    if (s < 0) {
      float u = PI * R - s;  // the part already folded over lies flat on top
      if (u <= maxU) { back(u, 0.97f); return 1.f; }
      c = fetch(page, w, h, x, y);
      // The fold above throws a soft shadow onto the page still lying flat.
      float fold = std::max(0.f, maxU - PI * R);
      float edge = -s - fold;  // distance outside the folded part's far edge
      float sh = edge > 0 ? 0.22f * std::exp(-edge / (0.35f * R)) : 0.f;
      sh = std::max(sh, 0.18f * std::exp(s / (0.5f * R))) * std::min(1.f, progress * 6);
      c = {c.r * (1 - sh), c.g * (1 - sh), c.b * (1 - sh)};
      return 0.f;
    }
    if (s <= R) {
      float th2 = PI - std::asin(std::min(1.f, s / R)), u2 = R * th2;
      if (u2 <= maxU) {
        // The back of the curl, facing up: lighter toward its top.
        back(u2, 0.8f + 0.2f * std::sin(th2));
        return 1.f;
      }
      float th1 = std::asin(std::min(1.f, s / R)), u1 = R * th1;
      if (u1 <= maxU) {
        Px f = fetch(page, w, h, qx + u1 * Dx, qy + u1 * Dy);
        float shade = 0.6f + 0.4f * std::cos(th1);
        c = {f.r * shade, f.g * shade, f.b * shade};
        return 1.f;
      }
    }
    // Revealed: the next picture, in the shadow of the lifted page.
    c = fetch(under, w, h, x, y);
    float sh = 0.42f * std::exp(-std::max(0.f, s - R * 0.6f) / (0.9f * R)) * std::min(1.f, progress * 6) * std::min(1.f, (1 - progress) * 4);
    c = {c.r * (1 - sh), c.g * (1 - sh), c.b * (1 - sh)};
    return 0.f;
  };

  auto rows = [&](int y0, int y1) {
    for (int y = y0; y < y1; y++)
      for (int x = 0; x < w; x++) {
        // Four samples per pixel for smooth edges.
        Px sum{0, 0, 0};
        for (int k = 0; k < 4; k++) {
          Px c;
          sample(x + 0.25f + 0.5f * (k & 1), y + 0.25f + 0.5f * (k >> 1), c);
          sum.r += c.r;
          sum.g += c.g;
          sum.b += c.b;
        }
        uint8_t* o = out + ((size_t)y * w + x) * 4;
        float n = (hash(x, y) - 0.5f) / 255.f;
        o[0] = (uint8_t)std::clamp((int)std::lround((sum.r / 4 + n) * 255), 0, 255);
        o[1] = (uint8_t)std::clamp((int)std::lround((sum.g / 4 + n) * 255), 0, 255);
        o[2] = (uint8_t)std::clamp((int)std::lround((sum.b / 4 + n) * 255), 0, 255);
        o[3] = 255;
      }
  };
  int n = (int)std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
  std::vector<std::thread> pool;
  for (int i = 0; i < n; i++) pool.emplace_back(rows, h * i / n, h * (i + 1) / n);
  for (auto& t : pool) t.join();
}

}  // namespace film
