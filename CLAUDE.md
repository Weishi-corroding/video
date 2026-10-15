# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What's here

Two seven-minute animated essay films and the engine behind one of them. Each project has a detailed README. Read the relevant one before making changes, because it records the conceptual and visual decisions the code has to respect.

- **Root (`index.html`, `js/`, `css/`, `vendor/`)**: *Does AI Make Us Smarter—or More Dependent?*, a browser film in Three.js r186 (vendored). `recommend.md` is a review of this film. It lists proposed changes, not implemented ones.
- **`engine/`**: `eng`, a small C++20/Vulkan offline renderer. It covers the scene graph, paper materials, SSAO, DoF, bloom and grading, HarfBuzz/FreeType text, a synthesizer, and MP4 output through ffmpeg. It runs headless, including on Mesa llvmpipe. `engine/ARCHITECTURE.md` (in Chinese) documents the renderer's passes, shading and post algorithms, and its limits. Check it before changing `renderer.cpp` or the shaders, and update it when they change.
- **`emptying-bookshop/`**: *The Emptying Bookshop*, a C++ film that links `eng` and renders to MP4. Chinese is the default language and English is optional. It used to be a Three.js film like the root one, and the C++ port keeps that film's structure.

## Commands

Browser film (no build, no dependencies):
```sh
python3 -m http.server 8000          # or: npm start. ES modules need HTTP, not file://
node tools/check-script.mjs          # pacing check for js/script.js (words/s per cue, overlaps, missing zh)
```
Review URLs: `?t=95` start time, `?still` paused with no start screen, `?play` muted autoplay, `?zh=1`, `?cc=0`.

C++ film (run from `emptying-bookshop/`):
```sh
cmake -S . -B build -G Ninja && ninja -C build
./build/emptying-bookshop --check                 # timing table + script lint; exits 1 on problems
./build/emptying-bookshop --lang en --check
./build/emptying-bookshop --still 95,240          # PNG stills → out/stills/ (fastest way to see a change)
./build/emptying-bookshop --from 180 --to 200 --size 1280x720   # render a section
./build/emptying-bookshop --timeline              # cue ids and their start/end times
./build/emptying-bookshop --audio-only out/soundtrack.wav --stems
./build/emptying-bookshop                         # full film → out/emptying-bookshop.mp4 (en: --lang en)
ENG_VALIDATION=1 ./build/emptying-bookshop --still 10   # or --validate: Khronos validation layer
```
Neither film has unit tests. The checks are the two script linters plus looking at rendered stills. With the software rasteriser, 1080p renders at a few frames a second. Check changes with `--still` or a short `--from/--to` range, not a full render. The binary finds `fonts/` and `narration/` through `FILM_DIR`, which is compiled in (override with `--data`), so it can run from any directory.

## Core architecture (both films)

**Every frame is a pure function of master time `t`.** No state carries from one frame to the next. That is what makes seeking (JS), `--from`/`--to`, and `--still` exact. Animation code computes everything from `t` with helpers like `ramp`, `win`, `pulse`, and the easing functions in `kit`. Never accumulate per-frame state.

**Narration cues drive all timing.** Each chapter module refers to cues by id, never by absolute time:
- JS: `c['4d'].s` in `js/chapters/*.js`. Cue `start`/`end` values are **set by hand** in `js/script.js` (`DURATION = 420`, chapter bounds fixed).
- C++: `c("3d").s`, or `T("3d", dt)` from `chapters/chapters.h`. Cue times are **computed** by `script::layout()` in `src/script.cpp` from each line's length in the film's language: at least `CPS` 4.4 hanzi/s or `WPS` 2.6 words/s, plus `pre`/`hold`/`GAP`/`LEAD`. Each slot grows to fit the recorded speech in `narration/<lang>/durations.tsv`. Editing one line retimes that line and everything after it, and the animation and score follow. `c()` with an unknown id stops the program.

**C++ chapter pattern** (`src/chapters/cN.cpp`): `buildCN(Ctx&)` runs once. It builds props, registers keyframes with the `World` (`addCam`, `addJia`, `addLight`, `addCrowd`, `addShop`, `addWarp`, `onUpdate`), and adds overlay labels through `Labels::add(LabelSpec{... .vis = [](float t){...}})`. It returns an `Update` lambda that animates the chapter's own props from `t`. The JS chapters (`js/chapters/cN.js`, `build(ctx)`) have the same shape.

**C++ frame loop** (`src/main.cpp`): `World::update(t)` (light, camera, crowd, Jia, pop-up, shop, updaters) → each chapter's `Update(t)` → `applyFx()` → `Renderer::render` → readback → `Labels::draw` + `Captions::draw` on the RGBA buffer (2D overlays are drawn on the CPU, after the 3D render) → `VideoWriter`. `World::finalize()` stable-sorts all keys by time and turns `cutAt` times into instant camera keys. Every chapter after the first opens with a page turn (`pageturn.cpp`). The frame at the turn's start is rendered once and cached, the film keeps rendering underneath, and the camera cuts under the page.

**Soundtrack**: `music.cpp` composes the score each run with `eng/synth`, timed from script cues and `ScoreEvents` (page turns, pop-up flaps). Narration WAVs are level-matched and mixed at their cue starts, and the music ducks under them. The score plays louder when no narration is present, which is the current state for Chinese.

**Engine conventions**: hex colours (`0xffcf5a`) are sRGB, and the engine converts them to linear. Factories (`group()`, `mesh()`, `geo::*`, `standard()`, `basic()`, `newTexture()`) return raw pointers that the engine owns for the program's lifetime, so never `delete` them. All Vulkan code is in `engine/src/renderer.cpp`. Shaders in `engine/shaders/` are compiled by `glslc` at build time into `.inc` C arrays that the renderer `#include`s, and every shader depends on `common.glsl`. A new shader must be added to `ENG_SHADERS` in `engine/CMakeLists.txt`.

## After editing the C++ film's script or text

1. Run `tools/narrate.py` again for the affected language. The pipeline is `--cues | python3 tools/narrate.py --lang <l> [--import DIR]`. English uses Piper from `~/.venvs/piper` and re-synthesizes only lines that changed. The tool rewrites `narration/<lang>/durations.tsv`, and **that file is committed**. The WAVs, `manifest.json`, `build/`, and `out/` are gitignored.
2. If you added any Chinese characters, anywhere in `src/`, run `python3 tools/subset-cjk.py`. The CJK fonts in `fonts/` are subsets containing only the characters already used in the source, so new characters come out as missing glyphs.
3. Run `--check` for both `zh` and `en`. It also flags Chinese lines longer than 48 characters, which don't fit on two subtitle lines.

## Content conventions

- All user-facing text is bilingual. In C++, use `script::tr(en, zh)` or the `{en, zh}` pairs in `LabelSpec`/`PanelSpec`. The film renders in one language at a time. In JS, Chinese is an optional extra subtitle line. Keep the two languages equal in meaning.
- Colour is semantic and fixed per film. Labels must also say in words what they are.
  - Bookshop: teal = fact Jia found, coral = assumption/problem, gold = named principle, blue/violet = the two sides of the contradiction.
  - AI film: amber = what the AI produces, mint = the learner's own reasoning. Colour follows the role, never the student. A wears a scarf and B a knit cap.
- No invented data. Characters are fictional and the film says so on screen. Trajectories have no axes or numbers. The philosophical terms ("metaphysical", "principal contradiction", "negation of the negation") are used in specific senses, which each film's README explains under "Conceptual safeguards" or "Conceptual notes". Don't loosen them.
- Bookshop visuals are all paper, built with the `kit.h` helpers: `standard({... .paper = 1})`, faceted geometry, cut card, and paper dolls.
