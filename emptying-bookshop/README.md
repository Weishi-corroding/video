# The Emptying Bookshop

A seven-minute animated film on how to apply Marxist principles to improve dialectical thinking
skills. It is rendered to MP4 by the C++/Vulkan engine in [`../engine`](../engine), with English
narration and captions; a second version adds Simplified Chinese subtitles.

日渐冷清的书店：一部七分钟的动画短片，讲述如何运用马克思主义基本原理提升辩证思维能力。由本仓库的
C++/Vulkan 引擎渲染为 MP4，配有英文旁白和英文字幕；另有一个加中文字幕的版本。

Jia runs a student second-hand bookshop, and fewer people come in each week. The film follows
Jia's choices. First come three quick reactions, then an investigation, the contradiction inside
the problem, small tests that add up, and a look back at the shop's history. Each principle is
shown at work before it is named.

## Make the film

Requirements: CMake 3.20+, a C++20 compiler, Vulkan headers and a driver, glslc, FreeType,
HarfBuzz, libpng and ffmpeg. On Debian or Ubuntu:

```sh
sudo apt install cmake ninja-build g++ libvulkan-dev glslc libfreetype-dev libharfbuzz-dev libpng-dev ffmpeg mesa-vulkan-drivers
```

Then, from this folder:

```sh
cmake -S . -B build -G Ninja && ninja -C build
./build/emptying-bookshop          # → out/emptying-bookshop.mp4 (English captions)
./build/emptying-bookshop --zh     # → out/emptying-bookshop.zh.mp4 (English and Chinese)
```

On a GPU the film renders in a few minutes. With Mesa's software rasteriser (no GPU) it runs at
about 4 frames a second at 1080p, so each version takes about an hour.

Useful options (`--help` lists them all):

- `--still 95,240` writes PNG stills at those times to `out/stills/`.
- `--from 180 --to 200` renders part of the film.
- `--size 1280x720`, `--fps 30` and `--crf 18` set the size, frame rate and quality.
- `--no-audio` leaves the narration out.
- `--check` prints the timing table and checks the script. It flags lines with too little room to
  be spoken and Chinese lines too long for two subtitle lines.

### Narration

The narration is recorded offline with [Piper](https://github.com/OHF-Voice/piper1-gpl), a neural
text-to-speech engine. It uses the `en_GB-cori-high` voice, which was trained on public-domain
LibriVox recordings. The recordings are not in the repository. Make them once (about a minute on
a CPU):

```sh
python3 -m venv ~/.venvs/piper && ~/.venvs/piper/bin/pip install piper-tts
mkdir -p ~/.cache/piper-voices && (cd ~/.cache/piper-voices && ~/.venvs/piper/bin/python -m piper.download_voices en_GB-cori-high)
./build/emptying-bookshop --cues | ~/.venvs/piper/bin/python tools/narrate.py
```

This writes `narration/<cue>.wav` and `narration/durations.tsv`. The timings file is committed, so
the film keeps the same timing without the recordings. If the recordings are missing, the video is
silent and the program says so. `tools/narrate.py` respells a few names for the voice
("Zedong" → "Zuh-dong") without changing the captions.

## Chapters

| Time | Chapter | What the animation shows |
|---|---|---|
| 0:00 | An Emptying Shop | The campus diorama, Jia, and a floating vignette of the shop's origin as a free swap shelf in a dorm lounge. A time-lapse shows fewer visitors week by week. |
| 0:43 | Three Quick Reactions | A heavy verdict, "Nobody reads anymore", drops in front of the shop while the campus dims around it (*isolated*). Another campus's half-price banner is copied onto the shop and changes nothing (*no concrete analysis*; Lenin's "living soul of Marxism"). After one quiet week, the frame freezes into a snapshot (*static*). These are named as metaphysical thinking in the Marxist sense. |
| 2:01 | Go and Find Out | "No investigation, no right to speak." Jia asks students and makes four findings: the shop moved off the main path; it closes at 5, as classes end; readings went online; and there is nowhere to study together in the evening. Each finding becomes a thread back to the shop, and the threads have different thicknesses (*universal connection, unequal weight*). |
| 3:06 | The Pull Inside the Problem | Two pillars pull the shop two ways: what the shop is (a daytime sales counter) and what students now need. An arch shows that each side depends on the other (*unity of opposites*). Small problems (sign, website, prices) are re-hung on the main cables, which turn gold (*principal contradiction, under these conditions*). An outer ring of arrows travels inward (*external causes work through internal causes*). |
| 4:19 | Test, Accumulate, Revise | Three tests: open until 9, a swap shelf, and patio tables. Beads drop into a jar as regulars accumulate, then link into a network (*quantity → quality*). Noise complaints reach the library, and Jia revises the layout (*practice, knowledge, again practice*). |
| 5:42 | Back, but Higher | The shop's history as a helix: swap shelf → shop → swap-and-study room. From above it looks like a circle, and from the side it climbs (*negation of the negation*). |
| 6:33 | Your Own Bookshop | Six numbered pins and a summary of Jia's turning points, a question for the viewer, and one action for this week. |

## How it's built

```
CMakeLists.txt           builds ../engine and the film
src/main.cpp             command line, the frame loop, narration mix, encoding
src/script.cpp           chapters and timed narration cues (English and Chinese)
src/world.cpp            light by time of day, campus, shop, crowd, Jia, camera rig
src/chapters/c1–c7.cpp   one file per chapter: builds its props, labels and keys, and animates them from time
src/kit.cpp              easing, materials, props (figures, shelves, tables, signs, threads…)
src/overlay.cpp          labels anchored to 3D points or the screen, panels, title cards, captions
tools/narrate.py         records the narration with Piper
tools/subset-cjk.py      cuts Noto Sans CJK SC down to the characters the film uses
fonts/                   Inter, Source Serif 4, and the Noto Sans SC subset
narration/durations.tsv  the length of each recorded line
```

Every frame is a pure function of film time, so any part of the film can be rendered on its own.
Chapters register camera, light, crowd, shop-state and Jia keyframes with the world, and every
key time comes from a narration cue (`c("3d").s`). Cue times are laid out from the length of each
line: at least 2.6 words a second, and longer if the recorded voice needs it. Editing a line in
`src/script.cpp` therefore retimes it and everything after it. After editing the script, re-run
`tools/narrate.py` (it re-records only changed lines), `tools/subset-cjk.py` if you added Chinese
characters, and `--check`.

Colour carries meaning, and each coloured label also says in words what it is:
- **teal:** a fact Jia found
- **coral:** an assumption or a problem
- **gold:** a named principle
- **blue / violet:** the two sides of the shop's contradiction

Jia wears a green apron and carries a notebook.

## Conceptual notes

- **"Metaphysical"** is used in the Marxist sense, as the opposite of dialectics (seeing things as isolated, static and one-sided), and the screen says so.
- **Principal contradiction.** The principal contradiction is not presented as "the biggest problem". The film names its two sides, shows that they depend on each other and pull apart, and shows why it shapes the smaller problems: a brighter sign brings people to a shop that still doesn't fit their day. It is named "under these conditions".
- **Internal and external causes.** "Nobody reads anymore" returns as a real outside condition (readings moving online) that acts through how the shop meets its own contradiction.
- **Quantity and quality.** The change in kind is shown as beads becoming a network: a pile of regulars turns into a place where people meet. The only numbers are the story's own ("two regulars, then five").
- **Negation of the negation** is presented as a way to read development, not a formula that predicts it.
- **Quotations:**
  - Mao Zedong, *Oppose Book Worship* (1930): "No investigation, no right to speak."
  - V. I. Lenin, *Kommunismus* (1920): a concrete analysis of a concrete situation as "the living soul of Marxism".
  - Mao Zedong, *On Practice* (1937): "practice, knowledge, again practice, and again knowledge".
- Jia, the shop and the campus are fictional, and the film says so on screen.

## Licence

MIT (see `LICENSE`). Inter, Source Serif 4 and Noto Sans CJK SC are under the SIL Open Font
License 1.1 (`fonts/OFL-*.txt`).
