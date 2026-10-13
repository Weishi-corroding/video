// Materials and textures.
#pragma once
#include <cstdint>
#include <vector>
#include "math.h"

namespace eng {

// An RGBA8 image, straight alpha, sRGB-encoded by default. Call touch() after changing the pixels.
struct Texture {
  int width = 0, height = 0;
  std::vector<uint8_t> pixels;
  bool srgb = true;
  bool mipmaps = true;
  uint32_t version = 1;
  int gpuSlot = -1;
  void touch() { version++; }
};
Texture* newTexture(int width, int height);

enum class Shading {
  Standard,  // lit: diffuse + GGX specular from the sun, the sky and point lights, plus emission
  Basic,     // unlit colour
};
enum class Side { Front, Back, Double };

struct Material {
  Shading shading = Shading::Standard;
  Color color{1, 1, 1};
  Color emissive{0, 0, 0};
  float emissiveIntensity = 1;
  float roughness = 1, metalness = 0;
  float opacity = 1;
  bool transparent = false;  // blended and drawn after opaque objects, back to front
  bool depthWrite = true, depthTest = true;
  bool fog = true;
  Side side = Side::Front;
  Texture* map = nullptr;   // multiplies the colour (and the alpha)
  float pointSize = 1;      // for point clouds, in pixels at 720p
  // How much the surface looks like paper (0..1, standard shading only): fibres in the colour, a
  // fine relief in the normal, and light that wraps a little past the terminator. See Scene::paper.
  float paper = 0;

  // Free slots for the application, e.g. an opacity to fade from.
  float user[4] = {-1, 0, 0, 0};

  Material* clone() const;
};
Material* newMaterial();

struct StandardParams {
  uint32_t color = 0xffffff;
  float roughness = 1, metalness = 0;
  uint32_t emissive = 0x000000;
  float emissiveIntensity = 1;
  float opacity = 1;
  bool transparent = false, depthWrite = true;
  Side side = Side::Front;
  Texture* map = nullptr;
  float paper = 0;
};
Material* standard(const StandardParams& p);

struct BasicParams {
  uint32_t color = 0xffffff;
  float opacity = 1;
  bool transparent = false, depthWrite = true, depthTest = true, fog = true;
  Side side = Side::Front;
  Texture* map = nullptr;
};
Material* basic(const BasicParams& p);

}  // namespace eng
