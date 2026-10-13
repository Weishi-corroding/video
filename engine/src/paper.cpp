#include "paper.h"

#include <algorithm>
#include <cmath>

namespace eng {

namespace {

struct Rand {
  uint32_t s;
  explicit Rand(uint32_t seed) : s(seed * 747796405u + 2891336453u) {}
  float operator()() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (float)(s / 4294967296.0);
  }
};

// Value noise on a periodic lattice of `cells` × `cells`, sampled at (x, y) in 0..1.
float periodicNoise(const std::vector<float>& lattice, int cells, float x, float y) {
  float fx = x * cells, fy = y * cells;
  int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
  float tx = fx - x0, ty = fy - y0;
  tx = tx * tx * (3 - 2 * tx);
  ty = ty * ty * (3 - 2 * ty);
  auto at = [&](int i, int j) { return lattice[((j % cells + cells) % cells) * cells + ((i % cells + cells) % cells)]; };
  float a = at(x0, y0), b = at(x0 + 1, y0), c = at(x0, y0 + 1), d = at(x0 + 1, y0 + 1);
  return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}

void blurWrap(std::vector<float>& img, int n, int radius) {
  // A box blur, horizontal then vertical, wrapping at the edges.
  std::vector<float> tmp(img.size());
  for (int pass = 0; pass < 2; pass++) {
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) {
        float sum = 0;
        for (int k = -radius; k <= radius; k++) {
          int xx = pass == 0 ? (x + k + n) % n : x, yy = pass == 0 ? y : (y + k + n) % n;
          sum += img[yy * n + xx];
        }
        tmp[y * n + x] = sum / (2 * radius + 1);
      }
    img.swap(tmp);
  }
}

}  // namespace

std::vector<uint8_t> paperTexture(int n, uint32_t seed) {
  Rand rnd(seed);
  // Formation: the cloudy unevenness of a sheet, from three octaves of periodic noise.
  std::vector<float> formation(n * n, 0.f);
  float amp = 1, total = 0;
  for (int cells : {4, 9, 20}) {
    std::vector<float> lat(cells * cells);
    for (float& v : lat) v = rnd();
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) formation[y * n + x] += amp * periodicNoise(lat, cells, (x + 0.5f) / n, (y + 0.5f) / n);
    total += amp;
    amp *= 0.5f;
  }
  for (float& v : formation) v = v / total - 0.5f;

  // Fibres: short, gently curving strokes laid in every direction, some lighter and some darker.
  std::vector<float> fibres(n * n, 0.f);
  int count = n * n / 90;
  for (int i = 0; i < count; i++) {
    float x = rnd() * n, y = rnd() * n;
    float a = rnd() * 6.2831853f;
    float len = n * (0.015f + 0.05f * rnd() * rnd());
    float bend = (rnd() - 0.5f) * 0.12f;
    float tone = (rnd() < 0.62f ? 1.f : -0.8f) * (0.4f + 0.6f * rnd());
    float width = rnd() < 0.85f ? 0.7f : 1.3f;
    int steps = (int)(len * 2);
    for (int s = 0; s < steps; s++) {
      a += bend;
      x += std::cos(a) * 0.5f;
      y += std::sin(a) * 0.5f;
      // Splat with a little anti-aliasing into the four nearest texels, wrapping around.
      float fx = x - 0.5f, fy = y - 0.5f;
      int ix = (int)std::floor(fx), iy = (int)std::floor(fy);
      float tx = fx - ix, ty = fy - iy;
      float taper = std::sin(3.14159f * (s + 0.5f) / steps);
      float v = tone * taper * width * 0.5f;
      for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++) {
          float w = (dx ? tx : 1 - tx) * (dy ? ty : 1 - ty);
          int px = ((ix + dx) % n + n) % n, py = ((iy + dy) % n + n) % n;
          fibres[py * n + px] += v * w;
        }
    }
  }
  std::vector<float> soft = fibres;
  blurWrap(soft, n, 1);
  // Grain: fine speckle.
  std::vector<float> grain(n * n);
  for (float& g : grain) g = rnd() - 0.5f;
  blurWrap(grain, n, 1);

  std::vector<uint8_t> out((size_t)n * n * 4);
  for (int i = 0; i < n * n; i++) {
    float colour = 0.55f * formation[i] + 0.35f * soft[i] + 0.25f * grain[i];
    float height = 0.5f + 0.45f * soft[i] + 0.35f * grain[i] + 0.25f * formation[i];
    auto b = [](float v) { return (uint8_t)std::clamp((int)std::lround(v * 255), 0, 255); };
    out[i * 4 + 0] = b(0.5f + colour);
    out[i * 4 + 1] = b(height);
    out[i * 4 + 2] = b(0.5f + grain[i] * 2);
    out[i * 4 + 3] = 255;
  }
  return out;
}

}  // namespace eng
