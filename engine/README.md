# eng — a small C++/Vulkan engine for rendering films

`eng` renders 3D films offline. A program builds a scene, poses it for each moment of the film,
and the engine renders that frame with Vulkan. It reads the frame back, draws 2D overlays (labels,
captions, titles) on top, and pipes the frames to ffmpeg as an H.264 MP4. It needs no window or
display: any Vulkan 1.2 device works, including Mesa's software rasteriser (llvmpipe), so it also
runs on servers.

It was written for [The Emptying Bookshop](../emptying-bookshop), and replaces the three.js
renderer that film used before.

## What it does

- **Scene graph:** nodes with position, Euler rotation (XYZ) and scale, groups, `lookAt`; meshes,
  instanced meshes (per-instance matrix and colour), line segments, point clouds, camera-facing
  sprites and point lights.
- **Geometry:** box, rounded box, plane, circle, sphere (with partial ranges), cylinder/cone,
  capsule, torus, subdivided icosahedron, tubes along curves (drawable progressively with a draw
  range), edge lines of any mesh. Curves: quadratic Bézier, Catmull-Rom, or any function, sampled by
  arc length with parallel-transport frames.
- **Materials:** a lit *standard* material and an unlit *basic* one. The standard material has
  Lambert diffuse plus GGX specular (height-correlated Smith visibility, Schlick Fresnel), with
  roughness, metalness and emission. Both take colour, opacity, an optional sRGB texture,
  front/back/double sides, depth test/write and fog.
- **Lights:** one sun with a soft shadow map (orthographic, 16 filtered comparisons, depth and
  normal bias), a second shadowless directional fill, a sky/ground hemisphere light, and up to
  four point lights with range falloff.
- **Frame:** a sky gradient painted on a sphere around the origin, linear fog, 4× MSAA in a
  half-float HDR target, opaque objects sorted front to back, then transparent objects back to
  front (double-sided ones in two passes, back faces first).
- **Post-processing:** a five-level bloom (bright pass, separable Gaussian blurs at halving
  resolutions, weighted sum) and ACES filmic tone mapping with exposure. Then a grade (saturation,
  vignette, fade to black, flash to white, a focus mask that dims and greys everything outside a
  circle), sRGB encoding and a little dither before quantising to 8 bits.
- **2D:** a premultiplied-alpha canvas (anti-aliased rectangles, rounded rectangles, rings,
  circles, Gaussian-blurred masks for shadows) and text shaped with HarfBuzz and rasterised with
  FreeType. Text supports variable-font weights, letter spacing, uppercase and strike-through.
  Paragraphs wrap with Latin and Chinese line-breaking rules and fall back to a CJK font for
  missing glyphs. Canvases become textures (signs in the scene) or are composited onto finished
  frames, with an optional backdrop blur behind each panel.
- **Output:** `VideoWriter` pipes RGBA frames to ffmpeg (x264, BT.709 tagged, optional AAC audio).
  `savePng` writes stills, and `Audio` loads, mixes and saves mono WAV for narration tracks.

## Using it

```cpp
#include <eng/eng.h>
using namespace eng;

Scene scene;
scene.sky.top = Color::hex(0x1d2a40);
scene.sky.horizon = Color::hex(0x51647a);
scene.hemi = {Color::hex(0xd6e4ff), Color::hex(0x6e5c48), 1.1f};
scene.sun.color = Color::hex(0xfff0dc);
scene.sun.intensity = 2.7f;
scene.sun.position = {40, 60, 50};
scene.sun.castShadow = true;

Mesh* box = mesh(geo::roundedBox(2, 2, 2, 2, 0.2f), standard({.color = 0xc98e6c, .roughness = 0.8f}));
box->position = {0, 1, 0};
box->castShadow = box->receiveShadow = true;
scene.add(box);

Camera cam;
cam.position = {8, 6, 10};
cam.target = {0, 1, 0};
cam.aspect = 16.f / 9;

Renderer renderer({.width = 1920, .height = 1080});
VideoWriter video({.path = "out.mp4", .width = 1920, .height = 1080, .fps = 30});
for (int f = 0; f < 90; f++) {
  box->rotation.y = f / 30.f;
  renderer.render(scene, cam, PostSettings{});
  video.write(renderer.pixels());
}
video.close();
```

Colours given as hex (`0xffcf5a`) are sRGB; the engine converts them and lights in linear space.
Nodes, geometry, materials and textures are owned by the engine and live for the whole program,
so the factories (`group()`, `mesh()`, `geo::box()`, `standard()`, `newTexture()`…) return plain
pointers.

## Building

C++20, CMake 3.20+, and these libraries (Debian/Ubuntu package names):

```sh
sudo apt install cmake ninja-build g++ libvulkan-dev glslc libfreetype-dev libharfbuzz-dev libpng-dev ffmpeg
# a Vulkan driver: your GPU's, or mesa-vulkan-drivers for the software rasteriser
```

Shaders in `shaders/` are compiled to SPIR-V by `glslc` at build time and embedded in the library.
Add the engine to a project with `add_subdirectory(path/to/engine)` and link `eng`.

Set `RendererOptions::validation` (the film also reads `ENG_VALIDATION=1`) to run under the
Khronos validation layer (`vulkan-validationlayers`).

## Layout

```
include/eng/   math.h  scene.h  geometry.h  curve.h  material.h  canvas.h  text.h  renderer.h  video.h  (eng.h: all of them)
src/           the implementations; renderer.cpp holds all the Vulkan code
shaders/       mesh, points and sprite vertex shaders, the lit/unlit fragment shader, shadow depth,
               sky, and the post passes (bright, blur, final)
```

## Licence

MIT, like the films in this repository.
