# The Emptying Bookshop

A seven-minute animated story built with Three.js on how to apply Marxist principles to improve dialectical thinking skills. It has English narration, English captions, and optional Simplified Chinese subtitles.

日渐冷清的书店：一部七分钟的 Three.js 动画故事，讲述如何运用马克思主义基本原理提升辩证思维能力。配有英文旁白和英文字幕，可选简体中文字幕。

Jia runs a student second-hand bookshop, and fewer people come in each week. The film follows Jia's choices. First come three quick reactions, then an investigation, the contradiction inside the problem, small tests that add up, and a look back at the shop's history. Each principle is shown at work before it is named.

## Run it

The page uses ES modules, so serve the folder over HTTP instead of opening the file directly. From this folder:

```sh
python3 tools/serve.py
# then open http://localhost:8017
```

`tools/serve.py` is a small static server that turns browser caching off. Plain `python3 -m http.server 8000` also works. But if your browser has visited another project on the same port, it may keep using that project's cached files, and the film will stop at "Loading…" or show the other project. If that happens, use `tools/serve.py`, pick another port, or hard-reload (<kbd>Ctrl</kbd>/<kbd>Cmd</kbd>+<kbd>Shift</kbd>+<kbd>R</kbd>).

There is no build step and nothing to install. Three.js r186 is vendored in `vendor/three-r186`, and the Latin fonts (Inter, Source Serif 4) are in `vendor/fonts`. The film therefore works offline and looks the same everywhere. Chinese text uses the system's Chinese font.

## Chapters

| Time | Chapter | What the animation shows |
|---|---|---|
| 0:00 | An Emptying Shop | The campus diorama, Jia, and a floating vignette of the shop's origin as a free swap shelf in a dorm lounge. A time-lapse shows fewer visitors week by week. |
| 0:42 | Three Quick Reactions | A heavy verdict, "Nobody reads anymore", drops in front of the shop while the campus dims around it (*isolated*). Another campus's half-price banner is copied onto the shop and changes nothing (*no concrete analysis*; Lenin's "living soul of Marxism"). After one quiet week, the frame freezes into a snapshot (*static*). These are named as metaphysical thinking in the Marxist sense. |
| 1:55 | Go and Find Out | "No investigation, no right to speak." Jia asks students and makes four findings: the shop moved off the main path; it closes at 5, as classes end; readings went online; and there is nowhere to study together in the evening. Each finding becomes a thread back to the shop, and the threads have different thicknesses (*universal connection, unequal weight*). |
| 2:59 | The Pull Inside the Problem | Two pillars pull the shop two ways: what the shop is (a daytime sales counter) and what students now need. An arch shows that each side depends on the other (*unity of opposites*). Small problems (sign, website, prices) are re-hung on the main cables, which turn gold (*principal contradiction, under these conditions*). An outer ring of arrows travels inward (*external causes work through internal causes*). |
| 4:12 | Test, Accumulate, Revise | Three tests: open until 9, a swap shelf, and patio tables. Beads drop into a jar as regulars accumulate, then link into a network (*quantity → quality*). Noise complaints reach the library, and Jia revises the layout (*practice, knowledge, again practice*). |
| 5:33 | Back, but Higher | The shop's history as a helix: swap shelf → shop → swap-and-study room. From above it looks like a circle, and from the side it climbs (*negation of the negation*). |
| 6:21 | Your Own Bookshop | Six numbered pins and a summary of Jia's turning points, a question for the viewer, and one action for this week. |

## Player

- **Play/pause, seeking, chapters:** click the picture or press <kbd>Space</kbd>. The seek bar is split by chapter. <kbd>←</kbd>/<kbd>→</kbd> seek ±5 s (hold <kbd>Shift</kbd> for 15 s). <kbd>↑</kbd>/<kbd>↓</kbd> or the Chapters menu move between chapters, and <kbd>1</kbd>–<kbd>7</kbd> jump straight to one.
- **Speed:** 0.5× to 2× (<kbd>,</kbd>/<kbd>.</kbd>).
- **Captions:** <kbd>C</kbd> toggles English captions and <kbd>Z</kbd> toggles Simplified Chinese subtitles. With Chinese on, the on-screen labels also show a Chinese line.
- **Narration:** <kbd>N</kbd> toggles narration, which uses the browser's speech synthesis with an English system voice. Choose the voice from the caret next to the Voice button. If no English voice exists, narration is disabled and the captions carry the full script.
- **Fullscreen:** <kbd>F</kbd>.
- Settings are saved in `localStorage`.

Each narration line is spoken as its own short utterance. If a voice is slower than the script allows, the clock waits before the next line, for up to 6 s, so picture and voice stay in step. After any seek or resume, from the keyboard or the seek bar, the current line is re-voiced if the playhead is in its first half. Otherwise narration picks up at the next line.

For review, these URL options help: `?t=95` starts at a time, `?still` skips the start screen and stays paused, `?play` skips the start screen and plays without narration, `?zh=1` turns Chinese on, `?cc=0` turns English captions off, and `?clean` hides the controls.

## How it's built

```
index.html               player markup, import map
css/style.css            palette, typography, labels, player chrome
src/script.js            chapters and timed narration cues (English and Chinese)
src/world.js             renderer, light by time of day, campus, shop, crowd, Jia, camera rig
src/chapters/c1–c7.js    one module per chapter: builds its props and animates them from time
src/kit.js               easing, materials, props (figures, shelves, tables, signs, threads…)
src/labels.js            HTML labels anchored to 3D points or the screen
src/timeline.js          master clock (play, pause, seek, speed)
src/narration.js         speech synthesis wrapper
src/ui.js                seek bar, menus, toggles, keys, captions, start screen
tools/check-script.mjs   pacing and consistency check for the script
tools/serve.py           local static server with caching off
vendor/three-r186, vendor/fonts
```

Everything on screen is a pure function of master time, so seeking, scrubbing, and speed changes are exact and never drift. Chapters register camera, light, crowd, shop-state and Jia keyframes with the world, and every key time comes from a narration cue (`c['3d'].s`). Cue times are laid out from word counts, so editing a line in `src/script.js` retimes it and everything that follows. After editing the script, run `node tools/check-script.mjs`. It reports lines that are too dense to voice in their slot, Chinese lines too long for two subtitle lines, and references to cue ids that no longer exist.

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

MIT (see `LICENSE`). Three.js is © three.js authors, MIT, in `vendor/three-r186/LICENSE`. Inter and Source Serif 4 are under the SIL Open Font License 1.1, in `vendor/fonts/OFL-*.txt`.
