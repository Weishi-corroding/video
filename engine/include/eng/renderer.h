// Offscreen Vulkan renderer: sun shadows, a multisampled HDR scene pass with a normal and depth
// buffer, ambient occlusion, depth of field, bloom, tone mapping and a grade, read back as RGBA8 for
// encoding.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "scene.h"

namespace eng {

struct PostSettings {
  // Bloom from pixels brighter than the threshold, blurred at five scales.
  float bloomStrength = 0, bloomRadius = 0, bloomThreshold = 1;
  // Grade, applied after tone mapping.
  float saturation = 1;
  float vignette = 0;
  float fade = 0;   // to black
  float flash = 0;  // to white
  // Everything outside a circle is dimmed and greyed by `focusAmount`.
  vec2 focusCenter{0.5f, 0.5f};  // 0..1, y down
  float focusRadius = 0.25f, focusAmount = 0;
  float dither = 1;  // amount of noise added before quantising to 8 bits

  // Screen-space ambient occlusion: darkens creases and contact points.
  float aoStrength = 0;  // 0 = off
  float aoRadius = 0.6f; // world units near the camera (grows slowly with distance)
  float aoPower = 1.4f;

  // Depth of field: sharp at `dofFocus` (distance along the view), blurring with distance from it.
  float dofFocus = 10;
  float dofAperture = 0;    // blur radius, in pixels at 1080p, of something infinitely far away (0 = off)
  float dofMaxRadius = 18;  // largest blur radius, in pixels at 1080p

  // Final grade in display space: contrast around mid grey, then gain and a lift of the shadows.
  float contrast = 1;
  vec3 gain{1, 1, 1};
  vec3 lift{0, 0, 0};
};

struct RendererOptions {
  int width = 1920, height = 1080;
  int samples = 4;          // MSAA
  int shadowMapSize = 2048;
  bool validation = false;  // Khronos validation layer, if installed
};

class Renderer {
 public:
  explicit Renderer(const RendererOptions& options);
  ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  // Render a frame. The camera's matrices are updated from its fields.
  void render(Scene& scene, Camera& camera, const PostSettings& post);
  // The last frame as RGBA8 (sRGB), top row first, width * height * 4 bytes.
  const uint8_t* pixels() const;
  int width() const;
  int height() const;
  std::string deviceName() const;

  struct Stats {
    int draws = 0;
    int shadowDraws = 0;
    size_t triangles = 0;
  };
  const Stats& stats() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace eng
