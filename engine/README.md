# eng — a small C++/Vulkan engine for rendering films

`eng` renders 3D films offline. A program builds a scene, poses it for each moment of the film,
and the engine renders that frame with Vulkan. It reads the frame back, draws 2D overlays (labels,
captions, titles) on top, and pipes the frames to ffmpeg as an H.264 MP4, with a soundtrack the
program can mix and even compose with the engine's small synthesizer. It needs no window or
display: any Vulkan 1.2 device works, including Mesa's software rasteriser (llvmpipe), so it also
runs on servers.

It was written for [The Emptying Bookshop](../emptying-bookshop), and replaces the three.js
renderer that film used before.

## What it does

- **Scene graph:** nodes with position, Euler rotation (XYZ) and scale, groups, `lookAt`; meshes,
  instanced meshes (per-instance matrix and colour), line segments, point clouds, camera-facing
  sprites and point lights.
- **Geometry:** box, rounded box, plane, circle, sphere (with partial ranges), cylinder/cone,
  capsule, torus, subdivided icosahedron, tubes and flat ribbons along curves (drawable
  progressively with a draw range), shapes cut from card (any simple polygon, extruded, with the
  faces and the cut edge separable), faceted copies of any mesh (one normal per triangle, like folded
  paper), merged meshes, and edge lines. Curves: quadratic Bézier, Catmull-Rom, or any function,
  sampled by arc length with parallel-transport frames.
- **Materials:** a lit *standard* material and an unlit *basic* one. The standard material has
  Lambert diffuse plus GGX specular (height-correlated Smith visibility, Schlick Fresnel), with
  roughness, metalness and emission. Both take colour, opacity, an optional sRGB texture,
  front/back/double sides, depth test/write and fog.
- **Paper:** a standard material with `paper` > 0 looks made of paper. A procedural, tileable sheet of
  fibres is mapped onto it from three sides in object space, so it stays put on moving things. It
  mottles the colour, adds a fine relief to the normal (a bump from the fibre height, no tangents
  needed), and lets diffuse light wrap a little past the terminator. `Scene::paper` sets the scale and
  strength, and can put the same fibres into the sky gradient, like a painted backdrop.
- **Lights:** one sun with a soft shadow map (orthographic, 16 filtered comparisons, depth and
  normal bias), a second shadowless directional fill, a sky/ground hemisphere light, and up to
  four point lights with range falloff.
- **Frame:** a sky gradient painted on a sphere around the origin, linear fog, 4× MSAA in a
  half-float HDR target, opaque objects sorted front to back, then transparent objects back to
  front (double-sided ones in two passes, back faces first). Opaque surfaces also write their
  view-space normal and depth to a second target for the passes below.
- **Ambient occlusion:** a hemisphere of samples above each surface at half resolution, then a
  blur that keeps to similar depths. It darkens creases and contact points, and spares bright
  emissive surfaces.
- **Depth of field:** a circle of confusion from the distance to a focus plane, a gather of 37
  samples at half resolution with foreground and background kept apart (so a blurred foreground can
  spill over sharp things behind it, but not the other way round), blended in by each pixel's
  blur. Wide shots of a model with a strong blur read as a miniature (tilt-shift). The sky stays sharp.
- **Post-processing:** a five-level bloom (bright pass, separable Gaussian blurs at halving
  resolutions, weighted sum) and ACES filmic tone mapping with exposure. Then a grade (saturation,
  a focus mask that dims and greys everything outside a circle, contrast, gain and a lift of the
  shadows, vignette, fade to black, flash to white), sRGB encoding and a little dither before
  quantising to 8 bits.
- **2D:** a premultiplied-alpha canvas (anti-aliased rectangles, rounded rectangles, rings,
  circles, Gaussian-blurred masks for shadows) and text shaped with HarfBuzz and rasterised with
  FreeType. Text supports variable-font weights, letter spacing, uppercase and strike-through.
  Paragraphs wrap with Latin and Chinese line-breaking rules. There are Chinese sans and serif
  families with bold faces; Latin text falls back to the matching Chinese family for characters it
  lacks, and the other way round. Canvases become textures (signs in the scene) or are composited onto finished
  frames, with an optional backdrop blur behind each panel.
- **Output:** `VideoWriter` pipes RGBA frames to ffmpeg (x264, BT.709 tagged, optional AAC audio).
  `savePng` writes stills, and `Audio` loads, resamples (windowed sinc), pans, mixes and saves mono or
  stereo WAV.
- **Synthesizer** (`eng/synth.h`): notes rendered offline into a stereo buffer through a shared
  reverb (after Freeverb). Instruments: a felt piano (additive partials with string stiffness, a
  two-stage decay and a soft hammer), a plucked string and a bass (Karplus-Strong), a string pad
  (detuned band-limited saws through a state-variable filter), a music box, and a few noises
  (shaker, wood-block tick, thud, camera shutter, a rising swell, a page turning, a piece of card
  snapping up). Chord names ("A/C#", "Gmaj7") spell out voicings, a ducking curve lowers music under
  a voice, and a look-ahead limiter keeps the mix under full scale.

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
include/eng/   math.h  scene.h  geometry.h  curve.h  material.h  canvas.h  text.h  renderer.h  video.h  synth.h
               (eng.h: all of them)
src/           the implementations; renderer.cpp holds all the Vulkan code, paper.cpp makes the fibre sheet
shaders/       mesh, points and sprite vertex shaders, the lit/unlit fragment shader, shadow depth,
               sky, and the post passes (ssao, aoblur, dofpre, dofgather, bright, blur, final)
```

[`ARCHITECTURE.md`](ARCHITECTURE.md) (in Chinese) explains how the engine works inside. It covers
the render targets and passes of a frame, the shading and post-processing algorithms, text layout,
output, the synthesizer, and the engine's limits.

## Licence

MIT, like the films in this repository.
